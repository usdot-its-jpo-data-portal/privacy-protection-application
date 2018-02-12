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
#include "cvlib.hpp"
#include "tool.hpp"
#include "di_multi.hpp"

int main( int argc, char **argv ) {
    // Set up the tool.
    tool::Tool tool("cv_di", "De-identify BSMP1 CSV data.");
    tool.AddOption(tool::Option('h', "help", "Print this message."));
    tool.AddOption(tool::Option('t', "thread", "The number of threads to use (default: 1 thread).", "1"));
    tool.AddOption(tool::Option('o', "out_dir", "The output directory (default: working directory).", ""));
    tool.AddOption(tool::Option('k', "kml_dir", "The KML output directory (default: working directory).", ""));
    tool.AddOption(tool::Option('q', "quad", "The file .quad file containing the circles defining the regions.", ""));
    tool.AddOption(tool::Option('c', "config", "A configuration file for de-identification.", ""));
    tool.AddOption(tool::Option('n', "count_pts", "Print summary of the points after de-identification to standard error."));
    
    if (!tool.ParseArgs(std::vector<std::string>{argv + 1, argv + argc})) {
        exit(1);
    }

    unsigned n_threads = 0;

    try {
        n_threads = static_cast<unsigned>(tool.GetIntVal("thread"));    
    } catch (std::out_of_range&) {
        std::cerr << "Invalid value for \"thread\"!" << std::endl;
        exit(1);
    }

    if (n_threads < 1) {
        std::cerr << "Number of threads must be greater than 1." << std::endl;
        exit(1);
    }
    
    try {
        DIMulti::DICSV parallel_csv(tool.GetSource(), tool.GetStringVal("quad"), tool.GetStringVal("out_dir"), tool.GetStringVal("config"), tool.GetStringVal("kml_dir"), tool.GetBoolVal("count_pts"));
        parallel_csv.Start(n_threads);
    } catch (std::invalid_argument& e) {    
        std::cerr << e.what() << std::endl; 
    }

    return 0;
}
