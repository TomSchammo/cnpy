// Copyright (C) 2011  Carl Rogers
// Released under MIT License
// license available in LICENSE file, or at
// http://www.opensource.org/licenses/mit-license.php

#include "../include/cnpy/cnpy.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <regex>
#include <stdexcept>

void cnpy::parse_npy_header(unsigned char *buffer, size_t &word_size,
                            std::vector<size_t> &shape, bool &fortran_order) {
  // std::string magic_string(buffer,6);
  uint8_t major_version = *reinterpret_cast<uint8_t *>(buffer + 6);
  uint8_t minor_version = *reinterpret_cast<uint8_t *>(buffer + 7);
  const uint16_t header_len = *reinterpret_cast<uint16_t *>(buffer + 8);
  std::string_view header(reinterpret_cast<char *>(buffer + 9), header_len);

  // fortran order
  const size_t fortran_order_start = header.find("fortran_order") + 16;
  fortran_order = header.substr(fortran_order_start, 4) == "True";

  // shape
  const size_t shape_start = header.find('(');
  const size_t shape_end = header.find(')');

  const std::regex num_regex("[0-9][0-9]*");
  std::smatch sm;
  shape.clear();

  auto str_shape =
      std::string(header.substr(shape_start + 1, shape_end - shape_start - 1));
  while (std::regex_search(str_shape, sm, num_regex)) {
    shape.push_back(std::stoi(sm[0].str()));
    str_shape = sm.suffix().str();
  }

  // endian, word size, data type
  // byte order code | stands for not applicable.
  // not sure when this applies except for byte array
  const size_t endian_offset = header.find("descr") + 9;
  const bool little_endian =
      header[endian_offset] == '<' || header[endian_offset] == '|';
  assert(little_endian);

  // char type = header[loc1+1];
  // assert(type == map_type(T));

  std::string_view str_ws = header.substr(endian_offset + 2);
  const size_t word_size_offset = str_ws.find('\'');
  word_size = atoi(str_ws.substr(0, word_size_offset).data());
}

void cnpy::parse_npy_header(FILE *fp, size_t &word_size,
                            std::vector<size_t> &shape, bool &fortran_order) {
  std::array<char, 256> buffer;
  if (const size_t res = fread(buffer.data(), sizeof(char), 11, fp);
      res != 11) {
    throw std::runtime_error("parse_npy_header: failed fread");
  }
  std::string header = fgets(buffer.data(), 256, fp);
  assert(header[header.size() - 1] == '\n');

  // fortran order
  size_t fortran_order_start = header.find("fortran_order");
  // TODO: can be simplified in C++23 with std::string::contains
  if (fortran_order_start == std::string::npos) {
    throw std::runtime_error(
        "parse_npy_header: failed to find header keyword: 'fortran_order'");
  }
  fortran_order_start += 16;
  fortran_order = header.substr(fortran_order_start, 4) == "True";

  // shape
  const size_t shape_start = header.find('(');
  const size_t shape_end = header.find(')');
  if (shape_start == std::string::npos || shape_end == std::string::npos) {
    throw std::runtime_error(
        "parse_npy_header: failed to find header keyword: '(' or ')'");
  }

  const std::regex num_regex("[0-9][0-9]*");
  std::smatch sm;
  shape.clear();

  std::string str_shape =
      header.substr(shape_start + 1, shape_end - shape_start - 1);
  while (std::regex_search(str_shape, sm, num_regex)) {
    shape.push_back(std::stoi(sm[0].str()));
    str_shape = sm.suffix().str();
  }

  // endian, word size, data type
  // byte order code | stands for not applicable.
  // not sure when this applies except for byte array
  size_t endian_offset = header.find("descr");
  if (endian_offset == std::string::npos) {
    throw std::runtime_error(
        "parse_npy_header: failed to find header keyword: 'descr'");
  }
  endian_offset += 9;
  const bool little_endian =
      header[endian_offset] == '<' || header[endian_offset] == '|';
  assert(little_endian);

  // char type = header[loc1+1];
  // assert(type == map_type(T));

  std::string str_ws = header.substr(endian_offset + 2);
  const size_t word_site_offset = str_ws.find('\'');
  word_size = atoi(str_ws.substr(0, word_site_offset).c_str());
}

