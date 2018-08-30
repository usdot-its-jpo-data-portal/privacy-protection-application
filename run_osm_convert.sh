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

USAGE="run_osm_convert.sh [POSTGIS_ADDR] [POSTGIS_PORT] [OSM_DB_NAME] [OSM_DB_USER] [OSM_DB_PASS] [OSM_ROAD_CONFIG_FILE] [OUTPUT_FILE]"

if [ -z $1 ]; then
    echo $USAGE
    exit 1
fi

if [ -z $2 ]; then
    echo $USAGE
    exit 1
fi

if [ -z $3 ]; then
    echo $USAGE
    exit 1
fi

if [ -z $4 ]; then
    echo $USAGE
    exit 1
fi

if [ -z $5 ]; then
    echo $USAGE
    exit 1
fi

if [ -z $6 ] || [ ! -f $6 ]; then
    echo "OSM road config file: "$6" not found!"
    echo $USAGE
    exit 1
fi

if [ -z $7 ]; then
    USER_OUT_FILE=$(pwd)/osm_network.csv

    echo "No output file specified, using: $USER_OUT_FILE"
elif [ -d $7 ]; then
    echo "Output file: "$7" is a directory!" 
    echo $USAGE
    exit 1
else
    USER_OUT_FILE=$7
fi

touch $USER_OUT_FILE

if [ $? -ne 0 ]; then
    exit 1
fi

DOCKER_DIR=/tmp/osm-out
MNT_DIR=/mnt

if [ -d $DOCKER_DIR ]; then
    rm -fr $DOCKER_DIR/*
else
    mkdir $DOCKER_DIR
fi

OUT_FILE=$DOCKER_DIR/osm_network.csv
OSM_CONFIG_FILE=$DOCKER_DIR/osm_road_config.json

MNT_OUT_FILE=$MNT_DIR/osm_network.csv
MNT_OSM_CONFIG_FILE=$MNT_DIR/osm_road_config.json

# copy the configs
cp $6 $OSM_CONFIG_FILE

if [ $? -ne 0 ]; then
    exit 1
fi

docker stop osm_convert &> /dev/null || true && docker rm -f osm_convert &> /dev/null || true
docker run -it --name osm_convert -v $DOCKER_DIR:$MNT_DIR ppa_image /build/osm-cl/osm_tool -a $1 -p $2 -d $3 -u $4 -s $5 -r $MNT_OSM_CONFIG_FILE $MNT_OUT_FILE

if [ $? -ne 0 ]; then
    exit 1
fi

# copy the OSM file to user location
cp $OUT_FILE $USER_OUT_FILE
