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
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "cvlib.hpp"

namespace Config {

    enum class TRACKTYPE : uint32_t { BSMP1 = 0, SHRP2, COUNT };

    class DIConfig {
        public:
            using Ptr = std::shared_ptr<DIConfig>;

            // public for ease of access; used for SHRP2
            uint32_t lat_field_idx = 0;
            uint32_t lon_field_idx = 0;
            uint32_t heading_field_idx = 0;
            uint32_t speed_field_idx = 0;
            uint32_t gentime_field_idx = 0;
            uint32_t uid_field_idx = 0;
            uint32_t num_fields = 19;

            TRACKTYPE tracktype = TRACKTYPE::BSMP1;
            std::string outfile_header = "";

            /**
             * \brief Deidentification configuration constructor.
             */
            DIConfig();

            // Getters and setters.
            void SetQuadSWLat(double quad_sw_lat);
            void SetQuadSWLng(double quad_sw_lng);
            void SetQuadNELat(double quad_ne_lat);
            void SetQuadNELng(double quad_ne_lng);
            void SetLatField(const std::string& lat_field);
            void SetLonField(const std::string& lon_field);
            void SetHeadingField(const std::string& heading_field);
            void SetSpeedField(const std::string& speed_field);
            void SetGentimeField(const std::string& gentime_field);
            void SetUIDFields(const std::string& uid_fields);
            void SetFitExt(double fit_ext);
            void ToggleScaleMapFit(bool scale_map_fit);
            void SetMapFitScale(double map_fit_scale);
            void SetHeadingGroups(uint32_t heading_groups);
            void SetMinEdgeTripPoints(uint32_t min_edge_trip_points);
            void SetTAMaxQSize(uint32_t ta_max_q_size);
            void SetTAAreaWidth(double ta_area_width);
            void SetTAMaxSpeed(double ta_max_speed);
            void SetTAHeadingDelta(double ta_heading_delta);
            void SetStopMaxTime(double stop_max_time);
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
            void TogglePlotKML(bool plot_kml);
            double GetQuadSWLat(void) const;
            double GetQuadSWLng(void) const;
            double GetQuadNELat(void) const;
            double GetQuadNELng(void) const;
            const std::string& GetLatField(void) const;
            const std::string& GetLonField(void) const;
            const std::string& GetSpeedField(void) const;
            const std::string& GetHeadingField(void) const;
            const std::string& GetGentimeField(void) const;
            const std::string& GetUIDFields(void) const;
            double GetFitExt(void) const;
            bool IsScaleMapFit(void) const;
            double GetMapFitScale(void) const;
            uint32_t GetHeadingGroups(void) const;
            uint32_t GetMinEdgeTripPoints(void) const;
            uint32_t GetTAMaxQSize(void) const;
            double GetTAAreaWidth(void) const;
            double GetTAMaxSpeed(void) const;
            double GetTAHeadingDelta(void) const;
            double GetStopMaxTime(void) const;
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
            bool IsPlotKML(void) const;

            TRACKTYPE GetTrackType() const;

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
            static DIConfig::Ptr ConfigFromFile(const std::string& config_file_path);

            /**
             * \brief Set configuration values using the values in the specified input stream.
             *
             * The stream has one key - value pair per line delimited by a colon.
             * <param name> : <value>
             *
             * \param stream an input stream containing key value pairs of parameters.
             * \return a shared pointer to the configuration instance.
             */
            static DIConfig::Ptr ConfigFromStream(std::istream& stream);
            
        private:
            // These settings are based on Ann Arbor Safety Pilot data headers.
            std::string lat_field_ = "Latitude";
            std::string lon_field_ = "Longitude";
            std::string heading_field_ = "Heading";
            std::string speed_field_ = "Speed";
            std::string gentime_field_ = "Gentime";
            std::string uid_fields_ = "RxDevice,FileId";

            // default bounding box around mapped area.
            double quad_sw_lat_                 = 42.17;
            double quad_sw_lng_                 = -83.91;
            double quad_ne_lat_                 = 42.431;
            double quad_ne_lng_                 = -83.54;

            bool plot_kml_                      = false;        // do not create KML files by default.
            double fit_ext_                     = 5.0;          // meters.
            bool scale_map_fit_                 = false;        // do not scale boxes based on road type.
            double map_fit_scale_               = 1;
            uint32_t n_heading_groups_          = 36;           // 10 degree sectors.
            uint32_t min_edge_trip_points_      = 50;

            uint32_t ta_max_q_size_             = 20;
            double ta_area_width_               = 30.0;
            double ta_max_speed_                = 15.0;         // meters per second.
            double ta_heading_delta_            = 90.0;

            double stop_max_time_               = 120.0;
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
