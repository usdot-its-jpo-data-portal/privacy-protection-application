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
#include <algorithm>

namespace Config {
    DIConfig::DIConfig() {}

    void DIConfig::SetLatField(const std::string& lat_field) {
        lat_field_ = lat_field;
    }

    void DIConfig::SetLonField(const std::string& lon_field) {
       lon_field_ = lon_field;
    }

    void DIConfig::SetHeadingField(const std::string& heading_field) {
        heading_field_ = heading_field;
    }

    void DIConfig::SetSpeedField(const std::string& speed_field) {
        speed_field_ = speed_field;
    }
    
    void DIConfig::SetGentimeField(const std::string& gentime_field) {
        gentime_field_ = gentime_field;
    }

    void DIConfig::SetUIDFields(const std::string& uid_fields) {
        uid_fields_ = uid_fields;
    }

    void DIConfig::SetQuadSWLat(double quad_sw_lat) {
        quad_sw_lat_ = quad_sw_lat;
    }

    void DIConfig::SetQuadSWLng(double quad_sw_lng) {
        quad_sw_lng_ = quad_sw_lng;
    }

    void DIConfig::SetQuadNELat(double quad_ne_lat) {
        quad_ne_lat_ = quad_ne_lat;
    }

    void DIConfig::SetQuadNELng(double quad_ne_lng) {
        quad_ne_lng_ = quad_ne_lng;
    }

    void DIConfig::SetFitExt(double fit_ext) {
        fit_ext_ = fit_ext;
    }

    void DIConfig::ToggleScaleMapFit(bool scale_map_fit) {
        scale_map_fit_ = scale_map_fit;
    }

    void DIConfig::SetMapFitScale(double map_fit_scale) {
        map_fit_scale_ = map_fit_scale;
    }

    void DIConfig::SetHeadingGroups(uint32_t n_heading_groups) {
        n_heading_groups_ = n_heading_groups;
    }

    void DIConfig::SetMinEdgeTripPoints(uint32_t min_edge_trip_points) {
        min_edge_trip_points_ = min_edge_trip_points;
    }

    void DIConfig::SetTAMaxQSize(uint32_t ta_max_q_size) {
        ta_max_q_size_ = ta_max_q_size;
    }

    void DIConfig::SetTAAreaWidth(double ta_area_width) {
        ta_area_width_ = ta_area_width;
    }

    void DIConfig::SetTAMaxSpeed(double ta_max_speed) {
        ta_max_speed_ = ta_max_speed;
    }

    void DIConfig::SetTAHeadingDelta(double ta_heading_delta) {
        ta_heading_delta_ = ta_heading_delta;
    }

    void DIConfig::SetStopMaxTime(double stop_max_time) {
        stop_max_time_ = stop_max_time;
    }

    void DIConfig::SetStopMinDistance(double stop_min_distance) {
        stop_min_distance_ = stop_min_distance;
    }

    void DIConfig::SetStopMaxSpeed(double stop_max_speed) {
        stop_max_speed_ = stop_max_speed;
    }

    void DIConfig::SetMinDirectDistance(double min_direct_distance) {
        min_direct_distance_ = min_direct_distance;
    }

    void DIConfig::SetMinManhattanDistance(double min_manhattan_distance) {
        min_manhattan_distance_ = min_manhattan_distance;
    }

    void DIConfig::SetMinOutDegree(uint32_t min_out_degree) {
        min_out_degree_ = min_out_degree;
    }

    void DIConfig::SetMaxDirectDistance(double max_direct_distance) {
        max_direct_distance_ = max_direct_distance;
    }

    void DIConfig::SetMaxManhattanDistance(double max_manhattan_distance) {
        max_manhattan_distance_ = max_manhattan_distance;
    }

    void DIConfig::SetMaxOutDegree(uint32_t max_out_degree) {
        max_out_degree_ = max_out_degree;
    }

    void DIConfig::SetRandDirectDistance(double rand_direct_distance) {
        rand_direct_distance_ = rand_direct_distance;
    }

    void DIConfig::SetRandManhattanDistance(double rand_manhattan_distance) {
        rand_manhattan_distance_ = rand_manhattan_distance;
    }

