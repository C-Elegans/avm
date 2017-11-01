cmake_minimum_required(VERSION 3.0)
project(AVM)
set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
  src/asprintf.c
  src/avm.c
  src/avm_debug.c
  src/avm_eval.c
  src/avm_parse.c
  src/avm_stringify.c
  src/avm_util.c
)

include_directories(src)

add_executable(avm ${SOURCE_FILES})
target_compile_definitions(avm PRIVATE AVM_EXECUTABLE)

add_library(avm_dynamic SHARED ${SOURCE_FILES})
set_target_properties(avm_dynamic PROPERTIES OUTPUT_NAME avm)
