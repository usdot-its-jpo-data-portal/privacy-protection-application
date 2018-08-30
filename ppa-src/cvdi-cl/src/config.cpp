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
#include "config.hpp"

#include "util.hpp"

#include <algorithm>

namespace config {

Config::Config() {}

void Config::SetSaveMM(bool save_mm) {
    save_mm_ = save_mm;
}

void Config::SetPlotKML(bool plot_kml) {
    plot_kml_ = plot_kml;
}

void Config::SetCountPoints(bool count_points) {
    count_points_ = count_points;
}

void Config::SetFitExt(double fit_ext) {
    fit_ext_ = fit_ext;
}

void Config::SetScaleMapFit(bool scale_map_fit) {
    scale_map_fit_ = scale_map_fit;
}

void Config::SetMapFitScale(double map_fit_scale) {
    map_fit_scale_ = map_fit_scale;
}

void Config::SetHeadingGroups(uint32_t n_heading_groups) {
    n_heading_groups_ = n_heading_groups;
}

void Config::SetMinEdgeTripPoints(uint32_t min_edge_trip_points) {
    min_edge_trip_points_ = min_edge_trip_points;
}

void Config::SetTAMaxQSize(uint32_t ta_max_q_size) {
    ta_max_q_size_ = ta_max_q_size;
}

void Config::SetTAAreaWidth(double ta_area_width) {
    ta_area_width_ = ta_area_width;
}

void Config::SetTAMaxSpeed(double ta_max_speed) {
    ta_max_speed_ = ta_max_speed;
}

void Config::SetTAHeadingDelta(double ta_heading_delta) {
    ta_heading_delta_ = ta_heading_delta;
}

void Config::SetStopMaxTime(uint64_t stop_max_time) {
    stop_max_time_ = stop_max_time;
}

void Config::SetStopMinDistance(double stop_min_distance) {
    stop_min_distance_ = stop_min_distance;
}

void Config::SetStopMaxSpeed(double stop_max_speed) {
    stop_max_speed_ = stop_max_speed;
}

void Config::SetMinDirectDistance(double min_direct_distance) {
    min_direct_distance_ = min_direct_distance;
}

void Config::SetMinManhattanDistance(double min_manhattan_distance) {
    min_manhattan_distance_ = min_manhattan_distance;
}

void Config::SetMinOutDegree(uint32_t min_out_degree) {
    min_out_degree_ = min_out_degree;
}

void Config::SetMaxDirectDistance(double max_direct_distance) {
    max_direct_distance_ = max_direct_distance;
}

void Config::SetMaxManhattanDistance(double max_manhattan_distance) {
    max_manhattan_distance_ = max_manhattan_distance;
}

void Config::SetMaxOutDegree(uint32_t max_out_degree) {
    max_out_degree_ = max_out_degree;
}

void Config::SetRandDirectDistance(double rand_direct_distance) {
    rand_direct_distance_ = rand_direct_distance;
}

void Config::SetRandManhattanDistance(double rand_manhattan_distance) {
    rand_manhattan_distance_ = rand_manhattan_distance;
}

void Config::SetRandOutDegree(double rand_out_degree) {
    rand_out_degree_ = rand_out_degree;
}

void Config::SetKMLStride(uint32_t kml_stride) {
    kml_stride_ = kml_stride;
} 

void Config::SetKMLSuppressDI(bool kml_suppress_di) {
    kml_suppress_di_ = kml_suppress_di;
}

bool Config::IsSaveMM(void) const {
    return save_mm_;
}

bool Config::IsPlotKML(void) const {
    return plot_kml_;
}

bool Config::IsCountPoints(void) const {
    return count_points_;
}

double Config::GetFitExt(void) const {
    return fit_ext_;
}

bool Config::IsScaleMapFit(void) const {
    return scale_map_fit_;
}

double Config::GetMapFitScale(void) const {
    return map_fit_scale_;
}

uint32_t Config::GetHeadingGroups(void) const {
    return n_heading_groups_;
}

uint32_t Config::GetMinEdgeTripPoints(void) const {
    return min_edge_trip_points_;
}

uint32_t Config::GetTAMaxQSize(void) const {
    return ta_max_q_size_;
}

double Config::GetTAAreaWidth(void) const {
    return ta_area_width_;
}

double Config::GetTAMaxSpeed(void) const {
    return ta_max_speed_;
}

double Config::GetTAHeadingDelta(void) const {
    return ta_heading_delta_;
}

uint64_t Config::GetStopMaxTime(void) const {
    return stop_max_time_;
}

double Config::GetStopMinDistance(void) const {
    return stop_min_distance_;
}

double Config::GetStopMaxSpeed(void) const {
    return stop_max_speed_;
}

double Config::GetMinDirectDistance(void) const {
    return min_direct_distance_;
}

double Config::GetMinManhattanDistance(void) const {
    return min_manhattan_distance_;
}

uint32_t Config::GetMinOutDegree(void) const {
    return min_out_degree_;
}

double Config::GetMaxDirectDistance(void) const {
    return max_direct_distance_;
}

double Config::GetMaxManhattanDistance(void) const {
    return max_manhattan_distance_;
}

uint32_t Config::GetMaxOutDegree(void) const {
    return max_out_degree_;
}

double Config::GetRandDirectDistance(void) const {
    return rand_direct_distance_;
}

double Config::GetRandManhattanDistance(void) const {
    return rand_manhattan_distance_;
}

double Config::GetRandOutDegree(void) const {
    return rand_out_degree_;
}

uint32_t Config::GetKMLStride(void) const {
    return kml_stride_;
}

bool Config::IsKMLSuppressDI(void) const {
    return kml_suppress_di_;
}

Config::Ptr Config::ConfigFromFile(const std::string& config_file_path) {
    std::ifstream file(config_file_path);
    Config::Ptr config_ptr;

    if (file.fail()) {
        throw std::invalid_argument("Could not open configuration file: " + config_file_path);
    }

    config_ptr = ConfigFromStream(file);

    file.close();

    return config_ptr;
}

Config::Ptr Config::ConfigFromStream(std::istream& stream) {
    std::string line;                 
    util::StrVector parts;
    Config::Ptr config_ptr = std::make_shared<Config>();

    while (std::getline(stream, line)) {
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
        parts = util::split_string(line, ':'); 

        if (parts.size() < 2) {
            std::cerr << "Ignoring configuration line: " + line << std::endl;

            continue;
        }      

        std::string key = util::strip(parts[0]);
        std::string val = util::strip(parts[1]);
        
        try {
            if (key == "af_fit_ext") {
                config_ptr->SetFitExt(std::stod(val));
            } else if (key == "af_toggle_scale") {
                config_ptr->SetScaleMapFit(!!std::stoi(val));
            } else if (key == "af_scale") {
                config_ptr->SetFitExt(std::stod(val));
            } else if (key == "n_heading_groups") {
                config_ptr->SetHeadingGroups(std::stoul(val));
            } else if (key == "min_edge_trip_pts") {
                config_ptr->SetMinEdgeTripPoints(std::stoul(val));
            } else if (key == "ta_max_q_size") {
                config_ptr->SetTAMaxQSize(std::stoul(val));
            } else if (key == "ta_area_width") {
                config_ptr->SetTAAreaWidth(std::stod(val));
            } else if (key == "ta_heading_delta") {
                config_ptr->SetTAAreaWidth(std::stod(val));
            } else if (key == "ta_max_speed") {
                config_ptr->SetTAMaxSpeed(std::stod(val));
            } else if (key == "stop_min_distance") {
                config_ptr->SetStopMinDistance(std::stod(val));
            } else if (key == "stop_max_time") {
                config_ptr->SetStopMaxTime(std::stoll(val));
            } else if (key == "stop_max_speed") {
                config_ptr->SetStopMaxSpeed(std::stod(val));
            } else if (key == "min_direct_distance") {
                config_ptr->SetMinDirectDistance(std::stod(val));
            } else if (key == "min_manhattan_distance") {
                config_ptr->SetMinManhattanDistance(std::stod(val));
            } else if (key == "min_out_degree") {
                config_ptr->SetMinOutDegree(std::stoul(val));
            } else if (key == "max_direct_distance") {
                config_ptr->SetMaxDirectDistance(std::stod(val));
            } else if (key == "max_manhattan_distance") {
                config_ptr->SetMaxManhattanDistance(std::stod(val));
            } else if (key == "max_out_degree") {
                config_ptr->SetMaxOutDegree(std::stoul(val));
            } else if (key == "rand_direct_distance") {
                config_ptr->SetRandDirectDistance(std::stod(val));
            } else if (key == "rand_manhattan_distance") {
                config_ptr->SetRandManhattanDistance(std::stod(val));
            } else if (key == "rand_out_degree") {
                config_ptr->SetRandOutDegree(std::stod(val));
            } else if (key == "kml_stride") {
                config_ptr->SetKMLStride(std::stoul(val));
            } else if (key == "kml_suppress_di") {
                config_ptr->SetKMLSuppressDI(!!std::stoi(val));
            } else if (key == "save_mm") {
                config_ptr->SetSaveMM(!!std::stoi(val));
            } else if (key == "plot_kml") {
                config_ptr->SetPlotKML(!!std::stoi(val));
            } else if (key == "count_points") {
                config_ptr->SetCountPoints(!!std::stoi(val));
            } else {
                std::cerr << "Ignoring configuration line: " + line << std::endl;
            }
        } catch (std::exception&) {
            std::cerr << "Error parsing configuration line: " + line << std::endl;

            continue;
        }
    } 

    return config_ptr;
}

void Config::PrintConfig(std::ostream& stream) const {
    stream << "*********************************** Configuration ****************************************" << std::endl;
    stream << "Save Map Matching Results: " << save_mm_ << std::endl;
    stream << "Plot KML: " << plot_kml_ << std::endl;
    stream << "Count Points: " << count_points_ << std::endl;
    stream << "Area fit extension: " << fit_ext_  << std::endl;
    stream << "Scale area fit: " << scale_map_fit_  << std::endl;
    stream << "N Heading groups: " << n_heading_groups_ << std::endl;
    stream << "Min edge trip points: " << min_edge_trip_points_ << std::endl;
    stream << "TA max queue size: " << ta_max_q_size_ << std::endl;
    stream << "TA area width: " << ta_area_width_ << std::endl;
    stream << "TA heading delta: " << ta_heading_delta_ << std::endl;
    stream << "Stop max time: " << stop_max_speed_ << std::endl;
    stream << "Stop min distance: " << stop_min_distance_ << std::endl;
    stream << "Stop max speed: " << stop_max_speed_ << std::endl;
    stream << "Min direct distance: " << min_direct_distance_ << std::endl;
    stream << "Min manhattan distance: " << min_manhattan_distance_ << std::endl;
    stream << "Min out degree: " << min_out_degree_ << std::endl;
    stream << "Max direct distance: " << max_direct_distance_ << std::endl;
    stream << "Max manhattan distance: " << max_manhattan_distance_ << std::endl;
    stream << "Max out degree: " << max_out_degree_ << std::endl;
    stream << "Rand direct distance: " << rand_direct_distance_ << std::endl; 
    stream << "Rand manhattan distance: " << rand_manhattan_distance_ << std::endl; 
    stream << "Rand out degree: " << rand_out_degree_ << std::endl; 
    stream << "KML Stride: " << kml_stride_ << std::endl;
    stream << "KML Suppess DeIdentified Trace: " << kml_suppress_di_ << std::endl;
    stream << "*****************************************************************************************" << std::endl;
}

}