void cnpy::parse_zip_footer(FILE *fp, uint16_t &nrecs,
                            size_t &global_header_size,
                            size_t &global_header_offset) {
  std::vector<char> footer(22);
  fseek(fp, -22, SEEK_END);
  if (const size_t res = fread(footer.data(), sizeof(char), 22, fp);
      res != 22) {
    throw std::runtime_error("parse_zip_footer: failed fread");
  }

  const uint16_t disk_no = *reinterpret_cast<uint16_t *>(&footer[4]);
  const uint16_t disk_start = *reinterpret_cast<uint16_t *>(&footer[6]);
  const uint16_t nrecs_on_disk = *reinterpret_cast<uint16_t *>(&footer[8]);
  nrecs = *reinterpret_cast<uint16_t *>(&footer[10]);
  global_header_size = *reinterpret_cast<uint32_t *>(&footer[12]);
  global_header_offset = *reinterpret_cast<uint32_t *>(&footer[16]);
  const uint16_t comment_len = *reinterpret_cast<uint16_t *>(&footer[20]);

  assert(disk_no == 0);
  assert(disk_start == 0);
  assert(nrecs_on_disk == nrecs);
  assert(comment_len == 0);
}

cnpy::npy_array load_the_npy_file(FILE *fp) {
  std::vector<size_t> shape;
  size_t word_size;
  bool fortran_order;
  cnpy::parse_npy_header(fp, word_size, shape, fortran_order);

  cnpy::npy_array arr(shape, word_size, fortran_order);
  if (const size_t nread = fread(arr.data<char>(), 1, arr.num_bytes(), fp);
      nread != arr.num_bytes()) {
    throw std::runtime_error("load_the_npy_file: failed fread");
  }
  return arr;
}

cnpy::npy_array load_the_npz_array(FILE *fp, const uint32_t compr_bytes,
                                   const uint32_t uncompr_bytes) {

  std::vector<unsigned char> buffer_compr(compr_bytes);
  std::vector<unsigned char> buffer_uncompr(uncompr_bytes);
  if (const size_t nread = fread(buffer_compr.data(), 1, compr_bytes, fp);
      nread != compr_bytes) {
    throw std::runtime_error("load_the_npy_file: failed fread");
  }

  z_stream d_stream;

  d_stream.zalloc = nullptr;
  d_stream.zfree = nullptr;
  d_stream.opaque = nullptr;
  d_stream.avail_in = 0;
  d_stream.next_in = nullptr;
  int err = inflateInit2(&d_stream, -MAX_WBITS);

  d_stream.avail_in = compr_bytes;
  d_stream.next_in = buffer_compr.data();
  d_stream.avail_out = uncompr_bytes;
  d_stream.next_out = buffer_uncompr.data();

  err = inflate(&d_stream, Z_FINISH);
  err = inflateEnd(&d_stream);

  std::vector<size_t> shape;
  size_t word_size;
  bool fortran_order;
  cnpy::parse_npy_header(buffer_uncompr.data(), word_size, shape,
                         fortran_order);

  cnpy::npy_array array(shape, word_size, fortran_order);

  const size_t offset = uncompr_bytes - array.num_bytes();
  memcpy(array.data<unsigned char>(), buffer_uncompr.data() + offset,
         array.num_bytes());

  return array;
}

