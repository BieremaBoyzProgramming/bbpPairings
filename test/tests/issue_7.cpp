// https://github.com/BieremaBoyzProgramming/bbpPairings/issues/7
void TEST_FUNCTION()
{
  auto output_path = testing::data_folder_path + STRINGIFY(TEST_ID) ".output";
  testing::run(
    testing::exe_path
    + " --dutch "
    + testing::data_folder_path
    + STRINGIFY(TEST_ID) ".input -p "
    + output_path);
  testing::assert_file_content_matches
    (output_path, testing::data_folder_path + STRINGIFY(TEST_ID) ".output.expected");
}
