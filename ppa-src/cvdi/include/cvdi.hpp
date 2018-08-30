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
#ifndef CVDI_HPP
#define CVDI_HPP

#include <deque>
#include <memory>
#include <unordered_set>
#include <vector>

#include "geo.hpp"
#include "geo_data.hpp"

/**
 * \brief Connected vehicle de-identification routines.
 */
namespace cvdi {

constexpr uint32_t kCriticalIntervalType = 1;
constexpr uint32_t kPrivacyIntervalType = 2;

/**
 * \brief Annotate a trace with fit area edges for each edge.
 */
class AreaFitter {
    public:
        using AreaSet = std::unordered_set<geo::Area::Ptr>;

        /**
         * \brief Construct an Area Fitter object.
         *
         * \param num_sectors the compass rose is divided into this number of equally sized sectors
         * \param min_fit_points use no less than this many points to infer a road segment
         */
         AreaFitter( double fit_width_scaling = 1.0, double fit_extension = 5.0, uint32_t num_sectors = 36, uint32_t min_fit_points = 50 );

        /**
         * \brief Fit a sample to a road area.
         *
         * \param sample pointer to a geo_data::Sample
         */
        void fit( geo_data::Sample::Ptr sample );

        /**
         * \brief Fit an entire trace to road areas.
         *
         * \param trace the trace to fit
         */
        void fit( geo_data::Sample::Trace& trace );

    private:
        using EdgeSet = std::unordered_set<geo::Edge::Ptr>;

        double fit_width_scaling_;
        double fit_extension_;

        long next_edge_id;                          ///< The UID to use for the next implicit edge.
        uint32_t num_sectors;                           ///< The number of divisions of the compass rose to determine when to change implicit edge.
        double sector_size;                             ///< Size in degrees of a sector.
        uint32_t min_fit_points;                        ///< Use no less than this many points to infer a road segment.

        // state vars.
        uint32_t current_sector;                        ///< The compass rose sector the trace is currently in (between 0 and num_sectors - 1)
        uint32_t num_fit_points;                        ///< number of points implicitly fit so far.
        geo::Edge::Ptr current_implicit_edge_;                     ///< The implicit edge currently being "built."
        geo::Edge::Ptr current_matched_edge_;                     ///< The implicit edge currently being "built."
        geo::Area::Ptr current_matched_area_;
        geo_data::Sample::Ptr implicit_edge_start_;
        geo_data::Sample::Ptr implicit_edge_end_;

        uint32_t get_sector( geo_data::Sample::Ptr sample ) const;
        bool is_edge_change( uint32_t heading_group ) const;

    public:
        EdgeSet implicit_edge_set;                      ///< The complete set of implicit edges.
        EdgeSet explicit_edge_set;                      ///< The complete set of implicit edges.
        AreaSet implicit_area_set;                               ///< The complete set of areas built from the implicit edges.
        AreaSet explicit_area_set;                               ///< The complete set of areas built from the implicit edges.
};

/**
 * \brief Annotate a trace with a cumulative count of intersection out degree.
 */
class IntersectionCounter {
    public:

        /**
         * \brief Construct an IntersectionCounter object.
         */
        IntersectionCounter();

        /**
         * \brief Annotate trace with its cumulative intersection out degree counts.
         * 
         * \param trace the trace to annotate
         */
        void count_intersections( geo_data::Sample::Trace& trace );

    private:
        geo::Edge::Ptr current_eptr;
        bool is_last_edge_;
        long last_edge_;
        
        uint32_t cumulative_outdegree;

        uint32_t current_count( geo_data::Sample::Ptr sample );
        
        uint32_t edge_out_degree(geo::Edge::Ptr edge);

};

/**
 * \brief A class that detects turnaround behavior within a trace.
 */
class TurnAround
{
    public:
        using AreaIndexPair = std::pair<geo::Area::Ptr, geo_data::Index>;
        using AreaSet = std::unordered_set<geo::Area::Ptr>;

        /**
         * \brief Construct a Turnaround Detector.
         *
         * \param max_q_size the maximum number of edges to retain for detecting a turnaround
         * \param area_width the width of boxes that encapsulate edges (in meters)
         * \param max_speed trip point speed below max_speed is a turnaround property
         * \param heading_delta heading changes greater that heading_delta is one turnaround property
         */
        TurnAround( size_t max_q_size = 20, double area_width = 30.0, double max_speed = 13.4112, double heading_delta = 90.0 );

