#include <iostream>
#include <fstream>
#include <vector>

void parse_query(const std::string& schema, const std::string& record_batch, size_t batch_index) {
    std::cout << "parsing batch [" + std::to_string(batch_index) + "] schema length: " << schema.length() << "\n";
    std::cout << "parsing batch [" + std::to_string(batch_index) + "] record_batch length: " << record_batch.length() << "\n";

    
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
    std::cout << "num_rows: " << num_rows << "\n";
    std::cout << "nbatches: " << nbatches << "\n";

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
