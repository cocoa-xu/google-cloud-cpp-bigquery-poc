#include <absl/log/initialize.h>
#include <google/cloud/bigquery/storage/v1/bigquery_read_client.h>
#include <iostream>
#include <fstream>

void DumpRowsInArrowFormat(
    ::google::cloud::bigquery::storage::v1::ArrowSchema const& schema,
    ::google::cloud::bigquery::storage::v1::ArrowRecordBatch const& rows,
    size_t batch_index) {
  auto aschema = schema.serialized_schema();
  auto undecoded = rows.serialized_record_batch();
  std::ofstream schema_file("schema_" + std::to_string(batch_index) + ".arrow", std::ios::binary);
  schema_file.write(aschema.data(), aschema.length());
  schema_file.close();
  std::ofstream record_batch_file("record_batch_" + std::to_string(batch_index) + ".arrow", std::ios::binary);
  record_batch_file.write(undecoded.data(), undecoded.length());
  record_batch_file.close();
  std::cout << "wrote batch [" + std::to_string(batch_index) + "] schema length: " << aschema.length() << "\n";
  std::cout << "wrote batch [" + std::to_string(batch_index) + "] record_batch length: " << undecoded.length() << "\n";
}

int main(int argc, char* argv[]) try {
  absl::InitializeLog();
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <project-id> <table-name>\n";
    return 1;
  }

  // project_name should be in the format "projects/<your-gcp-project>"
  std::string const project_name = "projects/" + std::string(argv[1]);
  // table_name should be in the format:
  // "projects/<project-table-resides-in>/datasets/<dataset-table_resides-in>/tables/<table
  // name>" The project values in project_name and table_name do not have to be
  // identical.
  std::string const table_name = argv[2];

  // Create a namespace alias to make the code easier to read.
  namespace bigquery_storage = ::google::cloud::bigquery_storage_v1;
  constexpr int kMaxReadStreams = 1;

  // Create the ReadSession.
  auto client = bigquery_storage::BigQueryReadClient(
      bigquery_storage::MakeBigQueryReadConnection());
  ::google::cloud::bigquery::storage::v1::ReadSession read_session;
  read_session.set_data_format(
      google::cloud::bigquery::storage::v1::DataFormat::ARROW);
  read_session.set_table(table_name);
  auto session =
      client.CreateReadSession(project_name, read_session, kMaxReadStreams);
  if (!session) throw std::move(session).status();

  // Read rows from the ReadSession.
  constexpr int kRowOffset = 0;
  auto read_rows = client.ReadRows(session->streams(0).name(), kRowOffset);

  std::int64_t num_rows = 0;
  size_t batch_index = 0;
  for (auto const& row : read_rows) {
    if (row.ok()) {
      num_rows += row->row_count();
      DumpRowsInArrowFormat(session->arrow_schema(), row->arrow_record_batch(), batch_index);
      batch_index++;
    }
  }

  std::ofstream num_rows_file("query_info.txt");
  num_rows_file << num_rows << ' ' << batch_index << '\n';
  num_rows_file.close();
  std::cout << "wrote query_info.txt\n";

  std::cout << num_rows << " rows read from table: " << table_name << "\n";
  return 0;
} catch (google::cloud::Status const& status) {
  std::cerr << "google::cloud::Status thrown: " << status << "\n";
  return 1;
}
