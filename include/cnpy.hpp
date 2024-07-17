// Copyright (C) 2011  Carl Rogers
// Released under MIT License
// license available in LICENSE file, or at
// http://www.opensource.org/licenses/mit-license.php

#ifndef LIBCNPY_HPP
#define LIBCNPY_HPP

#include <bit>
#include <cassert>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <zlib.h>

namespace cnpy {

struct npy_array {
  constexpr npy_array(const std::vector<size_t> &shape, const size_t word_size,
                      const bool fortran_order)
      : shape_(shape), word_size_(word_size), fortran_order_(fortran_order),
        num_vals_(1) {
    for (const unsigned long i : shape_) {
      num_vals_ *= i;
    }
    data_holder_ = std::make_shared<std::vector<char>>(num_vals_ * word_size_);
  }

  constexpr npy_array()
      : shape_(0), word_size_(0), fortran_order_(false), num_vals_(0) {}

  // TODO: can this be noexcept (because of reinterpret_cast)?
  template <typename T> constexpr T *data() {
    return reinterpret_cast<T *>(data_holder_->data());
  }

  // TODO: can this be noexcept (because of reinterpret_cast)?
  template <typename T> constexpr T *data() const {
    return reinterpret_cast<T *>(data_holder_->data());
  }

  template <typename T> std::vector<T> as_vec() const {
    const T *p = data<T>();
    return std::vector<T>(p, p + num_vals_);
  }

  [[nodiscard]] constexpr size_t num_bytes() const noexcept {
    return data_holder_->size();
  }
  [[nodiscard]] constexpr size_t num_vals() const noexcept { return num_vals_; }
  [[nodiscard]] constexpr size_t word_size() const noexcept {
    return word_size_;
  }
  [[nodiscard]] constexpr bool fortran_order() const noexcept {
    return fortran_order_;
  }
  [[nodiscard]] constexpr std::vector<size_t> shape() const noexcept {
    return shape_;
  }
  [[nodiscard]] constexpr std::vector<size_t> shape() noexcept {
    return shape_;
  }

private:
  std::shared_ptr<std::vector<char>> data_holder_;
  std::vector<size_t> shape_;
  size_t word_size_;
  bool fortran_order_;
  size_t num_vals_;
};

using npz_t = std::map<std::string, npy_array>;

constexpr char get_endianness() {
  if constexpr (std::endian::native == std::endian::little)
    return '<';
  else
    return '>';
}

template <typename T> consteval char map_type() {
  if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double> ||
                std::is_same_v<T, long double>) {
    return 'f';
  }

  if constexpr (std::is_same_v<T, int> || std::is_same_v<T, char> ||
                std::is_same_v<T, short> || std::is_same_v<T, long> ||
                std::is_same_v<T, long long>) {
    return 'i';
  }

  if constexpr (std::is_same_v<T, unsigned char> ||
                std::is_same_v<T, unsigned short> ||
                std::is_same_v<T, unsigned long> ||
                std::is_same_v<T, unsigned long long> ||
                std::is_same_v<T, unsigned int>) {
    return 'u';
  }

  if constexpr (std::is_same_v<T, bool>) {
    return 'b';
  }

  if constexpr (std::is_same_v<T, std::complex<float>> ||
                std::is_same_v<T, std::complex<double>> ||
                std::is_same_v<T, std::complex<long double>>) {
    return 'c';
  }

  return '?';
}

template <typename T>
std::vector<char> create_npy_header(const std::vector<size_t> &shape);
void parse_npy_header(FILE *fp, size_t &word_size, std::vector<size_t> &shape,
                      bool &fortran_order);
void parse_npy_header(unsigned char *buffer, size_t &word_size,
                      std::vector<size_t> &shape, bool &fortran_order);
void parse_zip_footer(FILE *fp, uint16_t &nrecs, size_t &global_header_size,
                      size_t &global_header_offset);
npz_t npz_load(const std::string &fname);
npy_array npz_load(const std::string &fname, const std::string &varname);
npy_array npy_load(const std::string &fname);

template <typename T>
constexpr std::vector<char> &operator+=(std::vector<char> &lhs, const T rhs) {
  // write in little endian
  for (size_t byte = 0; byte < sizeof(T); byte++) {
    char val = *(reinterpret_cast<const char *>(&rhs) + byte);
    lhs.push_back(val);
  }
  return lhs;
}

