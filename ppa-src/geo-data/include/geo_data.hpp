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
#ifndef GEO_DATA_HPP
#define GEO_DATA_HPP

#include <fstream>
#include <string>
#include <unordered_map>

#include <rapidjson/document.h>
#include <pqxx/pqxx>

#include "geo.hpp"

/**
 * \brief Routines and data structures for reading, writing, and displaying geospatial data.
 */
namespace geo_data {

/** \brief Header for CSV OSM road network file. */
const std::string kRoadCSVHeader = "gid,source,target,osm_id,reverse,class_id,priority,maxspeed_forward,maxspeed_backward,width,excluded,geom_string,valid,error";
/** \brief Number of fields (columns) in the CSV OSM road network .*/
const uint32_t kRoadCSVNumFields = 14;
/** \brief Header for BSMP1 CSV trace file. */
const std::string kTraceCSVHeader = "RxDevice,FileId,TxDevice,Gentime,TxRandom,MsgCount,DSecond,Latitude,Longitude,Elevation,Speed,Heading,Ax,Ay,Az,Yawrate,PathCount,RadiusOfCurve,Confidence";
/** \brief Number of fields in the BSMP1 CSV trace file. */
const uint32_t kTraceCSVNumFields = 19;

using OSMConfigMap = std::unordered_map<int, std::tuple<float, int, double, bool>>;

/**
 * \brief Base class for reading geo::Road OSM road networks.  
 */
class RoadReader {
    public:
        using Ptr = std::shared_ptr<RoadReader>;

        virtual geo::Road::Ptr next_road(void) = 0;
};

/*
 * \brief Construct a mapping of OSM way IDs to their configuration. The configuration file type is JSON.
 * 
 * \param config_file_path JSON file with way configuration
 * \return The mapping of OSM way IDs to their configurations
 */
OSMConfigMap osm_config_map(const std::string& config_file_path);

/** 
 * \brief Postgis (postgres) RoadReader. Reads roads from the postgis OSM bfmap_ways table of the database.
 */
class PostGISRoadReader : public RoadReader {
    public:
        /**
         * \brief Construct a PostGISRoadReader object.
         * 
         * \param host the address or FQDN of the postgres host
         * \param port the port the where the postgres server is running
         * \param database the name of OSM road network database
         * \param user the name of the postgres user
         * \param password the password for the postgres user 
         * \param osm_config_map configuration for OSM ways
         * \throws std::invalid_argument if the psotgres connection could not be established or the query fails
         */
        PostGISRoadReader(const std::string& host, uint32_t port, const std::string& database, const std::string& user, const std::string& password, const OSMConfigMap& osm_config_map_);

        /**
         * \brief Destruct a PostGISRoadReader object.
         */
        ~PostGISRoadReader();
        
        /**
         * \brief Get the next road from the OSM bfmap_ways table query.
         * 
         * \return a pointer to geo::Road or nullptr if there are no more roads
         */
        geo::Road::Ptr next_road(void);
    private:
        using ConnPtr = std::shared_ptr<pqxx::connection>;
        using ResultPtr = std::shared_ptr<pqxx::result>;

        const std::string kWayQueryPrefix = "SELECT gid,osm_id,class_id,source,target,length,reverse,maxspeed_forward,maxspeed_backward,priority, ST_AsBinary(geom) as geom FROM bfmap_ways";

        ResultPtr execute(const std::string& query_string);

        ConnPtr conn_ptr_ = nullptr;
        ResultPtr result_set_ = nullptr;
        geo::Spatial spatial_;
        pqxx::result::size_type result_index_;
        OSMConfigMap osm_config_map_;
};

/**
 * \brief CSV RoadReader. Reads roads from a CSV file.
 */
class CSVRoadReader : public RoadReader {
    public:
        /**
         * \brief Construct a CSV RoadReader object.
         * 
         * \param file_path the CSV file
         * \throws std::invalid_argument if the file does not exist or is empty 
         */ 
        CSVRoadReader(const std::string& file_path);
    
        /**
         * \brief Destruct a CSV RoadReader object.
         */
        ~CSVRoadReader();

