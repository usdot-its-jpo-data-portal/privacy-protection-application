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
#include "tool.hpp"

#include <iomanip>

namespace tool {

Option::Option(char short_name, const std::string& long_name, const std::string& description) :
            short_name_{short_name},
            long_name_{long_name},
            description_{description},
            val_{""},
            is_flag_{true},
            has_val_{false} 
        {}

Option::Option(char short_name, const std::string& long_name, const std::string& description, const std::string& default_val) :
            short_name_{short_name},
            long_name_{long_name},
            description_{description},
            val_{default_val},
            is_flag_{false},
            has_val_{!default_val.empty()} 
        {}

char Option::GetShortName() const {
    return short_name_;
}

const std::string& Option::GetLongName() const {
    return long_name_;
}

void Option::SetVal(const std::string& val) {
    val_ = val;
    has_val_ = true;
}

void Option::SetVal() {
    val_ = "1";
    has_val_ = true;
}
            
const std::string& Option::GetStringVal() const {
    return val_;
}            

int Option::GetIntVal() const {
    return std::stoi(val_);
}
        
uint64_t Option::GetUint64Val() const {
    return std::stoull(val_);
}

double Option::GetDoubleVal() const {
    return std::stod(val_);
}

bool Option::GetBoolVal() const {
    return !val_.empty();
}

bool Option::IsFlag() const {
    return is_flag_;
}

bool Option::HasVal() const {
    return has_val_;
}

std::ostream& operator<<(std::ostream& os, const Option& option) {
    std::string long_name_trunc = option.long_name_.size() > 15 ? option.long_name_.substr(0, 15) : option.long_name_;
    size_t width = 15 - long_name_trunc.size();
    return os << "-" << option.short_name_ << ", --" << long_name_trunc << std::setw(width) << "  " << option.description_;
}

Tool::Tool(const std::string& tool_name, const std::string& description, std::ostream& os) :
    name_{tool_name},
    description_(description),
    os_{os},
    has_last_opt_{false}
    {}

Tool::Tool(const Tool& command) :
    name_(command.name_),
    description_(command.description_),
    os_(command.os_),
    last_opt_(command.last_opt_),
    has_last_opt_(command.has_last_opt_),
    option_map_(command.option_map_),
    short_option_map_(command.short_option_map_)
    {}

void Tool::AddOption(const Option& option) {
    option_map_.insert(std::make_pair<std::string, Option>(static_cast<std::string>(option.GetLongName()), static_cast<Option>(option)));
    short_option_map_.insert(std::make_pair<char, std::string>(option.GetShortName(), static_cast<std::string>(option.GetLongName())));
}

void Tool::PrintHelp() {
    os_ << name_;
    os_ << std::endl;
    os_ << description_;
    os_ << std::endl;

    for (auto& map_ele : option_map_) {
        os_ << " " << map_ele.second;
        os_ << std::endl;
    }
}

void Tool::PrintUsage() {
    os_ << "Usage: " << name_ << " [OPTIONS] SOURCE" << std::endl;
}

const std::string& Tool::GetName() const {
    return name_;
}

const std::string& Tool::GetDesciption() const {
    return description_;
}


bool Tool::HandleShortArgs(const std::string& short_args) {
    for (auto& c : short_args) {
        if (c == 'h') {
            PrintHelp();

            return false;
        }

        try {
            option_map_.at(short_option_map_.at(c)).SetVal();

            if (!option_map_.at(short_option_map_.at(c)).IsFlag()) {
                last_opt_ = short_option_map_.at(c);
                has_last_opt_ = true;
            } else {
                has_last_opt_ = false;
            }

        } catch (std::out_of_range&) {
            os_ << "Invalid argument: " << c << std::endl;  

            return false;
        }
    }
    
    return true;
}

bool Tool::HandleLongArg(const std::string& long_arg) {
    if (long_arg == "help") {
        PrintHelp();
        
        return false;
    }

    try {
        option_map_.at(long_arg).SetVal();

        if (!option_map_.at(long_arg).IsFlag()) {
            last_opt_ = long_arg;
            has_last_opt_ = true;
        } else {
            has_last_opt_ = false;
        }

        return true;
    } catch (std::out_of_range&) {
        os_ << "Invalid argument: " << long_arg << std::endl;  

        return false;
    }
} 

void Tool::HandleOption(const std::string& option) {
    option_map_.at(last_opt_).SetVal(option);
}

bool Tool::ParseArgs(const std::vector<std::string> args) {
    bool has_source = false;

    if (args.size() < 1) {
        PrintUsage();
    
        return false;
    }

    for (auto& arg : args) {
        source_ = arg;

        if (has_last_opt_) {
            HandleOption(arg);
            has_last_opt_ = false;
    
            continue;
        } 
        
        if (arg.substr(0, 2) == "--") {
            if (!HandleLongArg(arg.substr(2))) {
                PrintUsage();

                return false;
            }
        } else if (arg.substr(0, 1) == "-") {
            if (!HandleShortArgs(arg.substr(1))) {
                PrintUsage();

                return false;
            }
        } else if (!has_source) {
            has_source = true;
        } else {
            os_ << "Invalid argument: " << arg << std::endl;  
            PrintUsage();

            return false;
        }
    }

    if (!has_source) {
        os_ << "No source file!" << std::endl;
        PrintUsage();
    
        return false;
    } 

    return true;
}

const std::string& Tool::GetStringVal(const std::string option) const {
    return option_map_.at(option).GetStringVal();
}

int Tool::GetIntVal(const std::string option) const {
    return option_map_.at(option).GetIntVal();
}
        
uint64_t Tool::GetUint64Val(const std::string option) const {
    return option_map_.at(option).GetUint64Val();
}

double Tool::GetDoubleVal(const std::string option) const {
    return option_map_.at(option).GetDoubleVal();
}

bool Tool::GetBoolVal(const std::string option) const {
    return option_map_.at(option).GetBoolVal();
}

/*
void Tool::DumpConfig() const {
    for (auto& map_elem : option_map_) {
        os_ << map_elem.first << ": " << map_elem.second.GetStringVal() << std::endl;
    } 
}
*/

const std::string& Tool::GetSource() const {
    return source_;
}
}
