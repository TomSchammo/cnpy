#include "../include/cnpy.hpp"
#include <complex>
#include <cstdlib>
#include <gtest/gtest.h>
#include <iostream>
#include <map>
#include <string>

const int Nx = 128;
const int Ny = 64;
const int Nz = 32;

constexpr auto npy_file = "../test/data/test_load.npy";
constexpr auto npz_file = "../test/data/test_load.npz";

auto get_data() {
  // set random seed so that result is reproducible (for testing)
  srand(0);

  std::vector<std::complex<double>> data(Nx * Ny * Nz);
  for (int i = 0; i < Nx * Ny * Nz; i++)
    data[i] = std::complex<double>(rand(), rand());

  return data;
}

TEST(NpyLoad, Npy) {

  const cnpy::NpyArray arr = cnpy::npy_load(npy_file);

  const auto *const loaded_data = arr.data<double>();

  EXPECT_EQ(loaded_data[0], 1.0f);
  EXPECT_EQ(loaded_data[1], 2.0f);
  EXPECT_EQ(loaded_data[2], 3.0f);
}

TEST(NpySave, Npy) {

  const auto data = get_data();

  // save it to file
  cnpy::npy_save("arr1.npy", &data[0], {Nz, Ny, Nx}, "w");

  // load it into a new array
  cnpy::NpyArray arr = cnpy::npy_load("arr1.npy");
  std::complex<double> *loaded_data = arr.data<std::complex<double>>();

  // make sure the loaded data matches the saved data
  ASSERT_EQ(arr.word_size, sizeof(std::complex<double>));
  ASSERT_TRUE(arr.shape.size() == 3 && arr.shape[0] == Nz &&
              arr.shape[1] == Ny && arr.shape[2] == Nx);
  for (int i = 0; i < Nx * Ny * Nz; i++)
    ASSERT_EQ(data[i], loaded_data[i]);
}

TEST(NpyAppend, Npy) {

  const auto data = get_data();

  cnpy::npy_save("arr1.npy", &data[0], {Nz, Ny, Nx}, "w");
  cnpy::npy_save("arr1.npy", &data[0], {Nz, Ny, Nx}, "a");

  auto expected = data;
  expected.insert(expected.end(), data.begin(), data.end());


  cnpy::NpyArray arr = cnpy::npy_load("arr1.npy");
  std::complex<double> *loaded_data = arr.data<std::complex<double>>();

  ASSERT_EQ(arr.word_size, sizeof(std::complex<double>));
  ASSERT_TRUE(arr.shape.size() == 3 && arr.shape[0] == Nz + Nz &&
              arr.shape[1] == Ny && arr.shape[2] == Nx);
  for (int i = 0; i < Nx * Ny * (Nz + Nz); i++)
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
  double myVar1 = 1.2;
  char myVar2 = 'a';
  cnpy::npz_save("out.npz", "myVar1", &myVar1, {1},
                 "w"); //"w" overwrites any existing file
  cnpy::npz_save("out.npz", "myVar2", &myVar2, {1},
                 "a"); //"a" appends to the file we created above
  cnpy::npz_save("out.npz", "arr1", &data[0], {Nz, Ny, Nx},
                 "a"); //"a" appends to the file we created above

  // load a single var from the npz file
  cnpy::NpyArray arr2 = cnpy::npz_load("out.npz", "arr1");

  // load the entire npz file
  cnpy::npz_t my_npz = cnpy::npz_load("out.npz");

  // check that the loaded myVar1 matches myVar1
  cnpy::NpyArray arr_mv1 = my_npz["myVar1"];
  double *mv1 = arr_mv1.data<double>();
  ASSERT_TRUE(arr_mv1.shape.size() == 1 && arr_mv1.shape[0] == 1);
  ASSERT_EQ(mv1[0], myVar1);
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