        /**
         * \brief Get the next road from the CSV file.
         * 
         * \return a pointer to geo::Road or nullptr if there are no more roads
         */
        geo::Road::Ptr next_road(void);
    private:
        std::ifstream in_file_;
        uint32_t n_lines_ = 0;
        bool error_ = false;
};

/** 
 * \brief A CSV RoadWriter. Write roads to a CSV file. Overwrites the existing file.
 */
class CSVRoadWriter {
    public:
        /**
         * \brief Construct a CSV RoadWriter object. 
         * 
         * \param file_path the CSV file
         * \throws std::invalid_argument if the file can not be opened 
         */
        CSVRoadWriter(const std::string& file_path);
        
        /**
         * \brief Destruct a CSV RoadWriter.
         */
        ~CSVRoadWriter();

        /**
         * \brief Write a road to the CSV file.
         * 
         */
        void write_road(geo::Road& road);

    private:
        std::ofstream out_file_;
        bool error_ = false;
};

using Index = std::size_t;

/**
 * \brief An integer-based interval class to be used with trajectories.
 */
class Interval {
    public:
        using Ptr = std::shared_ptr<Interval>;
        using PtrList = std::vector<Ptr>;
        using AuxSet = std::unordered_set<std::string>;
        using AuxSetPtr = std::shared_ptr<AuxSet>;

        /**
         * \brief Construct the interval [left,right]
         *
         * \param left the lower bound  of the interval
         * \param right the upper bound of the interval
         * \param aux a description of the interval
         * \param id a type identifier for the interval
         */
        Interval( Index left, Index right, std::string aux = "", uint32_t type = 0);

        /**
         * \brief Construct the interval [left,right]
         *
         * \param left the infimum of the interval
         * \param right the supremum of the interval
         * \param aux a description of the interval
         * \param id a type identifier for the interval
         */
        Interval( Index left, Index right, AuxSetPtr aux_set_ptr, uint32_t type = 0);

        /**
         * \brief Get the interval type.
         *
         * \return the ID of the interval
         */
        uint32_t type() const;

        /**
         * \brief Get the upper bound of the interval.
         *
         * \return the interval upper bound
         */
        Index right() const;

        /**
         * \brief Get the lower bound of the interval.
         *
         * \return the interval lower bound.
         */
        Index left() const;

        /**
         * \brief Get the auxiliary data set as a pointer.
         *
         * \return the set of strings that represent the auxiliary data
         */
        AuxSetPtr aux_set() const;

        /**
         * \brief Get the auxiliary data set as a single semicolon delimited string.
         *
         * \return the auxiliary data set as one string
         */
        const std::string aux_str(void) const;

        /**
         * \brief Predicate: interval upper bound <= value 
         *
         * \return true if interval upper bound <= value
         */
        bool is_before( Index value ) const;

        /**
         * \brief Predicate: value in [lower bound, upper bound); this check uses a right open interval.
         *
         * \return true if lower bound <= value < upper bound, false otherwise
         */
        bool contains( Index value ) const;

        /**
         * \brief Write the string form (id = x [lower bound, upper bound) types: { <aux data> }) of an Interval to
         * an output stream.
         *
         * \param os the output stream
         * \param interval the Interval to write
         *
         * \return the output stream
         */
        friend std::ostream& operator<< ( std::ostream& os, const Interval& interval );

