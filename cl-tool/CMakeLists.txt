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
project(CVTOOL)

set(CVTOOL_CURRENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Find the threading library.
find_package(Threads)

# Add local headers.
include_directories("${CVTOOL_CURRENT_DIR}/include")

# Add the library headers.
include_directories(${CVLIB_INCLUDE})

# Add the exectable.
set(CVTOOL_TARGET "cv_di")
add_executable(${CVTOOL_TARGET} 
               "${CVTOOL_CURRENT_DIR}/src/shrp2type.cpp" 
               "${CVTOOL_CURRENT_DIR}/src/cv_di.cpp" 
               "${CVTOOL_CURRENT_DIR}/src/tool.cpp"
               "${CVTOOL_CURRENT_DIR}/src/di_multi.cpp"
               "${CVTOOL_CURRENT_DIR}/src/config.cpp")
# Link with the library.
target_link_libraries(${CVTOOL_TARGET} ${CMAKE_THREAD_LIBS_INIT} CVLib)
