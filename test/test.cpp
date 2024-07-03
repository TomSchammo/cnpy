#include "../include/cnpy.hpp"
#include <complex>
#include <cstdlib>
#include <gtest/gtest.h>
#include <iostream>
#include <map>
#include <random>
#include <string>

constexpr int nx = 128;
constexpr int ny = 64;
constexpr int nz = 32;

constexpr auto npy_file = "../test/data/test_load.npy";
constexpr auto npz_file = "../test/data/test_load.npz";

auto get_data() {
  // set random seed so that result is reproducible (for testing)
  std::mt19937 generator(0);
  std::uniform_real_distribution<double> distribution(0.0, 1.0);

  std::vector<std::complex<double>> data(nx * ny * nz);
  for (int i = 0; i < nx * ny * nz; i++)
    data[i] = std::complex<double>(distribution(generator), distribution(generator));

  return data;
}

TEST(NpyLoad, Npy) {

  const cnpy::npy_array arr = cnpy::npy_load(npy_file);

  const auto *const loaded_data = arr.data<double>();

  EXPECT_EQ(loaded_data[0], 1.0f);
  EXPECT_EQ(loaded_data[1], 2.0f);
  EXPECT_EQ(loaded_data[2], 3.0f);
}

TEST(NpySave, Npy) {

  const auto data = get_data();

  // save it to file
  cnpy::npy_save("arr1.npy", &data[0], {nz, ny, nx}, "w");

  // load it into a new array
  cnpy::npy_array arr = cnpy::npy_load("arr1.npy");
  auto *loaded_data = arr.data<std::complex<double>>();

  // make sure the loaded data matches the saved data
  ASSERT_EQ(arr.word_size, sizeof(std::complex<double>));
  ASSERT_TRUE(arr.shape.size() == 3 && arr.shape[0] == nz &&
              arr.shape[1] == ny && arr.shape[2] == nx);
  for (int i = 0; i < nx * ny * nz; i++)
    ASSERT_EQ(data[i], loaded_data[i]);
}

TEST(NpyAppend, Npy) {

  const auto data = get_data();

  cnpy::npy_save("arr1.npy", &data[0], {nz, ny, nx}, "w");
  cnpy::npy_save("arr1.npy", &data[0], {nz, ny, nx}, "a");

  auto expected = data;
  expected.insert(expected.end(), data.begin(), data.end());

  cnpy::npy_array arr = cnpy::npy_load("arr1.npy");
  auto *loaded_data = arr.data<std::complex<double>>();

  ASSERT_EQ(arr.word_size, sizeof(std::complex<double>));
  ASSERT_TRUE(arr.shape.size() == 3 && arr.shape[0] == nz + nz &&
              arr.shape[1] == ny && arr.shape[2] == nx);
  for (int i = 0; i < nx * ny * (nz + nz); i++)
    ASSERT_EQ(expected[i], loaded_data[i]);
}

TEST(NpzLoadAll, Npz) {

  cnpy::npz_t npz = cnpy::npz_load(npz_file);

  {
    const auto f = npz["f"].as_vec<double>();
    EXPECT_EQ(f.size(), 3);
    EXPECT_EQ(f[0], .1);
    EXPECT_EQ(f[1], .2);
    EXPECT_EQ(f[2], .3);
  }

  {
    const auto s = npz["s"].as_vec<long long>();
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s[0], 1);
    EXPECT_EQ(s[1], 2);
    EXPECT_EQ(s[2], 3);
  }

  {
    const auto t = npz["t"].as_vec<char>();
    EXPECT_EQ(t.size(), 1);
    EXPECT_EQ(t[0], 'a');
  }
}

TEST(NpzLoadSingleFirst, Npz) {
  const auto f = cnpy::npz_load(npz_file, "f").as_vec<double>();
  EXPECT_EQ(f.size(), 3);
  EXPECT_EQ(f[0], .1);
  EXPECT_EQ(f[1], .2);
  EXPECT_EQ(f[2], .3);
}

// TODO: It looks like it's only possible to load the first one by name(?)
TEST(NpzLoadSingleSecond, Npz) {
  const auto s = cnpy::npz_load(npz_file, "s").as_vec<long long>();
  EXPECT_EQ(s.size(), 3);
  EXPECT_EQ(s[0], 1);
  EXPECT_EQ(s[1], 2);
  EXPECT_EQ(s[2], 3);
}

// TODO: It looks like it's only possible to load the first one by name(?)
TEST(NpzLoadSingleThird, Npz) {
  const auto t = cnpy::npz_load(npz_file, "t").as_vec<char>();
  EXPECT_EQ(t.size(), 1);
  EXPECT_EQ(t[0], 'a');
}

TEST(NpzSave, Npz) {

  const auto data = get_data();
  // now write to an npz file
  // non-array variables are treated as 1D arrays with 1 element
  double my_var1 = 1.2;
  char my_var2 = 'a';
  cnpy::npz_save("out.npz", "my_var1", &my_var1, {1},
                 "w"); //"w" overwrites any existing file
  cnpy::npz_save("out.npz", "my_var2", &my_var2, {1},
                 "a"); //"a" appends to the file we created above
  cnpy::npz_save("out.npz", "arr1", &data[0], {nz, ny, nx},
                 "a"); //"a" appends to the file we created above

  // load a single var from the npz file
  cnpy::npy_array arr2 = cnpy::npz_load("out.npz", "arr1");

  // load the entire npz file
  cnpy::npz_t my_npz = cnpy::npz_load("out.npz");

  // check that the loaded myVar1 matches myVar1
  cnpy::npy_array arr_mv1 = my_npz["my_var1"];
  auto *mv1 = arr_mv1.data<double>();
  ASSERT_TRUE(arr_mv1.shape.size() == 1 && arr_mv1.shape[0] == 1);
  ASSERT_EQ(mv1[0], my_var1);
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
