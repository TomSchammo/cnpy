
#include "../include/cnpy/file_handler.hpp"

FileHandler::FileHandler(std::string_view file_name, std::string_view mode)
    : file_name_(file_name), mode_(mode) {
  this->fp_ = fopen(file_name.data(), mode.data());
}

FileHandler::~FileHandler() { fclose(this->fp_); }

FILE *FileHandler::get() const { return this->fp_; }

bool FileHandler::is_open() const { return this->fp_ != nullptr; }
