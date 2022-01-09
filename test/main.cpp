#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace testing
{
  struct Context
  {
    Context(std::filesystem::path, std::filesystem::path);

    const std::filesystem::path exe_path;
    const std::filesystem::path data_folder_path;
  };

  void assert_file_content_matches(
    const std::filesystem::path &path1,
    const std::filesystem::path &path2)
  {
    std::ifstream stream1(path1);
	  std::ifstream stream2(path2);
    std::streampos line = 1;
    std::streampos column = 1;
	  while (stream1.good() && stream2.good())
    {
      auto c1 = stream1.get();
      auto c2 = stream2.get();
      if (
        c1 != c2
        && (stream1.good() || stream1.eof())
        && (stream2.good() || stream2.eof()))
      {
        throw std::runtime_error(
          "File "
            + path1.string()
            + " did not match file "
            + path2.string()
            + " at line "
            + std::to_string(line)
            + ", column "
            + std::to_string(column)
            + ".");
      }
      if (c1 == '\n')
      {
        line += 1;
        column = 1;
      }
      else
      {
        column += 1;
      }
    }
    if (!stream1.good() && !stream1.eof())
    {
      throw std::runtime_error("Error reading file " + path1.string());
    }
    if (!stream2.good() && !stream2.eof())
    {
      throw std::runtime_error("Error reading file " + path2.string());
    }
  }

  void run(const std::string &command)
  {
    auto result = std::system(command.data());
    if (result != 0)
    {
      throw std::runtime_error("Command " + command + " returned status code " + std::to_string(result));
    }
  }
}

#define TEST_FUNCTION_FOR(test_id) test_##test_id
#define RUN_MACRO(macro, ...) macro(__VA_ARGS__)
#define TEST_FUNCTION RUN_MACRO(TEST_FUNCTION_FOR, TEST_ID)
#define BEFORE_RUNNING_TESTS int result = 0;
#define RUN_TEST(test_id) \
  std::cout << "Running test " #test_id ":" << std::endl; \
  try \
  { \
    TEST_FUNCTION_FOR(test_id)(context); \
    std::cout << "...Passed!" << std::endl; \
  } \
  catch (const std::exception &exception) \
  { \
    std::cout << "Error: " << exception.what() << std::endl; \
    result = 1; \
  }
#define AFTER_RUNNING_TESTS return result;
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#include <test-includes.h>

namespace testing
{
  Context::Context(
      std::filesystem::path exe_path_,
      std::filesystem::path data_folder_path_)
    : exe_path(exe_path_.make_preferred()),
      data_folder_path(data_folder_path_.make_preferred())
  { }
}

int main(const int argc, char**const argv)
{
  if (argc != 3)
  {
    std::cerr
      << "Command line argument syntax:"
      << std::endl
      << argv[0]
      << " path-to-bbpPairings.exe path-to-tests-directory"
      << std::endl;
    return 1;
  }
  return runTests(testing::Context(argv[1], argv[2]));
}
