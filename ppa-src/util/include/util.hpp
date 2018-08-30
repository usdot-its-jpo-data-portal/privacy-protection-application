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
#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <vector>

/**
 * @brief Namespace for utility functions.
 */
namespace util {
using StrVector = std::vector<std::string>;

const std::string DELIMITERS = " \f\n\r\t\v";

/**
 * \brief Split the provided string at every occurrence of delimiter and return the components in a vector of strings.
 *
 * \param s the string to split.
 * \param delim the char where the splits are to be performed; default is ','
 * \return a vector of strings.
 */
StrVector split_string(const std::string &s, char delim = ',');

/**
 * \brief Remove the whitespace from the right side of the string; this is done
 * in-place; no copy happens.
 *
 * \param s the string to trim.
 * \return the new string without the whitespace on the right side.
 */
std::string& rstrip( std::string& s );

/**
 * \brief Remove the whitespace from the left side of the string to the first
 * non-whitespace character; this is done in-place; no copy happens.
 *
 * \param s the string to trim.
 * \return the new string without the whitespace on the left side.
 */
std::string& lstrip( std::string& s );

/**
 * \brief Remove the whitespace surrounding the string; this is done in-place; no copy
 * happens.
 *
 * \param s the string to trim.
 * \return the new string with the whitespace before and after removed.
 */
std::string& strip( std::string& s );
}

#endif
