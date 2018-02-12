# *******************************************************************************
# * Copyright 2018 UT-Battelle, LLC
# * All rights reserved
# * Route Sanitizer, version 0.9
# * 
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# * 
# *     http://www.apache.org/licenses/LICENSE-2.0
# * 
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *
# * For issues, question, and comments, please submit a issue via GitHub.
# *******************************************************************************/
cmake_minimum_required(VERSION 3.0)
project(CV)

# Needed for library and unit-tests.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CV_CURRENT_DIR ${PROJECT_SOURCE_DIR})
# The version number.
set(CV_VERSION_MAJOR 1)
set(CV_VERSION_MINOR 0)
set(CV_VERSION "${CV_VERSION_MAJOR}.${CV_VERSION_MINOR}")

message( STATUS "System: ${CMAKE_SYSTEM_NAME}")

# Make the Connected Vehicle library.
add_subdirectory("cv-lib")

# Build the library test directory.
add_subdirectory("cv-lib-test")

# Build the command line tool.
add_subdirectory("cl-tool")

# Build the electron app.
if (CMAKE_JS_VERSION)
    add_subdirectory("cv-gui-electron")
endif()
