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
#ifndef CONFIG
#define CONFIG

#include <iostream>
#include <fstream>
#include <memory>

namespace config {

class Config {
    public:
        using Ptr = std::shared_ptr<Config>;

        /**
         * \brief DeIdentification configuration constructor.
         */
        Config();

        // Getters and setters.
        void SetSaveMM(bool save_mm);
        void SetPlotKML(bool plot_kml);
        void SetCountPoints(bool count_points);
        void SetFitExt(double fit_ext);
        void SetScaleMapFit(bool scale_map_fit);
        void SetMapFitScale(double map_fit_scale);
        void SetHeadingGroups(uint32_t heading_groups);
        void SetMinEdgeTripPoints(uint32_t min_edge_trip_points);
        void SetTAMaxQSize(uint32_t ta_max_q_size);
        void SetTAAreaWidth(double ta_area_width);
        void SetTAMaxSpeed(double ta_max_speed);
        void SetTAHeadingDelta(double ta_heading_delta);
        void SetStopMaxTime(uint64_t stop_max_time);
        void SetStopMinDistance(double stop_min_distance);
        void SetStopMaxSpeed(double stop_max_speed);
        void SetMinDirectDistance(double min_direct_distance);
        void SetMinManhattanDistance(double min_manhattan_distance);
        void SetMinOutDegree(uint32_t min_out_degree);
        void SetMaxDirectDistance(double max_direct_distance);
        void SetMaxManhattanDistance(double max_manhattan_distance);
        void SetMaxOutDegree(uint32_t max_out_degree);
        void SetRandDirectDistance(double rand_direct_distance);
        void SetRandManhattanDistance(double rand_manhattan_distance);
        void SetRandOutDegree(double rand_out_degree);
        void SetKMLStride(uint32_t kml_stride);
        void SetKMLSuppressDI(bool suppress_di);
        bool IsSaveMM(void) const;
        bool IsPlotKML(void) const;
        bool IsCountPoints(void) const;
        double GetFitExt(void) const;
        bool IsScaleMapFit(void) const;
        double GetMapFitScale(void) const;
        uint32_t GetHeadingGroups(void) const;
        uint32_t GetMinEdgeTripPoints(void) const;
        uint32_t GetTAMaxQSize(void) const;
        double GetTAAreaWidth(void) const;
        double GetTAMaxSpeed(void) const;
        double GetTAHeadingDelta(void) const;
        uint64_t GetStopMaxTime(void) const;
        double GetStopMinDistance(void) const;
        double GetStopMaxSpeed(void) const;
        double GetMinDirectDistance(void) const;
        double GetMinManhattanDistance(void) const;
        uint32_t GetMinOutDegree(void) const;
        double GetMaxDirectDistance(void) const;
        double GetMaxManhattanDistance(void) const;
        uint32_t GetMaxOutDegree(void) const;
        double GetRandDirectDistance(void) const;
        double GetRandManhattanDistance(void) const;
        double GetRandOutDegree(void) const;
        uint32_t GetKMLStride(void) const;
        bool IsKMLSuppressDI(void) const;

        /**
         * \brief Print out the current configuration values to an output stream.
         *
         * \param stream the output stream where the parameters should be printed.
         */
        void PrintConfig(std::ostream& stream) const;

        /**
         * \brief Set configuration values using the values in the specified file.
         *
         * The file has one key - value pair per line delimited by a colon.
         * <param name> : <value>
         *
         * <param name> corresponds to the private member names without the trailing '_' character.
         * values that cannot be converted to the correct type will retain their default value.
         * keys that do not correspond to valid parameters will be ignored.
         *
         * \param config_file_path a file containing key value pairs of parameters.
         * \return a shared pointer to the configuration instance.
         */
        static Config::Ptr ConfigFromFile(const std::string& config_file_path);

        /**
         * \brief Set configuration values using the values in the specified input stream.
         *
         * The stream has one key - value pair per line delimited by a colon.
         * <param name> : <value>
         *
         * \param stream an input stream containing key value pairs of parameters.
         * \return a shared pointer to the configuration instance.
         */
        static Config::Ptr ConfigFromStream(std::istream& stream);
        
    private:
        // general options
        bool save_mm_                       = false;
        bool plot_kml_                      = true;
        bool count_points_                  = true;

        // options for each di procedure
        bool map_match_                     = true;
        bool implicit_map_fit_              = true;
        bool intersection_count_            = true;
        bool critical_intervals_            = true;
        bool privacy_intervals_             = true;
        bool output_map_match_              = false;
        bool output_di_traj_                = true;

        uint32_t kml_stride_                = 20;
        bool kml_suppress_di_               = false;

        double fit_ext_                     = 5.0;          // meters.
        bool scale_map_fit_                 = false;        // do not scale boxes based on road type.
        double map_fit_scale_               = 1;
        uint32_t n_heading_groups_          = 36;           // 10 degree sectors.
        uint32_t min_edge_trip_points_      = 50;

        uint32_t ta_max_q_size_             = 20;
        double ta_area_width_               = 30.0;
        double ta_max_speed_                = 15.0;         // meters per second.
        double ta_heading_delta_            = 90.0;

        uint64_t stop_max_time_             = 120;
        double stop_min_distance_           = 15.0;
        double stop_max_speed_              = 3.0;          // meters per second.

        double min_direct_distance_         = 500.0;        // meters.
        double max_direct_distance_         = 2500.0;       // meters.
        double min_manhattan_distance_      = 650.0;        // meters.
        double max_manhattan_distance_      = 3000.0;       // meters.
        uint32_t min_out_degree_            = 8;
        uint32_t max_out_degree_            = 16;

        // Additional random suppression is turned OFF by default.
        double rand_direct_distance_        = 0.0;
        double rand_manhattan_distance_     = 0.0;
        double rand_out_degree_             = 0.0;
};
}

#endif