        /**
         * \brief Detect all turnarounds in a trace and return a list of intervals where those turnarounds
         * exist.
         *
         * \param trace the trace where turnarounds should be found
         *
         * \return a shared pointer to a list of intervals; each interval is where a turnaround was detected
         */
        geo_data::Interval::PtrList& find_turn_arounds( const geo_data::Sample::Trace& trace );

    private:
        size_t max_q_size;
        double area_width;
        double max_speed;
        double heading_delta;
        bool is_previous_trip_point_fit;
        geo_data::Sample::Ptr fit_exit_point;
        bool is_fit_exit;

        std::deque<std::shared_ptr<AreaIndexPair>> area_q;
        geo::Edge::Ptr current_edge;                             ///> the current edge that the trace is fit to.

        geo_data::Interval::PtrList interval_list;

        void update_turn_around_state( geo_data::Sample::Ptr sample );
        bool is_critical_interval( geo_data::Sample::Ptr sample );

    public:
        AreaSet area_set;                                       ///> the set of areas around implicit edges where turn around behavior occurs.
};

/**
 * \brief A class that detects stop behavior within a trace.
 */
class Stop
{
    public:
        using HighwaySet = std::unordered_set<long>;
    

        /**
         * \brief A special deque with support for detecting stops in trace.
         */
        class Deque
        {
            public:
                friend class Stop;     // so Stop can see private members (state variables) of Deque.
                using size_type = std::deque<geo_data::Sample::Trace::const_iterator>::size_type;

                /**
                 * \brief Construct a Stop Deque object.
                 *
                 * \param detector a reference to the Stop instance to support
                 */
                Deque( Stop& detector );

                /**
                 * \brief Get the point-to-point distance in meters covered by points in the Stop Deque. 
                 *
                 * \return the distance in meters
                 */
                double delta_distance() const;

                /**
                 * \brief Get the distance between the first point and last point in the Stop Deque.
                 *
                 * \return the distance in meters
                 */
                double cover_distance() const;

                /**
                 * \brief Add a point (iterator) to the back (right) of the deque and update the point-to-point
                 * distance.
                 *
                 * \param it An iterator to a trace point
                 */
                void push_right( const geo_data::Sample::Trace::const_iterator& it );

                /**
                 * \brief Remove iterators (points) from the front of the deque until the following conditions are met:
                 *
                 * 1.  The distance covered in the deque is under the limit.
                 * 2.  The deque does not become empty.
                 * 3.  The velocity is below the limit and the road is not in the blacklist.
                 *
                 * \return true if the deque is empty (stop condition); false if it is not empty
                 */
                bool unwind();

                /**
                 * \brief Reset this Stop Deque instance. Remove all points (iterators) from the deque and reset
                 * cumulated distance to 0.
                 */
                void reset();

                /** \brief Predicate indicating whether the time elapsed from the first trip point in the deque to
                 * this trip point does not exceed max_time.
                 *
                 * \param sample a point in a trace; this point is later in the trip than the first point in the deque.
                 *
                 * \return true if the time difference is less than or equal to the max_time.
                 */
                bool under_time( geo_data::Sample::Ptr sample ) const; 

                /**
                 * \brief Predicate indicating whether the distance covered by the points in the deque is less than 
                 * the parameterized limit.
                 *
                 * Cover distance is used since GPS error can greatly skew the Manhattan distance in instances where no actual
                 * movement has taken place.  In cases where the driver is traveling in a straight line, both cover_distance and
                 * Manhattan distance will be equivalent.
                 *
                 * \return true if cover distance <= limit, false otherwise.
                 */
                bool under_distance() const; 
                
                /**
                 * \brief Predicate that indicates the speed in this point record is less than the limit.
                 *
                 * Points having speeds greater than or equal to this speed are not considered by the stop detector.
                 *
                 * \param sample sample point in a trace
                 * \return true if the points current speeds is less than the parameterized max speed.
                 */
                bool under_speed( geo_data::Sample::Ptr sample ) const;

                /**
                 * \brief Output to a stream details about this Stop Deque.
                 *
                 * \param os the output stream
                 * \param q the Stop Deque
                 * \return the output stream after printing for chaining
                 */
                friend std::ostream& operator<<( std::ostream& os, const Deque& q );

