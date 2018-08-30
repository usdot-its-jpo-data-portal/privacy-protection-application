#!/bin/bash

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

USAGE="host_import.sh [MAP_FILE]"

if [ -z $1 ] || [ ! -f $1 ]; then
    echo "Map file: "$1" not found!"
    echo $USAGE
    exit 1
fi

DOCKER_DIR=/tmp/cvdi-postgis

cp $1 $DOCKER_DIR/input.pbf

if [ $? -ne 0 ]; then
    exit 1
fi

# Note: DO NOT ADD -it to this command. 
# It will cause issues.
docker exec cvdi_postgis /root/import.sh