    private:
        Index left_;
        Index right_;
        AuxSet aux_set_;
        uint64_t type_;
};

/**
 * \brief Error types for Samples.
 */
enum class SampleError {
    NONE = 0,
    FIELD = 1,
    GEO = 2,
    HEADING = 3
};

/**
 * BSMP1 connected vehicle trace Sample object. Samples represent a spatial-temporal point created by an on-board device. Samples are part of a GPS sequence trace, which represents a self contained vehicle trajectory. The underlying GPS point 
 * is contained within an OGRPoint object.
 * 
 * The record for a sample is a CSV line. The record must have the correct format, number of fields, and unit types. Although only a few fields are used in the sample, the entire record must be correct to avoid flagging the record (and sample) as invalid. Invalid records have missing fields, invalid GPS, or values that are not convertible to the required type.
 * 
 * Samples have associated road edges. There are two types these edges, matched and fit. A matched edge is the edge to which the sample is map matched. A fit edge is edge to which the sample's GPS point falls within the geo::Area of the edge. If a sample has a matched edge and falls with the area of that edge, then that edge is also the sample's fit edge. If the sample does not have a matched edge or does not fall within the area of its matched edge, then the sample will have an implicit fit edge. 
 *
 * Samples have associated intervals marking the sample as critical or private.
 * 
 * Samples also store the accumulated out degree of the trace. This will be zero at the beginning of the trace and the maximum out degree at the end of the trace.
 */
class Sample {
    public:
        using Ptr = std::shared_ptr<Sample>;
        using Trace = std::vector<Ptr>;
    
        /**
         * \brief Construct a Sample object. A sample is valid if it was created successfully from the CSV record. Invalid records have missing fields, invalid GPS, or values that are not able to be converted to the required type.
         * 
         * \param id unique ID of this sample
         * \param index index of this sample within the trace
         * \param timestamp Unix timestamp as seconds
         * \param lat the decimal GPS latitude of the sample
         * \param lon the decimal GPS longitude of the sample
         * \param azimuth the azimuth (heading or bearing) in degrees from north
         * \param speed the instantaneous velocity in meters per second of the vehicle for this sample 
         * \param record the raw CSV record for this sample
         * \param is_valid flag indicating the sample is valid
         * \param error_type indicates SampleError type of the sample
         * \param error_msg message for the sample
         */
        Sample(const std::string& id, std::size_t index, long timestamp, double lat, double lon, double azimuth, double speed, const std::string& record, bool is_valid = true, SampleError error_type = SampleError::NONE, const std::string& error_msg = "");

        /**
         * \brief Get the ID of this sample.
         * 
         * \return the ID
         */
        std::string id() const;

        /**
         * \brief Get the index of this sample in the trace. Because sample's can be re-indexed due to errors and filtering, this might differ from the sample's index in the CSV file.
         * 
         * \return the index
         */
        std::size_t index() const;

        /**
         * \brief Get the raw index of this sample in the trace. The raw index is the initial index of sample in the CSV file, irrespective of the filtering or errors.
         * 
         * \return the raw index
         */
        std::size_t raw_index() const;

        /**
         * \brief Get the Unix timestamp of this sample in seconds. 
         * 
         * \return the timestamp
         */
        long timestamp() const;
    
        /**
         * \brief Get the GPS latitude of this sample in decimal degrees.
         * 
         * \return the latitude
         */
        double lat() const;

        /**
         * \brief Get the GPS of this sample in decimal degrees.
         * 
         * \return the longitude
         */
        double lon() const;
        
        /**
         * \brief Get azimuth (header or bearing) in degrees from north of the sample.
         * 
         * \return the azimuth
         */
        double azimuth() const;

        /**
         * \brief Get the speed in meters per second of the sample.
         * 
         * \return the speed
         */
        double speed() const;

        /**
         * \brief Get the underlying CSV record for this sample.
         * 
         * \return the record
         */
        std::string record() const;

        /**
         * \brief Return true if sample is valid. A sample is valid if it was created successfully from the CSV record. Invalid records have missing fields, invalid GPS, or values that are not convertible to the required type.
         * 
         * \return true if the sample is valid, false otherwise
         */
        bool is_valid() const;

        /**
         * \brief Get the SampleError type for the sample.
         * 
         * \return the sample error type
         */
        SampleError error_type() const;

        /**
         * \brief Get the error message of the sample.
         * 
         * \return the error message
         */
        std::string error_msg() const;
    
    
        /**
         * \brief Get the matched edge for the sample.
         * 
         * \return a pointer the matched edge
         */
        geo::Edge::Ptr matched_edge() const;

        /**
         * \brief Get the fit edge for the sample.
         * 
         * \return a pointer to the fit edge
         */
        geo::Edge::Ptr fit_edge() const;
        
