#!/bin/bash

# Copyright 2018 UT-Battelle, LLC
# All rights reserved
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

USAGE="run_ppa.sh [BATCH_FILE] [OSM_FILE] [CVDI_CONFIG_FILE] [OUTPUT_DIR] [N_THREADS]"

if [ -z $1 ] || [ ! -f $1 ]; then
    echo "Batch file: "$1" not found!"
    echo $USAGE
    exit 1
fi

if [ -z $2 ] || [ ! -f $2 ]; then
    echo "OSM file: "$2" not found!"
    echo $USAGE
    exit 1
fi

if [ -z $3 ] || [ ! -f $3 ]; then
    echo "Config file: "$3" not found!"
    echo $USAGE
    exit 1
fi

if [ -z $4 ] || [ ! -d $4 ]; then
    echo "Output directory: "$4" not found!"
    echo $USAGE
    exit 1
fi

if [ -z $5 ]; then
    echo "Must enter number of threads!"
    echo $USAGE
    exit 1
elif ! test "$5" -gt 0 2> /dev/null ; then
    echo "Number of threads must be a positive integer"
    echo $USAGE
    exit 1
fi

DOCKER_DIR=/tmp/ppa-out
MNT_DIR=/mnt

if [ -d $DOCKER_DIR ]; then
    rm -fr $DOCKER_DIR/*
else
    mkdir $DOCKER_DIR
fi

TRACE_DIR=$DOCKER_DIR/traces
OUT_DIR=$DOCKER_DIR/out
BATCH_FILE=$DOCKER_DIR/traces.batch
OSM_FILE=$DOCKER_DIR/osm_network.csv
CONFIG_FILE=$DOCKER_DIR/ppa_config.ini

MNT_TRACE_DIR=$MNT_DIR/traces
MNT_OUT_DIR=$MNT_DIR/out
MNT_BATCH_FILE=$MNT_DIR/traces.batch
MNT_OSM_FILE=$MNT_DIR/osm_network.csv
MNT_CONFIG_FILE=$MNT_DIR/ppa_config.ini

mkdir $TRACE_DIR
mkdir $OUT_DIR

# copy batch files into shared
for f in $(cat $1); do
    cp $f $TRACE_DIR

    if [ $? -ne 0 ]; then
        exit 1
    fi
done

# make the docker batch file
for f in $(ls $TRACE_DIR); do
    echo $MNT_TRACE_DIR/$f > $BATCH_FILE

    if [ $? -ne 0 ]; then
        exit 1
    fi
done

# copy the osm file
cp $2 $OSM_FILE

if [ $? -ne 0 ]; then
    exit 1
fi

# copy the config
cp $3 $CONFIG_FILE

if [ $? -ne 0 ]; then
    exit 1
fi

docker stop ppa &> /dev/null || true && docker rm -f ppa &> /dev/null || true
docker run -it --name ppa -v $DOCKER_DIR:$MNT_DIR ppa_image /build/cvdi-cl/cvdi -t $5 -e $MNT_OSM_FILE -o $MNT_OUT_DIR -c $MNT_CONFIG_FILE $MNT_BATCH_FILE

if [ $? -ne 0 ]; then
    exit 1
fi

# copy the results to the user location
cp $OUT_DIR/* $4
