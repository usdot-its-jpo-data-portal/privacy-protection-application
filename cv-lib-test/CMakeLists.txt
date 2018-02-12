# /*******************************************************************************
#  * Copyright 2018 UT-Battelle, LLC
#  * All rights reserved
#  * Route Sanitizer, version 0.9
#  * 
#  * Licensed under the Apache License, Version 2.0 (the "License");
#  * you may not use this file except in compliance with the License.
#  * You may obtain a copy of the License at
#  * 
#  *     http://www.apache.org/licenses/LICENSE-2.0
#  * 
#  * Unless required by applicable law or agreed to in writing, software
#  * distributed under the License is distributed on an "AS IS" BASIS,
#  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  * See the License for the specific language governing permissions and
#  * limitations under the License.
#  *
#  * For issues, question, and comments, please submit a issue via GitHub.
#  *******************************************************************************/
cmake_minimum_required(VERSION 2.6)
set(CVLIB_TEST_SRC "src/tests.cpp")

# Add the library headers.
include_directories(${CVLIB_INCLUDE})

set(CATCH_INCLUDE_DIR "include/catch")
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

# Build the test executable.
add_executable(cvlib_tests ${CVLIB_TEST_SRC})
target_link_libraries(cvlib_tests CVLib Catch)
target_compile_definitions(cvlib_tests PRIVATE _PPM_TESTS)

# Copy the data to the build.
set(UNIT_DATA_DIR $<TARGET_FILE_DIR:cvlib_tests>/unit-test-data)

# Make the base data directory.
add_custom_command(TARGET cvlib_tests PRE_BUILD COMMAND ${CMAKE_COMMAND} 
                   -E make_directory ${UNIT_DATA_DIR})
add_custom_command(TARGET cvlib_tests PRE_BUILD COMMAND ${CMAKE_COMMAND} 
                   -E copy_directory ${PROJECT_SOURCE_DIR}/unit-test-data
                   ${UNIT_DATA_DIR})
