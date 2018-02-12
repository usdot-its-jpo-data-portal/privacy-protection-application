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
#ifndef CVDI_H
#define CVDI_H

#include <nan.h>
#include <vector>
#include <string>

#include "cvlib.hpp"

/**
 * A wrapper around a trip file that brings it into memory.
 */
class CSVFactory {
    public:
        // Indices where data fields are found in the CSV.
        int RX_DEVICE;
        int FILE_ID;
        int TX_DEV;
        int GENTIME;
        int TX_RANDOM;
        int MSG_COUNT;
        int DSECONDS;
        int LAT;
        int LON;
        int ELEVATION;
        int SPEED;
        int HEADING;
        int AX;
        int AY;
        int AZ;
        int YAW_RATE;
        int PATH_COUNT;
        int RADIUS_OF_CURVE;
        int CONFIDENCE;
        
        /**
         * \brief Constructor.
         *
         * Determine which data columns corresponds with the fields needed for the algorithm.
         *
         * \param stream an input stream containing trip data.
         * \param header the entire header string if the file contained a header.
         * \param lat_field the name to use to identify the latitude data column in the stream.
         * \param lon_field the name to use to identify the longitude data column in the stream.
         * \param heading_field the name to use to identify the heading data column in the stream.
         * \param speed_field the name to use to identify the speed data column in the stream.
         * \param gentime_field the name to use to identify the time data column in the stream.
         */
        CSVFactory( std::istream& stream, const std::string& header, const std::string& lat_field, const std::string& lon_field, const std::string& heading_field, const std::string& speed_field, const std::string& gentime_field);

        /**
         * \brief Transform a line in the CSV file into a Point data structure. Return a pointer to this structure.
         *
         * \param fileline the CSV file line (trip record) containing the data to use to make the Point.
         * \return A shared pointer to the newly created trip record.
         *
         * \throws out_of_range and invalid_argument when the fileline cannot be parsed.
         */
        trajectory::Point::Ptr make_point( const std::string& fileline );

        /**
         * \brief Fill up traj with records from the wrapped file.
         *
         * \param traj an empty trajectory that will be filled.
         */
        void make_trajectory(trajectory::Trajectory& traj);

        /**
         * \brief Fill up traj with records from the wrapped file starting and ending at a specified byte offset in the
         * file.
         *
         * \param traj An empty trajectory that will be filled.
         * \param start The byte offset in the wrapped file where the first record should be read.
         * \param end The byte offset in the wrapped file where the last record should be read.
         */
        void make_trajectory(trajectory::Trajectory& traj, uint64_t start, uint64_t end );

        /**
         * \brief Write traj to the provided output stream with header being the first line in the stream.
         *
         * \param stream An output stream where the data should be written.
         * \param header The header to use as the first entry into the stream.
         * \param traj The trajectory to write.
         */
        static void write_trajectory( std::ostream& stream, const std::string& header, const trajectory::Trajectory& traj );

        /**
         * \brief Predicate indicating the wrapped file has a header.
         *
         * \return true if this file has a header, false otherwise.
         */

        bool HasHeader() const;

        /**
         * \brief Return a copy of the header string for this trip file.
         *
         * \param A copy of the header string.
         */
        const std::string GetHeader() const;

    private:
        std::istream& stream_;
        std::string header;
        std::string lat_field;
        std::string lon_field;
        std::string heading_field;
        std::string speed_field;
        std::string gentime_field;
        bool has_header;
        uint64_t index;

        /**
         * \brief Determine which data columns correspond with the fields needed for the algorithm.
         *
         * Called in the constructor.
         *
         * If a required field is not present throw and exception; the file cannot be processed.
         */
        void map_index_fields();
};
    
/**
 * A wrapper for a file that contains one or more trips.
 */
class CSVSplitter {
    public:
        using TripLocation = std::tuple<std::string, uint64_t, uint64_t>;       // trip id, start byte offset, end byte offset.
        using TripLocationPtr = std::shared_ptr<TripLocation>;
        using Ptr = std::shared_ptr<CSVSplitter>;