        /**
         * \brief Return true if this sample is map matched and falls within the area of the mapped edge.
         * 
         * \return true if explicitly fit, false otherwise
         */
        bool is_explicit_fit() const;

        /**
         * \brief Get the interval (critical or private) of this sample.
         * 
         * \return a pointer to the interval 
         */
        Interval::Ptr interval() const;

        /**
         * \brief Get the cumulative trace out degree of the trace at this sample.
         * 
         * \return the out degree
         */
        uint32_t out_degree() const;

        /**
         * \brief Set the matched edge of this sample.
         * 
         * \param pointer to the matched edge
         */
        void matched_edge(geo::Edge::Ptr edge);

        /**
         * \brief Set the fit edge of this sample.
         * 
         * \param pointer to the fit edge
         */
        void fit_edge(geo::Edge::Ptr edge);

        /**
         * \brief Set the sample to be explicitly fit.
         * 
         * \param flag for explicitly fit
         */
        void is_explicit_fit(bool explicit_fit);
    
        /**
         * \brief Set the interval (private or critical) for the sample.
         * 
         * \param pointer the interval 
         */
        void interval(Interval::Ptr interval); 

        /**
         * \brief Set the cumulative trace out degree of the sample.
         * 
         * \param the out degree
         */
        void out_degree(uint32_t out_degree);

        /**
         * \brief Set the index of the sample.
         * 
         * \param the new index of the sample
         */
        void index(std::size_t index);

        /** \brief The underlying OGRPoint for the GPS coordinates of the sample. */
        OGRPoint point;

    private:
        std::string id_;
        std::size_t index_;
        std::size_t raw_index_;
        long timestamp_;
        double azimuth_;
        double speed_;
        std::string record_;
        bool is_valid_;
        std::string error_msg_;
        SampleError error_type_;
        

        geo::Edge::Ptr matched_edge_;
        geo::Edge::Ptr fit_edge_;
        bool is_explicit_fit_;
        Interval::Ptr interval_;
        uint32_t out_degree_;
};

/**
 * \brief Make a trace of BSMP1 CSV connected vehicle trace Sample object from a CSV object.
 * 
 * \param input_file the input file
 * \return the sample trace (std::vector of Samples)
 */
Sample::Trace make_trace(const std::string& input_file);

/**
 * \brief Remove invalid samples from the sample trace and re-index the samples of the new trace.
 * 
 * \param trace the raw trace with invalid samples
 * \param out the new trace with invalid samples removed
 */
void remove_trace_errors(const Sample::Trace& trace, Sample::Trace& out);

/**
 * \brief Label each point in a trip with the interval that contains it.
 */
class IntervalMarker {
    public:
        /**
         * \brief Constructor
         *
         * \param list the list of intervals to use for marking the trajectory
         * \param interval_type the type of the interval (critical or private)
         */
        IntervalMarker( const std::initializer_list<Interval::PtrList> list, uint32_t interval_type );

        /**
         * \brief Annotate each sample in a trace with the interval containing it. All samples may not be
         * associated with an interval.
         *
         * \param trace the trace to annotate
         */
        void mark_trace( Sample::Trace& trace ); 

    private:
        uint32_t interval_type_;
        Interval::PtrList intervals;
        
        uint32_t critical_interval;
        Interval::Ptr iptr;

        void merge_intervals( const std::initializer_list<Interval::PtrList> list );
        void mark_trip_point( geo_data::Sample::Ptr sample );
        void set_next_interval();
        static bool compare( Interval::Ptr a, Interval::Ptr b );
};

/**
 * KML Color Values are specified as follows:
 * color_value is easiest to express in hex: 0xaabbggrr
 * where aa -> is the transparency alpha. 00 is transparent; ff is fully opaque.
 * where bb -> BLUE
 * where gg -> GREEN
 * where rr -> RED
 * This is backwards from how it is usually specified, i.e., RGB.
 */

/**
 * \brief A KML file.
 */
class KML {
    public:
        using StreamPtr = std::shared_ptr<std::ostream>;
        static const double MAX_SPEED;
    