    void DIConfig::SetRandOutDegree(double rand_out_degree) {
        rand_out_degree_ = rand_out_degree;
    }
            
    void DIConfig::TogglePlotKML(bool plot_kml) {
        plot_kml_ = plot_kml;
    }

    const std::string& DIConfig::GetLatField(void) const {
        return lat_field_;
    }

    const std::string& DIConfig::GetLonField(void) const {
        return lon_field_;
    }

    const std::string& DIConfig::GetSpeedField(void) const {
        return speed_field_;
    }

    const std::string& DIConfig::GetHeadingField(void) const {
        return heading_field_;
    }

    const std::string& DIConfig::GetGentimeField(void) const {
        return gentime_field_;
    }

    const std::string& DIConfig::GetUIDFields(void) const {
        return uid_fields_;
    }

    double DIConfig::GetQuadSWLat(void) const {
        return quad_sw_lat_;
    }

    double DIConfig::GetQuadSWLng(void) const {
        return quad_sw_lng_;
    }

    double DIConfig::GetQuadNELat(void) const {
        return quad_ne_lat_;
    }

    double DIConfig::GetQuadNELng(void) const {
        return quad_ne_lng_;
    }

    double DIConfig::GetFitExt(void) const {
        return fit_ext_;
    }

    bool DIConfig::IsScaleMapFit(void) const {
        return scale_map_fit_;
    }

    double DIConfig::GetMapFitScale(void) const {
        return map_fit_scale_;
    }

    uint32_t DIConfig::GetHeadingGroups(void) const {
        return n_heading_groups_;
    }

    uint32_t DIConfig::GetMinEdgeTripPoints(void) const {
        return min_edge_trip_points_;
    }

    uint32_t DIConfig::GetTAMaxQSize(void) const {
        return ta_max_q_size_;
    }

    double DIConfig::GetTAAreaWidth(void) const {
        return ta_area_width_;
    }

    double DIConfig::GetTAMaxSpeed(void) const {
        return ta_max_speed_;
    }

    double DIConfig::GetTAHeadingDelta(void) const {
        return ta_heading_delta_;
    }

    double DIConfig::GetStopMaxTime(void) const {
        return stop_max_time_;
    }

    double DIConfig::GetStopMinDistance(void) const {
        return stop_min_distance_;
    }

    double DIConfig::GetStopMaxSpeed(void) const {
        return stop_max_speed_;
    }

    double DIConfig::GetMinDirectDistance(void) const {
        return min_direct_distance_;
    }

    double DIConfig::GetMinManhattanDistance(void) const {
        return min_manhattan_distance_;
    }

    uint32_t DIConfig::GetMinOutDegree(void) const {
        return min_out_degree_;
    }

    double DIConfig::GetMaxDirectDistance(void) const {
        return max_direct_distance_;
    }

    double DIConfig::GetMaxManhattanDistance(void) const {
        return max_manhattan_distance_;
    }

    uint32_t DIConfig::GetMaxOutDegree(void) const {
        return max_out_degree_;
    }

    double DIConfig::GetRandDirectDistance(void) const {
        return rand_direct_distance_;
    }

    double DIConfig::GetRandManhattanDistance(void) const {
        return rand_manhattan_distance_;
    }

    double DIConfig::GetRandOutDegree(void) const {
        return rand_out_degree_;
    }
    
    bool DIConfig::IsPlotKML(void) const {
        return plot_kml_;
    }

    DIConfig::Ptr DIConfig::ConfigFromFile(const std::string& config_file_path) {
        std::ifstream file(config_file_path);
        DIConfig::Ptr config_ptr;

        if (file.fail()) {
            throw std::invalid_argument("Could not open configuration file: " + config_file_path);
        }

        config_ptr = ConfigFromStream(file);

        file.close();

        return config_ptr;
    }

