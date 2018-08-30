# Copyright 2018 UT-Battelle, LLC
# All rights reserved
# Route Sanitizer, version 0.9
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# For issues, question, and comments, please submit a issue via GitHub.
cmake_minimum_required(VERSION 3.7)
project(HMM_MM)

file(GLOB CPP_FILES src/*.cpp)
add_library(${PROJECT_NAME} STATIC ${CPP_FILES})
target_link_libraries(${PROJECT_NAME} GEO GEO_DATA)
include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/include" ${GEO_INCLUDE_DIRS} ${GEO_DATA_INCLUDE_DIRS})
set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