// TODO: rhs cannot be a const& for some reason???
template <>
constexpr std::vector<char> &cnpy::operator+=(std::vector<char> &lhs,
                                              const std::string rhs) { // NOLINT
  lhs.insert(lhs.end(), rhs.begin(), rhs.end());
  return lhs;
}

template <>
constexpr std::vector<char> &cnpy::operator+=(std::vector<char> &lhs,
                                              const char *rhs) {
  // write in little endian
  const size_t len = std::string_view(rhs).size();
  lhs.reserve(len);
  for (size_t byte = 0; byte < len; byte++) {
    lhs.push_back(rhs[byte]);
  }
  return lhs;
}

template <typename T>
void npy_save(const std::string_view fname, const T *data,
              const std::vector<size_t> &shape,
              const std::string_view mode = "w") {
  FILE *fp = nullptr;
  std::vector<size_t>
      true_data_shape; // if appending, the shape of existing + new data

  if (mode == "a") {
    fp = fopen(fname.data(), "r+b");
  }

  if (fp) {
    // file exists. we need to append to it. read the header, modify the array
    // size
    size_t word_size;
    bool fortran_order;
    parse_npy_header(fp, word_size, true_data_shape, fortran_order);
    assert(!fortran_order);

    if (word_size != sizeof(T)) {
      std::cout << "libnpy error: " << fname << " has word size " << word_size
                << " but npy_save appending data sized " << sizeof(T) << "\n";
    }
    if (true_data_shape.size() != shape.size()) {
      std::cout << "libnpy error: npy_save attempting to append misdimensioned "
                   "data to "
                << fname << "\n";
    }

    for (size_t i = 1; i < shape.size(); i++) {
      if (shape[i] != true_data_shape[i]) {
        std::cout
            << "libnpy error: npy_save attempting to append misshaped data to "
            << fname << "\n";
      }
    }
    true_data_shape[0] += shape[0];
  } else {
    fp = fopen(fname.data(), "wb");
    true_data_shape = shape;
  }

  std::vector<char> header = create_npy_header<T>(true_data_shape);
  const size_t nels =
      std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());

  fseek(fp, 0, SEEK_SET);
  fwrite(header.data(), sizeof(char), header.size(), fp);
  fseek(fp, 0, SEEK_END);
  fwrite(data, sizeof(T), nels, fp);
  fclose(fp);
}

