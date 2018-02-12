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
#include "util.hpp"
#include <sstream>

/**
 * Split the input string s by occurrences of delim and return the elements in a vector containing the component
 * strings.
 */
StrVector util::split(const std::string &s, char delim ) {
    std::stringstream ss{s};
    std::string item;
    StrVector elems;

    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }

    return elems;
}

StrPair util::split_attribute( const std::string& s, char delim ) {
    StrPair r;
    size_t pos = s.find(delim);
    if (pos < std::string::npos) {
        r.first = s.substr(0,pos);
        pos += 1;
        if (pos < std::string::npos) {
            r.second = s.substr(pos,s.size());
        }
    } 
    return r;
}
