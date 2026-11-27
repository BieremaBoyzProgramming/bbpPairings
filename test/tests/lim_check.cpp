void TEST_FUNCTION(const testing::Context &context)
{
  auto output_path =
    context.data_folder_path / (STRINGIFY(TEST_ID) ".output");
  testing::run(
    context.exe_path.string()
    + " --lim "
    + (context.data_folder_path / STRINGIFY(TEST_ID) ".input").string()
    + " -c > "
    + output_path.string());
  testing::assert_file_content_matches(
    output_path,
    context.data_folder_path / STRINGIFY(TEST_ID) ".output.expected");
}