        /**
         * \brief Constructor that explicitly sets the UID indices.
         *
         * \param stream the input stream to split and process.
         * \param uid_indices a vector of the column indices that should be used to determine the UID.
         * \param size the size in bytes of this file.
         * \param has_header flag when true indicates the stream contains a header that should be read.
         * \param delimiter the character delimiter for fields in the file
         */
        CSVSplitter( std::istream& stream, const std::vector<int> uid_indices, uint64_t size, bool has_header, char delimiter = ',' );

        /**
         * \brief Constructor that sets UID fields based on the fields in the string provided.
         *
         * \param stream the input stream to split and process.
         * \param size the size in bytes of this file.
         * \param uid_fields a delimiter delimited string of the fields to use for UID (should be in the header).
         * \param has_header flag when true indicates the stream contains a header that should be read.
         * \param delimiter the character delimiter for fields in the file
         * 
         * \throws invalid_argument if the uid_fields cannot be found in the file.
         */
        CSVSplitter( std::istream& stream, uint64_t size, const std::string& uid_fields, bool has_header, char delimiter = ',' );

        /**
         * \brief Constructor that assumes the first line is the header and RxDevice,FileId are the UID fields.
         *
         * \param stream the input stream to split and process.
         * \param size the size in bytes of this file.
         * \param delimiter the character delimiter for fields in the file
         * 
         * \throws invalid_argument if the uid_fields cannot be found in the file.
         */
        CSVSplitter( std::istream& stream, uint64_t size, char delimiter = ',' );

        /**
         * \brief Constructor that provides a header, UID column names and the file delimiter.
         *
         * \param stream the input stream to split and process.
         * \param size the size in bytes of this file.
         * \param header the header string for this file.
         * \param uid_fields a delimiter delimited string of the fields to use for UID (should be in the header).
         * \param delimiter the character delimiter for fields in the file
         * 
         * \throws invalid_argument if the uid_fields cannot be found in the file.
         */
        CSVSplitter( std::istream& stream, uint64_t size, const std::string& header, const std::string& uid_fields, char delimiter = ',');

        /**
         * \brief Constructor that provides a header and assumes RxDevice,FileId are the UID fields.
         *
         * \param stream the input stream to split and process.
         * \param size the size in bytes of this file.
         * \param header the header for the file.
         * \param delimiter the character delimiter for fields in the file. Default value is ','
         * 
         * \throws invalid_argument if the uid_fields cannot be found in the file.
         */
        CSVSplitter( std::istream& stream, uint64_t size, const std::string& header, char delimiter = ',');

        /**
         * \brief Find the end of the current trip in this input file and return its UID and starting and ending byte
         * offset.
         */
        const TripLocationPtr next_trajectory( );

        /**
         * \brief Return the file's header.
         *
         * \return A reference to a non-modifiable version of the header.
         */
        const std::string& get_header() const;

        /**
         * \brief Build the UID string for a trip using the trip record in line.
         *
         * \param header The header of the trip file being processed.
         * \param line The trip record whose UID is being extracted.
         * \param uid_fields The field names in the header (in order) that define the UID.
         * \param delimiter The character that delimits fields in the trip file.
         * \return The UID of the trip.
         *
         * \throws invalid_argument if the field names for the uid are not found in the header.
         */
        static const std::string get_uid(const std::string& header, const std::string& line, const std::string& uid_fields, char delimiter = ',');

    private:
        std::istream& stream_;
        std::vector<int> uid_indices;
        uint64_t size;
        std::string header;
        std::string uid_fields;
        char delimiter;
        uint64_t start;
        uint64_t end;
        std::string curr_line;
        std::string uid;
        std::string curr_uid;

        /**
         * \brief Get a copy of the uid for the current record in the file.
         *
         * \return a constant copy of the uid for the current line.
         */
        const std::string get_curr_uid() const;

        /**
         * \brief Find the ending byte offset of the current trip.
         *
         * This updates class state variables.
         */
        void find_next_trip();


