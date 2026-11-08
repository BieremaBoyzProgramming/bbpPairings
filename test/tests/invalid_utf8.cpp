// A file that is not legal UTF-8 is reported as a file format error, not as an
// unexpected error.
void TEST_FUNCTION(const testing::Context &context)
{
  const auto error_path =
    context.data_folder_path / (STRINGIFY(TEST_ID) ".output");
  const std::string command =
    context.exe_path.string()
      + " --dutch "
      + (context.data_folder_path / STRINGIFY(TEST_ID) ".input").string()
      + " -p 2> "
      + error_path.string();
  if (std::system(command.data()) == 0)
  {
    throw std::runtime_error("Command " + command + " succeeded.");
  }
  std::ifstream stream(error_path, std::ios::binary);
  const std::string content{
    std::istreambuf_iterator<char>(stream),
    std::istreambuf_iterator<char>() };
  if (
    content.find("The file is not legal UTF-8.") == std::string::npos
      || content.find("Unexpected error") != std::string::npos)
  {
    throw std::runtime_error(
      "Unexpected error output in " + error_path.string() + ": " + content);
  }
}
