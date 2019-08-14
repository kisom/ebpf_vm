## configuration
cmake_minimum_required(VERSION 3.0)
project(libebpf VERSION 0.0.1 LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
add_compile_options(-Werror -Wno-unused-function -Wall -g -O0)

if (APPLE)
else()
# can't static link on darwin
add_compile_options(-static)
add_link_options(-static)
endif()


add_library(${PROJECT_NAME} src/ebpf.c include/ebpf.h)
add_executable(ebpf_vm src/vm.c)
target_link_libraries(ebpf_vm libebpf)

add_executable(ebpf_isa_test src/ebpf_test.cc)
target_link_libraries(ebpf_isa_test libebpf)
add_test(ebpf_isa_test ebpf_isa_test)