        /**
         * \brief Determine which data column indices contain the UID fields.
         *
         * This updates class state variables: uid_indices.
         *
         * \throws invalid_argument when a field name cannot be found in the header.
         */
        void map_index_fields();
};

class DIConfig {
    public:
        using Ptr = std::shared_ptr<DIConfig>;

        /**
         * \brief Deidentification configuration constructor.
         */
        DIConfig();
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
        const std::string& GetLatField(void) const;
        void SetQuadSWLat(double quad_sw_lat);
        void SetQuadSWLon(double quad_sw_lon);
        void SetQuadNELat(double quad_ne_lat);
        void SetQuadNELon(double quad_ne_lon);
        const std::string& GetLonField(void) const;
        const std::string& GetSpeedField(void) const;
        const std::string& GetHeadingField(void) const;
        const std::string& GetGentimeField(void) const;
        const std::string& GetUIDFields(void) const;
        double GetFitExt(void) const;
        bool IsPlotKML(void) const;
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
        double GetQuadSWLat(void) const;
        double GetQuadSWLon(void) const;
        double GetQuadNELat(void) const;
        double GetQuadNELon(void) const;

        /**
         * \brief Print out the current configuration values to an output stream.
         *
         * \param stream the output stream where the parameters should be printed.
         */
        void PrintConfig(std::ostream& stream) const;

    private:

        // These settings are based on Ann Arbor Safety Pilot data headers.
        std::string lat_field_ = "Latitude";
        std::string lon_field_ = "Longitude";
        std::string heading_field_ = "Heading";
        std::string speed_field_ = "Speed";
        std::string gentime_field_ = "Gentime";
        std::string uid_fields_ = "RxDevice,FileId";

        // default bounding box around mapped area.
        double quad_sw_lat_             = 42.17;
        double quad_sw_lon_             = -83.91;
        double quad_ne_lat_             = 42.431;
        double quad_ne_lon_             = -83.54; 

        bool plot_kml_                  = false;

        double fit_ext_                 = 5;
        bool scale_map_fit_             = false;
        double map_fit_scale_           = 1.0;
        uint32_t n_heading_groups_      = 36;
        uint32_t min_edge_trip_points_  = 50;

        uint32_t ta_max_q_size_         = 20;
        double ta_area_width_           = 30.0;
        double ta_max_speed_            = 15.0;
        double ta_heading_delta_        = 90.0;

        double stop_max_time_           = 120.0;
        double stop_min_distance_       = 15.0;
        double stop_max_speed_          = 3.0;

        double min_direct_distance_     = 500.0;
        double max_direct_distance_     = 2500.0;
        double min_manhattan_distance_  = 650.0;
        double max_manhattan_distance_  = 3000.0;

        uint32_t min_out_degree_        = 8;
        uint32_t max_out_degree_        = 16;

        double rand_direct_distance_    = 0.0;
        double rand_manhattan_distance_ = 0.0;
        double rand_out_degree_         = 0.0;
};

/**
 * \brief Metadata for a file: size, contains more than one trip, error on opening, and path on file system.
 */
class FileInfo {
    public:
        using Ptr = std::shared_ptr<FileInfo>;

        /**
         * \brief Constructor.
         *
         * \param path the path to the file.
         * \param is_multi flag when true indicates the file contains more than one trip.
         */
        FileInfo(const std::string& path, bool is_multi=false);

        /**
         * \brief Get the file path.
         *
         * \return a reference to a constant string containing the path.
         */
        const std::string& GetPath(void) const;

        /**
         * \brief Predicate indicating this file has multiple trips.
         *
         * \return true if multiple trips, false otherwise.
         */
        bool IsMulti(void) const;

        /**
         * \brief Return the number of bytes in this file.
         *
         * \return the number of bytes in the file.
         */
        uint64_t GetSize(void) const;

        /**
         * \brief Predicate indicating there was a problem opening the file.
         *
         * \return true if the file could not be opened, false otherwise.
         */
        bool IsError(void) const;

    private:
        std::string path_;
        bool is_multi_;
        uint64_t size_;
        bool is_error_;
};

#endif
