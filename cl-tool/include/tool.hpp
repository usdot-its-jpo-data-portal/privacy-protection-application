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
#ifndef TOOL_HPP
#define TOOL_HPP

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace tool {
class Option {
    public:
        Option(char short_name, const std::string& long_name, const std::string& description);
        Option(char short_name, const std::string& long_name, const std::string& description, const std::string& default_val);
        char GetShortName(void) const;
        const std::string& GetLongName(void) const;
        void SetVal(const std::string& val);
        void SetVal();
        const std::string& GetStringVal() const;
        int GetIntVal() const;
        uint64_t GetUint64Val() const;
        double GetDoubleVal() const;
        bool GetBoolVal() const;
        bool IsFlag() const;
        bool HasVal() const;
        friend std::ostream& operator<<(std::ostream& os, const Option& option);
    private:
        char short_name_;
        std::string long_name_;
        std::string description_;
        std::string val_;
        bool is_flag_;
        bool has_val_;
};

class Tool {
    public:
        Tool(const std::string& tool_name, const std::string& description, std::ostream& os = std::cerr);
        Tool(const Tool& command);
        void PrintHelp(void);
        void PrintUsage(void);
        const std::string& GetName(void) const;
        const std::string& GetDesciption(void) const;
        bool ParseArgs(const std::vector<std::string> args);
        void AddOption(const Option& option);
        const std::string& GetStringVal(const std::string option) const;
        int GetIntVal(const std::string option) const;
        uint64_t GetUint64Val(const std::string option) const;
        double GetDoubleVal(const std::string option) const;
        bool GetBoolVal(const std::string option) const;
        const std::string& GetSource() const;
    private:
        std::string name_;
        std::string description_;
        std::ostream& os_;
        std::unordered_map<std::string, Option> option_map_;
        std::string source_;
        std::unordered_map<char, std::string> short_option_map_;
        std::string last_opt_;
        bool has_last_opt_;

        bool HandleShortArgs(const std::string& short_args);
        bool HandleLongArg(const std::string& long_arg);
        void HandleOption(const std::string& option);
};
}

#endif
