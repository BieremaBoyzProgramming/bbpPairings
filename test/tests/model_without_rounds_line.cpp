// A tournament generated from a model file that has no 142/XXR line must still
// state the total number of rounds (the model-based writer appends "142 n").
void TEST_FUNCTION(const testing::Context &context)
{
  const auto output_path =
    context.data_folder_path / (STRINGIFY(TEST_ID) ".output");
  testing::run(
    context.exe_path.string()
    + " --dutch "
    + (context.data_folder_path / STRINGIFY(TEST_ID) ".input").string()
    + " -g -o "
    + output_path.string()
    + " -s 1");
  std::ifstream stream(output_path, std::ios::binary);
  const std::string content{
    std::istreambuf_iterator<char>(stream),
    std::istreambuf_iterator<char>() };
  if (content.find("\r142 3\r") == std::string::npos)
  {
    throw std::runtime_error(
      "File " + output_path.string() + " does not contain the line \"142 3\".");
  }
}
