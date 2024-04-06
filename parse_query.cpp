#include <iostream>
#include <fstream>
#include <vector>

#include <flatbuffers/flatbuffers.h>
#include <arrow/Schema_generated.h>
#include <arrow/Message_generated.h>
#include <nanoarrow/nanoarrow.h>

// a helper function for debugging
const char * arrow_schema_format_to_str(const char * format);

void * parse_encapsulated_message(uintptr_t * data_ptr, org::apache::arrow::flatbuf::MessageHeader expected_header, void * private_data) {
    // https://arrow.apache.org/docs/format/Columnar.html#encapsulated-message-format
    bool continuation = *(uint32_t *)data_ptr == 0xFFFFFFFF;
    data_ptr = (uintptr_t *)(((uint64_t)(uint64_t *)data_ptr) + 4);

    // metadata_size:
    // A 32-bit little-endian length prefix indicating the metadata size
    int32_t metadata_size = *(int32_t *)data_ptr;
#if ADBC_DRIVER_BIGQUERY_ENDIAN == 0
    metadata_size = __builtin_bswap32(metadata_size);
#endif
    data_ptr = (uintptr_t *)(((uint64_t)(uint64_t *)data_ptr) + 4);
    printf("  continuation: %d\n", continuation);
    printf("  metadata_size: %d\n", metadata_size);
    auto header = org::apache::arrow::flatbuf::GetMessage(data_ptr);
    auto body_data = (uintptr_t *)(((uint64_t)(uint64_t *)data_ptr) + metadata_size);
    printf("body_data = %p\n", body_data);
    auto header_type = header->header_type();
    printf("  header_type: %hhu\r\n", header_type);
    if (header_type == expected_header) {
        if (header_type == org::apache::arrow::flatbuf::MessageHeader::Schema) {
            std::cout << "  Schema\n";
            auto schema = header->header_as_Schema();
            auto fields = schema->fields();
            for (size_t i = 0; i < fields->size(); i++) {
                auto field = fields->Get(i);
                std::cout << "    name=" << field->name()->str() << ", ";
                std::cout << "type=" << org::apache::arrow::flatbuf::EnumNameType(field->type_type()) << "\n";
            }

            // user will pass an initialized schema in adbc_driver_bigquery
            struct ArrowSchema * out = new struct ArrowSchema;
            ArrowSchemaInit(out);
            out->name = nullptr;
            ArrowSchemaSetTypeStruct(out, fields->size());

            const org::apache::arrow::flatbuf::Int * field_int = nullptr;
            const org::apache::arrow::flatbuf::FloatingPoint * field_fp = nullptr;
            const org::apache::arrow::flatbuf::Decimal * field_decimal = nullptr;
            const org::apache::arrow::flatbuf::Date * field_date = nullptr;
            for (size_t i = 0; i < fields->size(); i++) {
                auto field = fields->Get(i);
                auto child = out->children[i];
                ArrowSchemaSetName(child, field->name()->str().c_str());
                switch (field->type_type())
                {
                case org::apache::arrow::flatbuf::Type::NONE:
                    ArrowSchemaSetType(child, NANOARROW_TYPE_UNINITIALIZED);
                    break;
                case org::apache::arrow::flatbuf::Type::Null:
                    ArrowSchemaSetType(child, NANOARROW_TYPE_NA);
                    break;
                case org::apache::arrow::flatbuf::Type::Int:
                    field_int = field->type_as_Int();
                    if(field_int->is_signed()) {
                        if (field_int->bitWidth() == 8) {
                            ArrowSchemaSetType(child, NANOARROW_TYPE_INT8);
                        } else if (field_int->bitWidth() == 16) {
                            ArrowSchemaSetType(child, NANOARROW_TYPE_INT16);
                        } else if (field_int->bitWidth() == 32) {
                            ArrowSchemaSetType(child, NANOARROW_TYPE_INT32);
                        } else {
                            ArrowSchemaSetType(child, NANOARROW_TYPE_INT64);
                        }
                    } else {
                        if (field_int->bitWidth() == 8) {
                            ArrowSchemaSetType(child, NANOARROW_TYPE_UINT8);
                        } else if (field_int->bitWidth() == 16) {
                            ArrowSchemaSetType(child, NANOARROW_TYPE_UINT16);
                        } else if (field_int->bitWidth() == 32) {
                            ArrowSchemaSetType(child, NANOARROW_TYPE_UINT32);
                        } else {
                            ArrowSchemaSetType(child, NANOARROW_TYPE_UINT64);
                        }
                    }
                    break;
                case org::apache::arrow::flatbuf::Type::FloatingPoint:
                    field_fp = field->type_as_FloatingPoint();
                    if (field_fp->precision() == org::apache::arrow::flatbuf::Precision::HALF) {
                        ArrowSchemaSetType(child, NANOARROW_TYPE_HALF_FLOAT);
                    } else if (field_fp->precision() == org::apache::arrow::flatbuf::Precision::SINGLE) {
                        ArrowSchemaSetType(child, NANOARROW_TYPE_FLOAT);
                    } else {
                        ArrowSchemaSetType(child, NANOARROW_TYPE_DOUBLE);
                    }
                    ArrowSchemaSetType(child, NANOARROW_TYPE_DOUBLE);
                    break;
                case org::apache::arrow::flatbuf::Type::Binary:
                    ArrowSchemaSetType(child, NANOARROW_TYPE_BINARY);
                    break;
                case org::apache::arrow::flatbuf::Type::Utf8:
                    ArrowSchemaSetType(child, NANOARROW_TYPE_STRING);
                    break;
                case org::apache::arrow::flatbuf::Type::Bool:
                    ArrowSchemaSetType(child, NANOARROW_TYPE_BOOL);
                    break;
                case org::apache::arrow::flatbuf::Type::Decimal:
                    field_decimal = field->type_as_Decimal();
                    if (field_decimal->bitWidth() == 128) {
                        ArrowSchemaSetType(child, NANOARROW_TYPE_DECIMAL128);
                    } else if(field_decimal->bitWidth() == 256) {
                        ArrowSchemaSetType(child, NANOARROW_TYPE_DECIMAL256);
                    }
                    break;
                case org::apache::arrow::flatbuf::Type::Date:
                    // NANOARROW_TYPE_DATE32?
                    // NANOARROW_TYPE_DATE64?
                    field_date = field->type_as_Date();
                    if (field_date->unit() == org::apache::arrow::flatbuf::DateUnit::DAY) {
                        ArrowSchemaSetType(child, NANOARROW_TYPE_DATE32);
                    } else {
                        ArrowSchemaSetType(child, NANOARROW_TYPE_DATE64);
                    }
                    break;
                case org::apache::arrow::flatbuf::Type::Time:
                    // NANOARROW_TYPE_TIME32?
                    // NANOARROW_TYPE_TIME64?
                    ArrowSchemaSetType(child, NANOARROW_TYPE_TIME64);
                    break;
                case org::apache::arrow::flatbuf::Type::Timestamp:
                    ArrowSchemaSetType(child, NANOARROW_TYPE_TIMESTAMP);
                    break;
                case org::apache::arrow::flatbuf::Type::Interval:
                    // NANOARROW_TYPE_INTERVAL_MONTHS?
                    // NANOARROW_TYPE_INTERVAL_DAY_TIME?
                    // NANOARROW_TYPE_INTERVAL_MONTH_DAY_NANO?
                    ArrowSchemaSetType(child, NANOARROW_TYPE_INTERVAL_DAY_TIME);
                    break;
                case org::apache::arrow::flatbuf::Type::List:
                    ArrowSchemaSetType(child, NANOARROW_TYPE_LIST);
                    break;
                case org::apache::arrow::flatbuf::Type::Struct_:
                    // todo: recursively parse struct
                    ArrowSchemaSetType(child, NANOARROW_TYPE_STRUCT);
                    break;
                case org::apache::arrow::flatbuf::Type::Union:
                    // NANOARROW_TYPE_SPARSE_UNION?
                    // NANOARROW_TYPE_DENSE_UNION?
                    ArrowSchemaSetType(child, NANOARROW_TYPE_SPARSE_UNION);
                    break;
                case org::apache::arrow::flatbuf::Type::FixedSizeBinary:
                    ArrowSchemaSetType(child, NANOARROW_TYPE_FIXED_SIZE_BINARY);
                    break;
                case org::apache::arrow::flatbuf::Type::FixedSizeList:
                    ArrowSchemaSetType(child, NANOARROW_TYPE_FIXED_SIZE_LIST);
                    break;
                case org::apache::arrow::flatbuf::Type::Map:
                    ArrowSchemaSetType(child, NANOARROW_TYPE_MAP);
                    break;
                case org::apache::arrow::flatbuf::Type::Duration:
                    ArrowSchemaSetType(child, NANOARROW_TYPE_DURATION);
                    break;
                case org::apache::arrow::flatbuf::Type::LargeBinary:
                    ArrowSchemaSetType(child, NANOARROW_TYPE_LARGE_BINARY);
                    break;
                case org::apache::arrow::flatbuf::Type::LargeUtf8:
                    ArrowSchemaSetType(child, NANOARROW_TYPE_LARGE_STRING);
                    break;
                case org::apache::arrow::flatbuf::Type::LargeList:
                    ArrowSchemaSetType(child, NANOARROW_TYPE_LARGE_LIST);
                    break;
                case org::apache::arrow::flatbuf::Type::RunEndEncoded:
                    // no idea which type may correspond to this
                    // but according to the spec, the format string is "+r"
                    child->format = "+r";
                    break;
                
                // not quite sure which type(s) the following *View types correspond to
                // but they do appear on the specs, and have a format string associated
                case org::apache::arrow::flatbuf::Type::BinaryView:
                    child->format = "vz";
                    break;
                case org::apache::arrow::flatbuf::Type::Utf8View:
                    child->format = "vu";
                    break;
                case org::apache::arrow::flatbuf::Type::ListView:
                    child->format = "+vl";
                    break;
                case org::apache::arrow::flatbuf::Type::LargeListView:
                    child->format = "+vL";
                    break;
                default:
                    // malformed schema?
                    break;
                }
            }
            
            return out;
        } else if (header_type == org::apache::arrow::flatbuf::MessageHeader::RecordBatch) {
            std::cout << "record batch\n";
            struct ArrowSchema * schema = (struct ArrowSchema *)private_data;
            auto data_header = header->header_as_RecordBatch();
            auto nodes = data_header->nodes();

            auto buffers = data_header->buffers();
            printf("  buffers: length=%d\n", buffers->size());
            int buffer_index = 0;

            struct ArrowArray * out = (struct ArrowArray *)malloc(sizeof(struct ArrowArray));
            memset(out, 0, sizeof(struct ArrowArray));

            out->n_children = nodes->size();
            out->children = (struct ArrowArray **)malloc(sizeof(struct ArrowArray *) * out->n_children);
            for (size_t i = 0; i < nodes->size(); i++) {
                out->children[i] = (struct ArrowArray *)malloc(sizeof(struct ArrowArray));
                memset(out->children[i], 0, sizeof(struct ArrowArray));
            }

            for (size_t i = 0; i < nodes->size(); i++) {
                auto node = nodes->Get(i);
                auto child_schema = schema->children[i];
                
                // ==== debug ====
                std::cout << "    FieldNode " << i << ": " << arrow_schema_format_to_str(schema->children[i]->format) << ' ' << schema->children[i]->name << "\n";
                std::cout << "      node: length=" << node->length() << ", ";
                std::cout << "null_count=" << node->null_count() << "\n";
                // ==== debug ====

                auto child = out->children[i];
                child->length = node->length();
                child->null_count = node->null_count();
                
                child->n_buffers = 2;
                int format_len = strlen(child_schema->format);
                if ((format_len == 1 && (strncmp(child_schema->format, "u", 1) == 0 || strncmp(child_schema->format, "U", 1) == 0) || (format_len == 2 && strncmp(child_schema->format, "vu", 2) == 0))) {
                    child->n_buffers = 3;
                }
                child->buffers = (const void **)malloc(sizeof(uint8_t *) * child->n_buffers);
                printf("      child_schema->format: %s\n", child_schema->format);
                printf("      child->n_buffers: %lld\n", child->n_buffers);

                printf("      buffer[%d]: ", buffer_index);
                auto buffer = buffers->Get(buffer_index);
                printf("offset=%lld, length=%lld\n", buffer->offset(), buffer->length());
                child->buffers[0] = (const uint8_t *)(((uint64_t)(uint64_t*)body_data) + buffer->offset());
                buffer_index++;

                printf("      buffer[%d]: ", buffer_index);
                buffer = buffers->Get(buffer_index);
                printf("offset=%lld, length=%lld\n", buffer->offset(), buffer->length());
                child->buffers[1] = (const uint8_t *)(((uint64_t)(uint64_t*)body_data) + buffer->offset());
                buffer_index++;

                if (child->n_buffers == 3) {
                    printf("      buffer[%d]: ", buffer_index);
                    buffer = buffers->Get(buffer_index);
                    printf("offset=%lld, length=%lld\n", buffer->offset(), buffer->length());
                    child->buffers[2] = (const uint8_t *)(((uint64_t)(uint64_t*)body_data) + buffer->offset());
                    buffer_index++;
                }
            }
            return out;
        } else if (header_type == org::apache::arrow::flatbuf::MessageHeader::DictionaryBatch) {
            std::cout << "dictionary batch\n";
            auto dictionary_batch = header->header_as_DictionaryBatch();
            auto id = dictionary_batch->id();
            auto data = dictionary_batch->data();
            return nullptr;
        } else {
            // The columnar IPC protocol utilizes a one-way stream of binary messages of these types:
            //
            // - Schema
            // - RecordBatch
            // - DictionaryBatch
            std::cout << "unexpected format\n";
            return nullptr;
        }
    } else {
        // error?
        return nullptr;
    }
}

