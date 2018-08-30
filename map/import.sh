#!/bin/bash

#
# Copyright (C) 2015, BMW Car IT GmbH
#
# Author: Sebastian Mattheis <sebastian.mattheis@bmw-carit.de>
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
# in compliance with the License. You may obtain a copy of the License at
# http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in
# writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific
# language governing permissions and limitations under the License.
#
# Modified 5/14/2018 Aaron Ferber
database=${POSTGRES_DB}
user=${POSTGRES_USER}
password=${POSTGRES_PASSWORD}
input=/mnt/input.pbf
config=/root/tools/road-types.json
mode="slim"

echo $database
echo $user

echo "Starting population of OSM data (osmosis) ..."
psql -h localhost -d ${database} -U ${user} -f /root/pgsnapshot_schema_0.6.sql
rm -rf /mnt/tmp
mkdir /mnt/tmp

if [ -z "$JAVACMD_OPTIONS" ]; then
    JAVACMD_OPTIONS="-Djava.io.tmpdir=/mnt/tmp"
else
    JAVACMD_OPTIONS="$JAVACMD_OPTIONS -Djava.io.tmpdir=/mnt/tmp"
fi

export JAVACMD_OPTIONS
/opt/osmosis/package/bin/osmosis --read-pbf file=${input} --tf accept-ways highway=* --write-pgsql user="${user}" password="${password}" database="${database}"
echo "Done."

echo "Start extraction of routing data (bfmap tools) ..."
cd /root/tools/

if [ "$mode" = "slim" ]
then
	python osm2ways.py --host localhost --port 5432 --database ${database} --table temp_ways --user ${user} --password ${password} --slim
elif [ "$mode" = "normal" ]
then
	python osm2ways.py --host localhost --port 5432 --database ${database} --table temp_ways --user ${user} --password ${password} --prefix _tmp
fi

python ways2bfmap.py --source-host localhost --source-port 5432 --source-database ${database} --source-table temp_ways --source-user ${user} --source-password ${password} --target-host localhost --target-port 5432 --target-database ${database} --target-table bfmap_ways --target-user ${user} --target-password ${password} --config ${config}
echo "Done."
