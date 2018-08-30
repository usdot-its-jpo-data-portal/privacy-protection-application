#!/bin/sh

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

USAGE="run_postgis.sh [POSTGRES_DB] [POSTGRES_USER] [POSTGRES_PASSWORD]"

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

POSTGRES_DB=$1
POSTGRES_USER=$2
POSTGRES_PASSWORD=$3

DOCKER_DIR=/tmp/ppa-postgis
MNT_DIR=/mnt

if [ -d $DOCKER_DIR ]; then
    rm -fr $DOCKER_DIR/*
else
    mkdir $DOCKER_DIR
fi

docker stop ppa_postgis &> /dev/null || true && docker rm -f ppa_postgis &> /dev/null || true
docker run -d -p 5432:5432 --name ppa_postgis -e POSTGRES_DB=$POSTGRES_DB -e POSTGRES_USER=$POSTGRES_USER -e POSTGRES_PASSWORD=$POSTGRES_PASSWORD -v $DOCKER_DIR:$MNT_DIR ppa_postgis_image
