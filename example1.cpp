#include"cnpy.h"
#include<complex>
#include<cstdlib>
#include<iostream>
#include<map>
#include<string>
#include <gtest/gtest.h>

const int Nx = 128;
const int Ny = 64;
const int Nz = 32;

auto get_data() {
  // set random seed so that result is reproducible (for testing)
  srand(0);

  std::vector<std::complex<double>> data(Nx * Ny * Nz);
  for (int i = 0; i < Nx * Ny * Nz; i++)
    data[i] = std::complex<double>(rand(), rand());

   return data;
}


TEST(NpySaveLoad, Npy) {

const auto data = get_data();

  // save it to file
  cnpy::npy_save("arr1.npy", &data[0], {Nz, Ny, Nx}, "w");

  // load it into a new array
  cnpy::NpyArray arr = cnpy::npy_load("arr1.npy");
  std::complex<double> *loaded_data = arr.data<std::complex<double>>();

  // make sure the loaded data matches the saved data
  ASSERT_EQ(arr.word_size, sizeof(std::complex<double>));
  ASSERT_TRUE(arr.shape.size() == 3 && arr.shape[0] == Nz && arr.shape[1] == Ny &&
         arr.shape[2] == Nx);
  for (int i = 0; i < Nx * Ny * Nz; i++)
    ASSERT_EQ(data[i], loaded_data[i]);
}

TEST(NpyAppend, Npy) {

const auto data = get_data();

  // save it to file
  cnpy::npy_save("arr1.npy", &data[0], {Nz, Ny, Nx}, "w");

  // append the same data to file
  // npy array on file now has shape (Nz+Nz,Ny,Nx)
  cnpy::npy_save("arr1.npy", &data[0], {Nz, Ny, Nx}, "a");
}

TEST(NpzSaveLoad, Npz) {

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
