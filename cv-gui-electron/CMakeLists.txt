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
project(CVGUI)
set(CVGUI_CURRENT_DIR ${PROJECT_SOURCE_DIR})
set(CVGUI_TARGET cvdi_nm)

# Add the library headers.
include_directories(${CVLIB_INCLUDE})
include_directories(${CVLIB_INCLUDE_SUB})

# Add the GUI specific headers.
include_directories("cpp/include")

# Set up for the node module build.
set(ELECTRON_APP_DIR "${CVGUI_CURRENT_DIR}")

# Add the module executable.
add_library(${CVGUI_TARGET} SHARED "cpp/src/${CVGUI_TARGET}.cc")

# Gives our library file a .node extension without any "lib" prefix
set_target_properties(${CVGUI_TARGET} PROPERTIES PREFIX "" SUFFIX ".node")

# Add the Connected Vehicle Library to the executable.
target_link_libraries(${CVGUI_TARGET} CVLib)

# Essential include files to build a node module.
target_include_directories(${CVGUI_TARGET} PRIVATE ${CMAKE_JS_INC})

# Essential library files to link to a node module.
target_link_libraries(${CVGUI_TARGET} ${CMAKE_JS_LIB})

# After the build, create the Electron App.
set(ELECTRON_APP_CVGUI_TARGET_DIR 
    $<TARGET_FILE_DIR:${CVGUI_TARGET}>/electron-app)

# Make the base chrome-app directory.
add_custom_command(TARGET ${CVGUI_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} 
                   -E make_directory ${ELECTRON_APP_CVGUI_TARGET_DIR})

# Copy the app files.
add_custom_command(TARGET ${CVGUI_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} 
                   -E copy_directory ${ELECTRON_APP_DIR} 
                   ${ELECTRON_APP_CVGUI_TARGET_DIR})

set(ELECTRON_APP_MODULE_DIR "${ELECTRON_APP_CVGUI_TARGET_DIR}/modules")

# Make the module direcotry in the app directory.
add_custom_command(TARGET ${CVGUI_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} 
                   -E make_directory ${ELECTRON_APP_MODULE_DIR})

# Move the module file to the app.
add_custom_command(TARGET ${CVGUI_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} 
                   -E rename $<TARGET_FILE:${CVGUI_TARGET}> 
                   "${ELECTRON_APP_MODULE_DIR}/${CVGUI_TARGET}.node")