        /**
         * \brief Construct a KML object.
         *
         * \param stream the output stream to write the KML
         * \param doc_name the name of the KML document
         * \param visibility when true elements will be rendered when the KML loads
         */
        KML(std::ostream& stream, const std::string& doc_name, bool visibility = true);

        /**
         * \brief Finalize the KML file (write the ending tags).
         */
        void finish();

        /**
         * \brief Write a style for a line to the KML output stream.
         *
         * \param name the style ID
         * \param color_value the KML color value as an integer (will be converted to hex)
         * \param width the width of the line in pixels
         */
        void write_line_style( const std::string& name, unsigned int color_value, int width );

        /**
         * \brief Write a style for an icon to the KML output stream.
         *
         * \param name the style ID
         * \param href the URL for the icon graphic
         * \param scale the size adjustment of the icon relative to its original size
         */
        void write_icon_style( const std::string& name, const std::string& href, float scale = 1.0 );

        /**
         * \brief Write a style for a polygon to the KML output stream.
         *
         * \param name the style ID
         * \param color_value the KML color value as an integer (will be converted to hex)
         * \param width the width of the line in pixels
         * \param fill true indicates the polygon should be color filled
         * \param outline true indicates the polygon should be outlined
         */
        void write_poly_style( const std::string& name, unsigned int color_value, int width = 2, bool fill = true, bool outline = true );

        /**
         * \brief Write a single point to the KML output stream using the point instance.
         *
         * \param point the point to represent in KML
         * \param style_name the ID of the style for the point
         */
        void write_point( const OGRPoint& point, const std::string& style_name );

        /**
         * \brief Write a rendering of a trajectory to the KML output stream considering speed and optionally special intervals. 
         * The rendering colors points based on the speed of the traveler. Privacy interval points and critical
         * interval points can be excluded.
         *
         * \param trace the trace to base the line on
         * \param de_identify flag to suppress the privacy and critical intervals
         * \param stride the number of trajectory points to skip while rendering the line as a sequence of segments
         */
        void write_trace( const Sample::Trace& trace, bool de_identify = false, int stride = 20 );

        /**
         * \brief Write the critical and privacy intervals to the KML output stream.
         *
         * The intervals contain indexes into the provided trajectory.
         *
         * \param intervals the list of intervals to render in the KML
         * \param trace the trace that generated the intervals
         * \param stylename the style ID for the interval trip points
         * \param marker_style the style ID to use for the first point in the interval (highlights the interval type
         * or what triggered it)
         * \param stride the number of trip points to skip to minimize the amount of data rendered
         */
        void write_intervals( const Interval::PtrList& intervals, const Sample::Trace& trace, const std::string& stylename, const std::string& marker_style, int stride = 10 );

        /**
         * \brief Write the critical and privacy interavls to the KML output stream.
         *
         * The intervals contain indexes into the provided trajectory. This method does NOT mark the first point in
         * the interval.
         *
         * \param intervals the list of intervals to render in the KML
         * \param traj the trajectory that generated the intervals
         * \param stylename the style ID for the interval sample points
         * \param stride the number of trip points to skip to minimize the amount of data rendered
         */
        void write_intervals( const Interval::PtrList& intervals, const Sample::Trace& trace, const std::string& stylename, int stride = 10 );

        /**
         * \brief Write a set of geo::Area objects to the KML output stream.
         *
         * \param aptrset the set of areas to render
         * \param stylename the style to use for the polygon rendering
         */
        void write_areas( const std::unordered_set<geo::Area::Ptr>& aptrset, const std::string& stylename );

        /**
         * \brief Write a vector of geo::Area objects to the KML output stream.
         *
         * \param areas the set of areas to render
         * \param stylename the style to use for the polygon rendering
         */
        void write_areas( const std::vector<geo::Area::Ptr>& areas, const std::string& stylename );

    private:
        std::ostream& stream_;
        std::vector<unsigned int> colors;

        unsigned int get_speed_color( double speed );
        void start_folder( const std::string& name, const std::string& description, const std::string& id, const bool open );
        void stop_folder();
};

}

#endif
