cmake_minimum_required(VERSION 2.8)

file(GLOB HEADERS "*.h")

find_package(OpenMP REQUIRED)

# Enable OpenMP in UNIX.
# Enable multicore builds in MSVC.
if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
else(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif(UNIX)

# Library targets:
#   evasion_core - shared objects for evasion
project(evasion_core)
set(SRCS
    "evasion_core.cpp")
add_library(evasion_core STATIC ${SRCS} ${HEADERS})

# Executable targets:
#   evasion - the main solution
#   evasion_gtest - all tests

project(evasion)
set(SRCS
    "evasion.cpp")
add_executable(evasion ${SRCS} ${HEADERS})
target_link_libraries(evasion evasion_core)
if(WIN32)
  set_target_properties(evasion PROPERTIES
                        COMPILE_FLAGS ${OpenMP_CXX_FLAGS})
endif(WIN32)

if(HPS_GTEST_ENABLED)
  project(evasion_gtest)
  set(SRCS
      "evasion_gtest.cpp")
  include_directories(${GTEST_INCLUDE_DIRS})
  add_executable(evasion_gtest ${SRCS} ${HEADERS})
  target_link_libraries(evasion_gtest evasion_core gtest)
  if(WIN32)
    set_target_properties(evasion_gtest PROPERTIES
                          COMPILE_FLAGS ${OpenMP_CXX_FLAGS})
  endif(WIN32)
  add_test(evasion_gtest evasion_gtest)
  set(COPY_FILES "evasion_vis.py")
  file(COPY ${COPY_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif(HPS_GTEST_ENABLED)

project(evasion_all)
