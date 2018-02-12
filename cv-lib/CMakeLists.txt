cmake_minimum_required(VERSION 2.6)

# The version number.
set(CVLIB CVLIB)
set(CVLIB_VERSION_MAJOR 1)
set(CVLIB_VERSION_MINOR 0)
set(CVLIB_VERSION "${CVLIB_VERSION_MAJOR}.${CVLIB_VERSION_MINOR}")
set(CVLIB_CURRENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CVLIB_INCLUDE_DIR "${CVLIB_CURRENT_DIR}/include")
set(CVLIB_OUT_INCLUDE_DIR "${PROJECT_BINARY_DIR}/include")

set(CVLIB_SRC "src/quad.cpp" 
              "src/utilities.cpp" 
              "src/osm.cpp" 
              "src/entity.cpp" 
              "src/shapes.cpp"
              "src/mapfit.cpp"
              "src/kml.cpp"
              "src/trajectory.cpp"
              "src/critical.cpp"
              "src/privacy.cpp"
              "src/bsmp1.cpp"
              "src/instrument.cpp"
              "src/error.cpp")

# Make the library.
add_library(CVLib STATIC ${CVLIB_SRC})
set_target_properties(CVLib PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Make the include directory in the build.
add_custom_command(TARGET CVLib PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${CVLIB_INCLUDE_DIR})

# Configure and copy the headers.
configure_file("${CVLIB_CURRENT_DIR}/cvlib.hpp.in" "${CVLIB_OUT_INCLUDE_DIR}/cvlib.hpp")
configure_file("${CVLIB_INCLUDE_DIR}/shapes.hpp" "${CVLIB_OUT_INCLUDE_DIR}/shapes.hpp" COPYONLY)
configure_file("${CVLIB_INCLUDE_DIR}/bsmp1.hpp" "${CVLIB_OUT_INCLUDE_DIR}/bsmp1.hpp" COPYONLY)
configure_file("${CVLIB_INCLUDE_DIR}/critical.hpp" "${CVLIB_OUT_INCLUDE_DIR}/critical.hpp" COPYONLY)
configure_file("${CVLIB_INCLUDE_DIR}/entity.hpp" "${CVLIB_OUT_INCLUDE_DIR}/entity.hpp" COPYONLY)
configure_file("${CVLIB_INCLUDE_DIR}/kml.hpp" "${CVLIB_OUT_INCLUDE_DIR}/kml.hpp" COPYONLY)
configure_file("${CVLIB_INCLUDE_DIR}/mapfit.hpp" "${CVLIB_OUT_INCLUDE_DIR}/mapfit.hpp" COPYONLY)
configure_file("${CVLIB_INCLUDE_DIR}/names.hpp" "${CVLIB_OUT_INCLUDE_DIR}/names.hpp" COPYONLY)
configure_file("${CVLIB_INCLUDE_DIR}/osm.hpp" "${CVLIB_OUT_INCLUDE_DIR}/osm.hpp" COPYONLY)
configure_file("${CVLIB_INCLUDE_DIR}/privacy.hpp" "${CVLIB_OUT_INCLUDE_DIR}/privacy.hpp" COPYONLY)
configure_file("${CVLIB_INCLUDE_DIR}/quad.hpp" "${CVLIB_OUT_INCLUDE_DIR}/quad.hpp" COPYONLY)
configure_file("${CVLIB_INCLUDE_DIR}/trajectory.hpp" "${CVLIB_OUT_INCLUDE_DIR}/trajectory.hpp" COPYONLY)
configure_file("${CVLIB_INCLUDE_DIR}/utilities.hpp" "${CVLIB_OUT_INCLUDE_DIR}/utilities.hpp" COPYONLY)
configure_file("${CVLIB_INCLUDE_DIR}/instrument.hpp" "${CVLIB_OUT_INCLUDE_DIR}/instrument.hpp" COPYONLY)
configure_file("${CVLIB_INCLUDE_DIR}/error.hpp" "${CVLIB_OUT_INCLUDE_DIR}/error.hpp" COPYONLY)

# Just include the location where everything is copied to.
include_directories(${CVLIB_OUT_INCLUDE_DIR})

set(CVLIB_INCLUDE ${CVLIB_OUT_INCLUDE_DIR} PARENT_SCOPE)