cnpy::npz_t cnpy::npz_load(const std::string &fname) {
  FILE *fp = fopen(fname.c_str(), "rb");

  if (!fp) {
    throw std::runtime_error("npz_load: Error! Unable to open file " + fname +
                             "!");
  }

  cnpy::npz_t arrays;

  while (true) {
    std::vector<char> local_header(30);
    if (const size_t headerres =
            fread(local_header.data(), sizeof(char), 30, fp);
        headerres != 30) {
      throw std::runtime_error("npz_load: failed fread");
    }

    // if we've reached the global header, stop reading
    if (local_header[2] != 0x03 || local_header[3] != 0x04) {
      break;
    }

    // read in the variable name
    const uint16_t name_len = *reinterpret_cast<uint16_t *>(&local_header[26]);
    std::string varname(name_len, ' ');
    if (const size_t vname_res =
            fread(varname.data(), sizeof(char), name_len, fp);
        vname_res != name_len) {
      throw std::runtime_error("npz_load: failed fread");
    }

    // erase the lagging .npy
    varname.erase(varname.end() - 4, varname.end());

    // read in the extra field
    if (const uint16_t extra_field_len =
            *reinterpret_cast<uint16_t *>(&local_header[28]);
        extra_field_len > 0) {
      std::vector<char> buff(extra_field_len);
      if (const size_t efield_res =
              fread(buff.data(), sizeof(char), extra_field_len, fp);
          efield_res != extra_field_len) {
        throw std::runtime_error("npz_load: failed fread");
      }
    }

    const uint16_t compr_method =
        *reinterpret_cast<uint16_t *>(local_header.data() + 8);
    const uint32_t compr_bytes =
        *reinterpret_cast<uint32_t *>(local_header.data() + 18);
    const uint32_t uncompr_bytes =
        *reinterpret_cast<uint32_t *>(local_header.data() + 22);

    if (compr_method == 0) {
      arrays[varname] = load_the_npy_file(fp);
    } else {
      arrays[varname] = load_the_npz_array(fp, compr_bytes, uncompr_bytes);
    }
  }

  fclose(fp);
  return arrays;
}

cnpy::npy_array cnpy::npz_load(const std::string &fname,
                               const std::string &varname) {
  FILE *fp = fopen(fname.c_str(), "rb");

  if (!fp) {
    throw std::runtime_error("npz_load: Unable to open file " + fname);
  }

  while (true) {
    std::vector<char> local_header(30);
    if (const size_t header_res =
            fread(local_header.data(), sizeof(char), 30, fp);
        header_res != 30) {
      throw std::runtime_error("npz_load: failed fread");
    }

    // if we've reached the global header, stop reading
    if (local_header[2] != 0x03 || local_header[3] != 0x04) {
      break;
    }

    // read in the variable name
    const uint16_t name_len = *reinterpret_cast<uint16_t *>(&local_header[26]);
    std::string vname(name_len, ' ');
    if (const size_t vname_res =
            fread(vname.data(), sizeof(char), name_len, fp);
        vname_res != name_len) {
      throw std::runtime_error("npz_load: failed fread");
    }
    vname.erase(vname.end() - 4, vname.end()); // erase the lagging .npy

    // read in the extra field
    const uint16_t extra_field_len =
        *reinterpret_cast<uint16_t *>(&local_header[28]);
    fseek(fp, extra_field_len, SEEK_CUR); // skip past the extra field

    const uint16_t compr_method =
        *reinterpret_cast<uint16_t *>(local_header.data() + 8);
    const uint32_t compr_bytes =
        *reinterpret_cast<uint32_t *>(local_header.data() + 18);
    const uint32_t uncompr_bytes =
        *reinterpret_cast<uint32_t *>(local_header.data() + 22);

    if (vname == varname) {
      npy_array array = compr_method == 0 ? load_the_npy_file(fp)
                                          : load_the_npz_array(fp, compr_bytes,
                                                               uncompr_bytes);
      fclose(fp);
      return array;
    } else {
      // skip past the data
      const uint32_t size = *reinterpret_cast<uint32_t *>(&local_header[22]);
      fseek(fp, size, SEEK_CUR);
    }
  }

  fclose(fp);

  // if we get here, we haven't found the variable in the file
  throw std::runtime_error("npz_load: Variable name " + varname +
                           " not found in " + fname);
}

cnpy::npy_array cnpy::npy_load(const std::string &fname) {

  FILE *fp = fopen(fname.c_str(), "rb");

  if (!fp) {
    throw std::runtime_error("npy_load: Unable to open file " + fname);
  }

  npy_array arr = load_the_npy_file(fp);

  fclose(fp);
  return arr;
}
