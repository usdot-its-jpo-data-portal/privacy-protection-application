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

/**
 * \brief Namespace for command line tool functions.
 */
namespace tool {

/**
 * \brief A basic command line option.
 */
class Option {
    public:

        /**
         * \brief Option constructor with no default value.
         *
         * \param short_name the character used for the short name of the option
         * \param long_name the string used for the long name of the option
         * \param description the string used for the description of the option
         */
        Option(char short_name, const std::string& long_name, const std::string& description);

        /**
         * \brief Option constructor with a default value.
         *
         * \param short_name the character used for the short name of the option
         * \param long_name the string used for the long name of the option
         * \param description the string used for the description of the option
         * \param default_val the string used for the default value of the option
         */
        Option(char short_name, const std::string& long_name, const std::string& description, const std::string& default_val);

        /**
         * \brief Get short name of this option.
         * 
         * \return the character representing the short name of this option
         */
        char GetShortName(void) const;
    
        /**
         * \brief Get the long name of this option.
         * 
         * \return the string representing the long name of this option
         */
        const std::string& GetLongName(void) const;

        /**
         * \brief Set the value of this option.
         * 
         * \param val the string representing the new value of this option
         */
        void SetVal(const std::string& val);

        /**
         * \brief Set the value of this option to true.
         */
        void SetVal();

        /**
         * \brief Get the value of this option.
         *
         * \return the string value of this option
         */
        const std::string& GetStringVal() const;

        /**
         * \brief Get the value of this option as an integer.
         * 
         * \return the integer value of this option
         * \throws std::invalid_argument if the value cannot be converted to an integer
         */
        int GetIntVal() const;
        
        /**
         * \brief Get the value of this option as an unsigned long long (unsigned 64-bit).
         * 
         * \return the unsigned long long value of this option
         * \throws std::invalid_argument if the value cannot be converted to a long long value
         */
        uint64_t GetUint64Val() const;

        /**
         * \brief Get the value of this option as a double.
         * 
         * \return the double value of this option
         * \throws std::invalid_argument if the value cannot be converted to a double
         */
        double GetDoubleVal() const;

        /**
         * \brief Get the value of this option as a boolean. This returns True if the underlying string value is anything other than an empty string. Otherwise returns False.
         * 
         * \return the boolean value of this option
         */
        bool GetBoolVal() const;

        /**
         * \brief Get the flag status of this option.
         * 
         * \return true if the option is a flag, otherwise false
         */
        bool IsFlag() const;

        /**
         * \brief Get the value status of this option.
         * 
         * \return true if this option has its value set, otherwise false
         */
        bool HasVal() const;

        /**
         * \brief Write an Option to a stream formatted for command line help output.
         * 
         * \param os the stream object where the point will be written
         * \param pt the option to write
         * \return returns the given stream object
         */
        friend std::ostream& operator<<(std::ostream& os, const Option& option);
    private:
        char short_name_;
        std::string long_name_;
        std::string description_;
        std::string val_;
        bool is_flag_;
        bool has_val_;
};

/**
 * \brief A basic command line tool, represented as a container for options. A tool is a simple command line interface. It supports multiple optional arguments and a single positional source argument.
 */
class Tool {
    public:
        /**
         * \brief New tool constructor.
         *
         * \param tool_name the name of the command line tool
         * \param description the description for this command line tool
         * \param os the error stream used to display help and error information
         */
        Tool(const std::string& tool_name, const std::string& description, std::ostream& os = std::cerr);

        /**
         * \brief Tool copy constructor.
         * 
         * \param command the Tool to copy
         */
        Tool(const Tool& command);

        /**
         * \brief Print the help message for this tool to the error stream.
         */
        void PrintHelp(void);

        /**
         * \brief Print the usage message for this tool to the error stream.
         */
        void PrintUsage(void);

        /**
         * \brief Get the name of the tool.
         *
         * \return the name of the tool
         */
        const std::string& GetName(void) const;

        /**
         * \brief Get the description of this tool.
         * 
         * \return the description of this tool
         */
        const std::string& GetDesciption(void) const;

        /**
         * \brief Parse the command line options and store the results in this tool.
         * 
         * \param args the command line arguments
         * \return true if the parsing succeeds, otherwise false
         */
        bool ParseArgs(const std::vector<std::string> args);

        /**
         * \brief Add a new Option to this tool. This maps the option's long name to the value of the option.
         * 
         * \param option The new option to add.
         */
        void AddOption(const Option& option);

        /**
         * \brief Get the string value for an option supported by this tool.
         *
         * \param option the long name of the option
         * \return the string representation of the option
         * \throws std::out_of_range if the option can not be found in the tool
         */
        const std::string& GetStringVal(const std::string option) const;

        /**
         * \brief Get the integer value for an option supported by this tool.
         * 
         * \param option the long name of the option
         * \return the integer representation of this option
         * \throws std::out_of_range if the option can not be found in the tool
         * \throws std::invalid_argument if the value cannot be converted to an integer.
         */
        int GetIntVal(const std::string option) const;

        /**
         * \brief Get the unsigned long long (unsigned 64-bit) value for an option supported by this tool.
         * 
         * \param option the long name of the option
         * \return the unsigned long long value of this option
         * \throws std::out_of_range if the option can not be found in the tool
         * \throws std::invalid_argument if the value cannot be converted to a long long (unsigned 64-bit).
         */
        uint64_t GetUint64Val(const std::string option) const;

        /**
         * \brief Get the double value for an option supported by this tool.
         * 
         * \param option the long name of the option
         * \return the integer representation of this option
         * \throws std::out_of_range if the option can not be found in the tool
         * \throws std::invalid_argument if the value cannot be converted to an double.
         */
        double GetDoubleVal(const std::string option) const;

        /**
         * \brief Get the boolean value for an option supported by this tool.
         * 
         * \param option the long name of the option
         * \return the boolean representation of this option
         * \throws std::out_of_range if the option can not be found in the tool
         */
        bool GetBoolVal(const std::string option) const;

        /**
         * \brief Get the positional source argument for this tool.
         * 
         * \return the positional source argument
         */        
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
