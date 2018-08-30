/*******************************************************************************
 * Copyright 2018 UT-Battelle, LLC
 * All rights reserved
 * Route Sanitizer, version 0.9
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For issues, question, and comments, please submit a issue via GitHub.
 *******************************************************************************/
#include "geo.hpp"
#include "geo_data.hpp"
#include "tool.hpp"

int main( int argc, char **argv ) {
    // Set up the tool.
    tool::Tool tool("osm_map", "Build CSV OSM road network file from database.");
    tool.AddOption(tool::Option('h', "help", "Print this message."));
    tool.AddOption(tool::Option('r', "road_config", "The OSM way configuration file. (default: osm_road_config.json)", "osm_road_config.json"));
    tool.AddOption(tool::Option('a', "address", "The postgis address. (default: 172.17.0.2)", "172.17.0.2"));
    tool.AddOption(tool::Option('p', "port", "The postgis port. (default: 5432)", "5432"));
    tool.AddOption(tool::Option('d', "name", "The OSM postgis database name. (default: osm)", "osm"));
    tool.AddOption(tool::Option('u', "user", "The OSM postgis database username. (default: user)", "user"));
    tool.AddOption(tool::Option('s', "pass", "The OSM postgis database password. (default: password)", "password"));

    if (!tool.ParseArgs(std::vector<std::string>{argv + 1, argv + argc})) {
        exit(1);
    }

    try {
        // read the network from the db
        geo_data::OSMConfigMap osm_config = geo_data::osm_config_map(tool.GetStringVal("road_config"));
        geo_data::PostGISRoadReader road_reader(tool.GetStringVal("address"), tool.GetIntVal("port"), tool.GetStringVal("name"), tool.GetStringVal("user"), tool.GetStringVal("pass"), osm_config);

        // write the network file to csv
        geo_data::CSVRoadWriter writer(tool.GetSource());
        geo::Road::Ptr curr_road = road_reader.next_road();

        while (curr_road != nullptr) {
            writer.write_road(*curr_road);
            curr_road = road_reader.next_road();
        }
    } catch (std::invalid_argument& e) {    
        std::cerr << e.what() << std::endl; 

        return 1;
    }

    return 0;
}
