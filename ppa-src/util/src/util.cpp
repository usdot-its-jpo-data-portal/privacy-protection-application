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

namespace util {
StrVector split_string(const std::string &s, char delim) {
    std::stringstream ss(s);
    std::string item;
    StrVector elems;

    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }

    if (s.back() == delim) {
        elems.push_back("");
    }

    return elems;
}

std::string& rstrip( std::string& s ) {
  return s.erase( s.find_last_not_of( DELIMITERS ) + 1 );
}

std::string& lstrip( std::string& s ) {
  return s.erase( 0, s.find_first_not_of( DELIMITERS ) );
}

std::string& strip( std::string& s ) {
  return lstrip( rstrip ( s ));
}
}
