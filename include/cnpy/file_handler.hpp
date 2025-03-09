#ifndef FILE_HANDLER_HPP
#define FILE_HANDLER_HPP

#include <cstdio>
#include <string_view>

class FileHandler {
public:
  FileHandler(std::string_view file_name, std::string_view mode);
  FileHandler(FileHandler &&) = default;
  FileHandler(const FileHandler &) = delete;
  FileHandler &operator=(FileHandler &&) = default;
  FileHandler &operator=(const FileHandler &) = delete;
  ~FileHandler();
  [[nodiscard]] FILE *get() const;
  [[nodiscard]] bool is_open() const;

private:
  FILE *fp_ = nullptr;
  std::string_view file_name_;
  std::string_view mode_;
};

#endif // FILE_HANDLER_HPP