template <typename T>
void npz_save(const std::string_view zipname, std::string fname, const T *data,
              const std::vector<size_t> &shape,
              const std::string_view mode = "w") {
  // first, append a .npy to the fname
  fname += ".npy";

  // now, on with the show
  FILE *fp = nullptr;
  uint16_t nrecs = 0;
  size_t global_header_offset = 0;
  std::vector<char> global_header;

  if (mode == "a") {
    fp = fopen(zipname.data(), "r+b");
  }

  if (fp) {
    // zip file exists. we need to add a new npy file to it.
    // first read the footer. this gives us the offset and size of the global
    // header then read and store the global header. below, we will write the
    // the new data at the start of the global header then append the global
    // header and footer below it
    size_t global_header_size;
    parse_zip_footer(fp, nrecs, global_header_size, global_header_offset);
    fseek(fp, global_header_offset, SEEK_SET);
    global_header.resize(global_header_size);
    size_t res =
        fread(global_header.data(), sizeof(char), global_header_size, fp);
    if (res != global_header_size) {
      throw std::runtime_error(
          "npz_save: header read error while adding to existing zip");
    }
    fseek(fp, global_header_offset, SEEK_SET);
  } else {
    fp = fopen(zipname.data(), "wb");
  }

  std::vector<char> npy_header = create_npy_header<T>(shape);

  size_t nels =
      std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());
  size_t nbytes = nels * sizeof(T) + npy_header.size();

  // get the CRC of the data to be added
  uint32_t crc = crc32(0L, reinterpret_cast<uint8_t *>(npy_header.data()),
                       npy_header.size());
  crc = crc32(crc, reinterpret_cast<const uint8_t *>(data), nels * sizeof(T));

  // clang-format off
  // build the local header
  std::vector<char> local_header;
  local_header += "PK";                   // first part of sig
  local_header += static_cast<uint16_t>(0x0403);       // second part of sig
  local_header += static_cast<uint16_t>(20);           // min version to extract
  local_header += static_cast<uint16_t>(0);            // general purpose bit flag
  local_header += static_cast<uint16_t>(0);            // compression method
  local_header += static_cast<uint16_t>(0);            // file last mod time
  local_header += static_cast<uint16_t>(0);            // file last mod date
  local_header += static_cast<uint32_t>(crc);          // crc
  local_header += static_cast<uint32_t>(nbytes);       // compressed size
  local_header += static_cast<uint32_t>(nbytes);       // uncompressed size
  local_header += static_cast<uint16_t>(fname.size()); // fname length
  local_header += static_cast<uint16_t>(0);            // extra field length
  local_header += fname;

  // build global header
  global_header += "PK";             // first part of sig
  global_header += static_cast<uint16_t>(0x0201); // second part of sig
  global_header += static_cast<uint16_t>(20);     // version made by
  global_header.insert(global_header.end(), local_header.begin() + 4,
                       local_header.begin() + 30);
  global_header += static_cast<uint16_t>(0); // file comment length
  global_header += static_cast<uint16_t>(0); // disk number where file starts
  global_header += static_cast<uint16_t>(0); // internal file attributes
  global_header += static_cast<uint32_t>(0); // external file attributes
  global_header += static_cast<uint32_t>(
      global_header_offset); // relative offset of local file header, since it
                            // begins where the global header used to begin
  global_header += fname;

  // build footer
  std::vector<char> footer;
  footer += "PK";                           // first part of sig
  footer += static_cast<uint16_t>(0x0605);               // second part of sig
  footer += static_cast<uint16_t>(0);                    // number of this disk
  footer += static_cast<uint16_t>(0);                    // disk where footer starts
  footer += static_cast<uint16_t>(nrecs + 1);          // number of records on this disk
  footer += static_cast<uint16_t>(nrecs + 1);          // total number of records
  footer += static_cast<uint32_t>(global_header.size()); // nbytes of global headers
  footer += static_cast<uint32_t>(
      global_header_offset + nbytes +
      local_header.size()); // offset of start of global
                                             // headers, since global header now
                                             // starts after newly written array
  footer += static_cast<uint16_t>(0);                     // zip file comment length
  // clang-format on

  // write everything
  fwrite(local_header.data(), sizeof(char), local_header.size(), fp);
  fwrite(npy_header.data(), sizeof(char), npy_header.size(), fp);
  fwrite(data, sizeof(T), nels, fp);
  fwrite(global_header.data(), sizeof(char), global_header.size(), fp);
  fwrite(footer.data(), sizeof(char), footer.size(), fp);
  fclose(fp);
}

template <typename T>
void npy_save(const std::string_view fname, const std::vector<T> data,
              const std::string_view mode = "w") {
  std::vector<size_t> shape;
  shape.push_back(data.size());
  npy_save(fname, data.data(), shape, mode);
}

template <typename T>
void npz_save(const std::string_view zipname, const std::string_view fname,
              const std::vector<T> data, const std::string_view mode = "w") {
  std::vector<size_t> shape;
  shape.push_back(data.size());
  npz_save(zipname, fname, data.data(), shape, mode);
}

template <typename T>
std::vector<char> create_npy_header(const std::vector<size_t> &shape) {

  std::vector<char> dict;
  dict += "{'descr': '";
  dict += get_endianness();
  dict += map_type<T>();
  dict += std::to_string(sizeof(T));
  dict += "', 'fortran_order': False, 'shape': (";
  dict += std::to_string(shape[0]);
  for (size_t i = 1; i < shape.size(); i++) {
    dict += ", ";
    dict += std::to_string(shape[i]);
  }
  if (shape.size() == 1) {
    dict += ",";
  }
  dict += "), }";
  // pad with spaces so that preamble+dict is modulo 16 bytes. preamble is 10
  // bytes. dict needs to end with \n
  const int remainder = 16 - (10 + dict.size()) % 16;
  dict.insert(dict.end(), remainder, ' ');
  dict.back() = '\n';

  std::vector<char> header;
  header += static_cast<char>(0x93);
  header += "NUMPY";
  header += static_cast<char>(0x01); // major version of numpy format
  header += static_cast<char>(0x00); // minor version of numpy format
  header += static_cast<uint16_t>(dict.size());
  header.insert(header.end(), dict.begin(), dict.end());

  return header;
}

} // namespace cnpy

#endif