            private:
                Stop& stop_detector;                    ///< reference to stop to access parameters.
                std::deque<geo_data::Sample::Trace::const_iterator> q;    ///< deque of iterators to points where between them there is distance.
                geo::Spatial spatial_;
                double cumulative_distance;             ///< cumulative distance across points in the deque.

                geo_data::Sample::Trace::const_iterator pop_left();

                size_type length() const;

                uint64_t delta_time() const;

                size_type left_index() const;

                size_type right_index() const;
        };

    private:
        static HighwaySet               excluded_highways;
        uint64_t                        max_time;             ///< in this implementation this is in microseconds and it is converted during construction.
        double                          min_distance;
        double                          max_speed;
        geo::Spatial spatial_;

        geo_data::Interval::PtrList   critical_intervals;

    public:
        friend class Deque;

        /**
         * \brief Set the OSM highway types where stops should be ignored.
         *
         * This static method will clear the set first.
         *
         * \param excludes a constant set of OSM highway types
         * \return the size of the set to ignore
         */
        static geo_data::Index set_excluded_highways( const HighwaySet& excludes );

        /**
         * \brief Add an OSM highway type to the current exclude set.
         *
         * \param hw an OSM highway to add to the excluded set
         * \return the size of the set to ignore
         */
        static geo_data::Index add_excluded_highway( long highway );

        /**
         * \brief Predicate that indicates this point should be considered for stop detection.
         *
         * \param sample sample trace point.
         * \return true if the road the point is fit to is IMPLICIT (unknown) or it is NOT in the blacklist
         */
        static bool valid_highway( const geo_data::Sample::Ptr sample);

        /**
         * \brief Construct a stop detector.
         *
         * Stop detectors find stops using distance and time, not by speed alone.  If speed exceeds a certain
         * threshold OR it is on an excluded highway type, it will not be considered for stop detection.
         *
         * \param max_time (seconds) the maximum loiter time considered as a stop
         * \param min_distance (meters) stops not cover more than this distance
         * \param max_speed (meters/second) only consider trip points whose speed is less than max speed for stops
         */
        Stop( uint64_t max_time = 120, double min_distance = 15.0, double max_speed = 2.5);

        /**
         * \brief Find the critical intervals in the trace that exhibit stop behavior.
         *
         * \param trace the trace to review for stops
         * \return a list of pointers to intervals; the intervals capture the stop critical intervals
         */
        geo_data::Interval::PtrList& find_stops( const geo_data::Sample::Trace& trace );
};


/**
 * \brief A method that builds two intervals, one for the beginning point of a trip and one for the end point of a trip.
 */
class StartEndIntervals
{
    public:
        /**
         * \brief Return a list of two intervals that define the start - end trace critical intervals.  Each
         * interval has one point.
         *
         * \param trace The trace to pull the intervals from
         * \return a length 2 list of pointers to the start and end trip critical intervals
         */
        const geo_data::Interval::PtrList& get_start_end_intervals( const geo_data::Sample::Trace& trace );

    private:
        geo_data::Interval::PtrList intervals;            ///< The start and end trip critical intervals.
};

/**
 * \brief Define the intervals that "hide" critical intervals. Instances of this class determine the portion of a trace extended from the ends
 * of the previously defined critical intervals to protect those locations.  This is done using intersection out degree,
 * direct distance, and Manhattan distance.
 */
class PrivacyIntervalFinder {
    public:
        using TrajectoryIterator = geo_data::Sample::Trace::iterator;
        using RevTrajectoryIterator = geo_data::Sample::Trace::reverse_iterator;

        /**
         * \brief Construct a PrivacyIntervalFinder object.
         *
         * \param min_dd minimum direct distance
         * \param min_md minimum manhattan distance
         * \param min_out_degree minimum intersection out degree
         * \param max_dd maximum direct distance
         * \param max_md maximum manhattan distance
         * \param max_out_degree maximum intersection out degree
         * \param dd_rand direct distance randomization factor
         * \param md_rand manhattan distance randomization factor
         * \param out_degree_rand out degree randomization factor
         */
        PrivacyIntervalFinder( double min_dd, double min_md, uint32_t min_out_degree, double max_dd, double max_md, uint32_t max_out_degree, double dd_rand, double md_rand, double out_degree_rand );

        /**
         * \brief Find the privacy intervals in the provided trace.
         *
         * \param trace the trace to search for privacy intervals
         * \return a list of the intervals that define the privacy intervals
         */
        const geo_data::Interval::PtrList& find_intervals( geo_data::Sample::Trace& trace );

