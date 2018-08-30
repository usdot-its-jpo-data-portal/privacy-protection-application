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
FROM ubuntu:18.04
USER root

WORKDIR /code

# add build tools
RUN apt-get update && apt-get install -y software-properties-common make gcc-7 g++-7 gcc-7-base && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 100 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 100

# install wget
RUN apt-get install -y wget

# install cmake
RUN wget https://cmake.org/files/v3.7/cmake-3.7.2.tar.gz && tar -xvf cmake-3.7.2.tar.gz
RUN cd cmake-3.7.2 && ./bootstrap && make && make install && cd /

# install geographic lib
RUN wget https://sourceforge.net/projects/geographiclib/files/distrib/GeographicLib-1.49.tar.gz && tar -xvf GeographicLib-1.49.tar.gz
RUN cd GeographicLib-1.49 && mkdir build && cd build && cmake .. && make && make install

# install postgres libraries
RUN apt-get install -y libpqxx-dev libpqxx-4.0v5

# install GDAL/OGR libraries
RUN apt-get install -y libgdal-dev

# install rapidjson
RUN apt-get install -y git
RUN git clone https://github.com/Tencent/rapidjson.git && cd rapidjson && mkdir build && cd build && cmake .. && make && make install && cd /

# add the source and build files
RUN mkdir /code/cvdi
ADD CMakeLists.txt /code/cvdi
ADD util /code/cvdi/util
ADD geo /code/cvdi/geo
ADD geo-data /code/cvdi/geo-data
ADD hmm-mm /code/cvdi/hmm-mm
ADD cvdi /code/cvdi/cvdi
ADD tool /code/cvdi/tool
ADD cvdi-cl /code/cvdi/cvdi-cl
ADD osm-cl /code/cvdi/osm-cl
ADD test /code/cvdi/test

# do the build.
RUN mkdir /build && cd /build && cmake /code/cvdi && make
