cmake_minimum_required(VERSION 3.12)
project(entropy_coder C)

set(CMAKE_C_STANDARD 11)
include_directories(include)
set(SOURCES src/ec_decoder.c src/ec_encoder.c src/ec_utils.c)

add_library(entropy_coder ${SOURCES})