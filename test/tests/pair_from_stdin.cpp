void TEST_FUNCTION(const testing::Context &context)
{
  auto output_filename = STRINGIFY(TEST_ID) ".output";
  testing::run(
    context.exe_path.string()
    // + " --dutch -p "
    // + output_filename
    // + " < "
    // + (context.data_folder_path / "issue_7.input").string());
    + " --dutch "
    + (context.data_folder_path / "issue_7.input").string()
    + " -p "
    + output_filename);
  testing::assert_file_content_matches(
    context.data_folder_path / output_filename,
    context.data_folder_path / "issue_7.output.expected");
}