    DIConfig::Ptr DIConfig::ConfigFromStream(std::istream& stream) {
        std::string line;                 
        StrVector parts;
        DIConfig::Ptr config_ptr = std::make_shared<DIConfig>();
    
        while (std::getline(stream, line)) {
            line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
            parts = string_utilities::split(line, ':'); 
            
            try {
                if (parts[0] == "mf_fit_ext") {
                    config_ptr->SetFitExt(std::stod(parts[1]));
                } else if (parts[0] == "mf_toggle_scale") {
                    config_ptr->ToggleScaleMapFit(!!std::stoi(parts[1]));
                } else if (parts[0] == "mf_scale") {
                    config_ptr->SetFitExt(std::stod(parts[1]));
                } else if (parts[0] == "n_heading_groups") {
                    config_ptr->SetHeadingGroups(std::stoul(parts[1]));
                } else if (parts[0] == "min_edge_trip_pts") {
                    config_ptr->SetMinEdgeTripPoints(std::stoul(parts[1]));
                } else if (parts[0] == "ta_max_q_size") {
                    config_ptr->SetTAMaxQSize(std::stoul(parts[1]));
                } else if (parts[0] == "ta_area_width") {
                    config_ptr->SetTAAreaWidth(std::stod(parts[1]));
                } else if (parts[0] == "ta_heading_delta") {
                    config_ptr->SetTAAreaWidth(std::stod(parts[1]));
                } else if (parts[0] == "ta_max_speed") {
                    config_ptr->SetTAMaxSpeed(std::stod(parts[1]));
                } else if (parts[0] == "stop_min_distance") {
                    config_ptr->SetStopMinDistance(std::stod(parts[1]));
                } else if (parts[0] == "stop_max_time") {
                    config_ptr->SetStopMaxTime(std::stod(parts[1]));
                } else if (parts[0] == "stop_max_speed") {
                    config_ptr->SetStopMaxSpeed(std::stod(parts[1]));
                } else if (parts[0] == "min_direct_distance") {
                    config_ptr->SetMinDirectDistance(std::stod(parts[1]));
                } else if (parts[0] == "min_manhattan_distance") {
                    config_ptr->SetMinManhattanDistance(std::stod(parts[1]));
                } else if (parts[0] == "min_out_degree") {
                    config_ptr->SetMinOutDegree(std::stoul(parts[1]));
                } else if (parts[0] == "max_direct_distance") {
                    config_ptr->SetMaxDirectDistance(std::stod(parts[1]));
                } else if (parts[0] == "max_manhattan_distance") {
                    config_ptr->SetMaxManhattanDistance(std::stod(parts[1]));
                } else if (parts[0] == "max_out_degree") {
                    config_ptr->SetMaxOutDegree(std::stoul(parts[1]));
                } else if (parts[0] == "rand_direct_distance") {
                    config_ptr->SetRandDirectDistance(std::stod(parts[1]));
                } else if (parts[0] == "rand_manhattan_distance") {
                    config_ptr->SetRandManhattanDistance(std::stod(parts[1]));
                } else if (parts[0] == "rand_out_degree") {
                    config_ptr->SetRandOutDegree(std::stod(parts[1]));
                } else if (parts[0] == "quad_sw_lat") {
                    config_ptr->SetQuadSWLat(std::stod(parts[1]));
                } else if (parts[0] == "quad_sw_lng") {
                    config_ptr->SetQuadSWLng(std::stod(parts[1]));
                } else if (parts[0] == "quad_ne_lat") {
                    config_ptr->SetQuadNELat(std::stod(parts[1]));
                } else if (parts[0] == "quad_ne_lng") {
                    config_ptr->SetQuadNELng(std::stod(parts[1]));
                } else if (parts[0] == "plot_kml") {
                    config_ptr->TogglePlotKML(!!std::stoi(parts[1]));
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

    void DIConfig::PrintConfig(std::ostream& stream) const {
        stream << "*********************************** Configuration ****************************************" << std::endl;
        stream << "Quad SW latitude: " << quad_sw_lat_ << std::endl;
        stream << "Quad SW longitude: " << quad_sw_lng_ << std::endl;
        stream << "Quad NE latitude: " << quad_ne_lat_ << std::endl;
        stream << "Quad NE longitude: " << quad_ne_lng_ << std::endl;
        stream << "Fit extension: " << fit_ext_  << std::endl;
        stream << "Scale map fit: " << scale_map_fit_  << std::endl;
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
        stream << "Plot KML: " << plot_kml_ << std::endl;
        stream << "*****************************************************************************************" << std::endl;
    }
}
