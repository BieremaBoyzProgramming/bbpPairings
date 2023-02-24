// https://github.com/BieremaBoyzProgramming/bbpPairings/issues/15
void TEST_FUNCTION(const testing::Context &context)
{
  auto output_filename = STRINGIFY(TEST_ID) ".output";
  testing::run(
    context.exe_path.string()
    + " --burstein "
    + (context.data_folder_path / STRINGIFY(TEST_ID) ".input").string()
    + " -c > "
    + (context.data_folder_path / output_filename).string());
  testing::assert_file_content_matches(
    context.data_folder_path / output_filename,
    context.data_folder_path / STRINGIFY(TEST_ID) ".output.expected");
}