void parse_query(const std::string& serialized_schema, const std::string& serialized_record_batch, size_t batch_index) {
    // https://arrow.apache.org/docs/format/Columnar.html#encapsulated-message-format
    std::cout << "parsing batch [" + std::to_string(batch_index) + "] schema length: " << serialized_schema.length() << "\n";
    struct ArrowSchema * parsed_schema = (struct ArrowSchema *)parse_encapsulated_message(
        (uintptr_t *)serialized_schema.data(), 
        org::apache::arrow::flatbuf::MessageHeader::Schema, 
        nullptr);

    std::cout << "parsing batch [" + std::to_string(batch_index) + "] record_batch length: " << serialized_record_batch.length() << "\n";
    parse_encapsulated_message(
        (uintptr_t *)serialized_record_batch.data(),
        org::apache::arrow::flatbuf::MessageHeader::RecordBatch,
        parsed_schema);
}

int main(int argc, char* argv[]) {
    std::ifstream query_info_file("query_info.txt");
    if (!query_info_file.is_open()) {
        std::cerr << "Error: could not open query_info.txt\n";
        return 1;
    }

    size_t num_rows = 0;
    size_t nbatches = 0;
    query_info_file >> num_rows >> nbatches;
    query_info_file.close();

    std::cout << "read query_info.txt\n";
    std::cout << "  num_rows: " << num_rows << "\n";
    std::cout << "  nbatches: " << nbatches << "\n";

    for (size_t batch_index = 0; batch_index < nbatches; batch_index++) {
        std::ifstream schema_file("schema_" + std::to_string(batch_index) + ".arrow", std::ios::binary);
        std::ifstream record_batch_file("record_batch_" + std::to_string(batch_index) + ".arrow", std::ios::binary);
        if (!schema_file.is_open() || !record_batch_file.is_open()) {
            std::cerr << "Error: could not open files for batch " << batch_index << "\n";
            return 1;
        }
        schema_file.seekg(0, std::ios::end);
        size_t schema_size = schema_file.tellg();
        schema_file.seekg(0, std::ios::beg);
        record_batch_file.seekg(0, std::ios::end);
        size_t record_batch_size = record_batch_file.tellg();
        record_batch_file.seekg(0, std::ios::beg);
        
        std::vector<char> schema_buffer(schema_size);
        std::vector<char> record_batch_buffer(record_batch_size);
        schema_file.read(schema_buffer.data(), schema_size);
        record_batch_file.read(record_batch_buffer.data(), record_batch_size);
        schema_file.close();
        record_batch_file.close();

        parse_query(std::string(schema_buffer.data(), schema_size), std::string(record_batch_buffer.data(), record_batch_size), batch_index);
    }
}