    private:
        double min_dd;
        double min_md;
        uint32_t min_out_degree;
        double max_dd;
        double max_md;
        uint32_t max_out_degree;
        double dd_rand;
        double md_rand;
        double out_degree_rand;
        double rand_min_dd;
        double rand_min_md;
        uint32_t rand_min_out_degree;
        geo_data::Interval::Ptr curr_ciptr;
        geo_data::Sample::Ptr init_priv_point;
        double md;
        uint32_t out_degree;
        geo_data::Index interval_start;
        geo_data::Index last_pi_end;
        geo_data::Interval::PtrList interval_list;
	    TrajectoryIterator curr_tp_it;

        bool is_edge_change(geo::Edge::Ptr a, geo::Edge::Ptr b) const;
        void update_intervals( const TrajectoryIterator tp_it, geo_data::Sample::Trace& trace );
        void find_interval( const TrajectoryIterator start, const TrajectoryIterator end );
        void find_interval( const RevTrajectoryIterator start, const RevTrajectoryIterator end );
        geo_data::Index find_interval_end( const TrajectoryIterator start, const TrajectoryIterator end );
        geo_data::Index find_interval_end( const RevTrajectoryIterator start, const RevTrajectoryIterator end );
        bool handle_edge_change( const TrajectoryIterator curr, const TrajectoryIterator prev, geo::Edge::Ptr eptr );
        bool handle_edge_change( const RevTrajectoryIterator curr, const RevTrajectoryIterator prev, geo::Edge::Ptr eptr );
        geo_data::Interval::Ptr make_interval(geo_data::Index start, geo_data::Index end, const std::string& tag, double dd, double md, uint32_t od);;

        geo::Spatial spatial_;
};

/** \brief Header for point count CSV. */
const std::string kPointCountHeader = "total_points,field_error_points,geo_error_points,heading_error_points,ci_points,pi_points";

/**
 * \brief A class to gather statistics about a trace.
 */
struct PointCounter {
    uint64_t n_points;                      ///> The number of points in a trace.
    uint64_t n_invalid_field_points;        ///> The number of points with invalid field values.
    uint64_t n_invalid_geo_points;          ///> The number of points with invalid position.
    uint64_t n_invalid_heading_points;      ///> The number of points with an invalid heading.
    uint64_t n_ci_points;                   ///> The number of points in critical intervals.
    uint64_t n_pi_points;                   ///> The number of points in privacy intervals.

    /**
     * \brief Default constructor.
     *
     * All statistics are initialized to 0.
     */
    PointCounter();

    /**
     * \brief Constructor with initialization of count values.
     *
     * \param n_points initial number of point value
     * \param n_invalid_field_points initial number of points with invalid fields
     * \param n_invalid_geo_points initial number of points that have invalid GPS coordinates
     * \param n_invalid_heading_points initial number of points that have invalid headings
     * \param n_ci_points initial number of points in a critical interval
     * \param n_pi_points initial number of points in a privacy interval
     */
    PointCounter(uint64_t n_points, uint64_t n_invalid_field_points, uint64_t n_invalid_geo_points, uint64_t n_invalid_heading_points, uint64_t n_ci_points, uint64_t n_pi_points);

    /**
     * \brief Return a new PointCounter instance = this PointCounter + other PointCounter. Values in each
     * PointCounter are added together.
     *
     * This PointCounter is NOT modified.
     *
     * \param other the PointCounter whose value are to be added to this point counter
     * \return a NEW PointCounter instance
     */
    PointCounter operator+( const PointCounter& other) const;
    
    /**
     * \brief Write a PointCounter to an output stream.
     *
     * The form of the output is a comma-delimited list of the various values managed by the PointCounter.
     *
     * \param os the output stream to write the string form of this PointCounter
     * \param point_counter the PointCounter to write
     * \return the output stream after the write
     */
    friend std::ostream& operator<<( std::ostream& os, const PointCounter& point_counter );
};

/**
 * \brief Count the points (samples) for the trace. Counts invalid GPS points, headings, CSV fields, and critical and privacy points.
 * 
 * \param raw_trace trace with invalid samples 
 * \param counter_out the resulting point count
 */
void count_points(const geo_data::Sample::Trace& raw_trace, PointCounter& counter_out);

}

#endif