#include <map>

const char * arrow_schema_format_to_str(const char * format) {
    // https://arrow.apache.org/docs/format/CDataInterface.html#data-type-description-format-strings
    static std::map<std::string, const char *> format_map = {
        {"n", "null"},
        {"b", "boolean"},
        {"c", "int8"},
        {"C", "uint8"},
        {"s", "int16"},
        {"S", "uint16"},
        {"i", "int32"},
        {"I", "uint32"},
        {"l", "int64"},
        {"L", "uint64"},
        {"e", "float16"},
        {"f", "float32"},
        {"g", "float64"},

        {"z", "binary"},
        {"Z", "large-binary"},
        {"vz", "binary-view"},
        {"u", "utf-8"},
        {"U", "large-utf-8"},
        {"vu", "utf-8-view"},

        {"tdD", "date32-days"},
        {"tdm", "date64-milliseconds"},
        {"tts", "time32-seconds"},
        {"ttm", "time32-milliseconds"},
        {"ttu", "time64-microseconds"},
        {"ttn", "time64-nanoseconds"},
        {"tDs", "duration-seconds"},
        {"tDm", "duration-milliseconds"},
        {"tDu", "duration-microseconds"},
        {"tDn", "duration-nanoseconds"},
        {"tiM", "interval-months"},
        {"tiD", "interval-days-time"},
        {"tin", "interval-month-day-nanoseconds"},

        {"+l", "list"},
        {"+L", "large-list"},
        {"+vl", "list-view"},
        {"+vL", "large-list-view"},
        {"+s", "struct"},
        {"+m", "map"},
        {"+r", "run-end encoded"},
    };
    auto it = format_map.find(format);
    if (it != format_map.end()) {
        return it->second;
    } else {
        return format;
    }
}
