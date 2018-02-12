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
#ifndef CTES_DI_CRITICAL_HPP
#define CTES_DI_CRITICAL_HPP

#include "names.hpp"
#include "entity.hpp"
#include "trajectory.hpp"

#include <deque>

namespace Detector {

    /**
     * \brief A class that detects turnaround behavior within a trajectory (trip).
     */
    class TurnAround
    {
        public:
            using AreaIndexPair = std::pair<geo::Area::Ptr, uint64_t>;
            using AreaSet = std::unordered_set<geo::Area::Ptr>;

            /**
             * \brief Construct a Turnaround Detector
             *
             * \param max_q_size The maximum number of edges to retain for detecting a turnaround.
             * \param area_width The width of boxes that encapsulate edges (in meters)
             * \param max_speed Trip point speed below max_speed is a turnaround property.
             * \param heading_delta Heading changes greater that heading_delta is one turnaround property.
             */
            TurnAround( size_t max_q_size, double area_width, double max_speed, double heading_delta );

            /**
             * \brief Detect all turnarounds in a trajectory and return a list of intervals where those turnarounds
             * exist.
             *
             * \param traj The trajectory (trip) where turnaounds should be found.
             *
             * \return A shared pointer to a list of intervals; each interval is where a turnaround was detected.
             */
            trajectory::Interval::PtrList& find_turn_arounds( const trajectory::Trajectory& traj );

        private:
            size_t max_q_size;
            double area_width;
            double max_speed;
            double heading_delta;
            bool is_previous_trip_point_fit;
            trajectory::Point::Ptr fit_exit_point;
            bool is_fit_exit;

            std::deque<std::shared_ptr<AreaIndexPair>> area_q;
            geo::EdgeCPtr current_edge;                             ///> the current edge that the traj is fit to.

            trajectory::Interval::PtrList interval_list;

            /**
             * \brief Update detector state variables based on the current trip point.
             *
             * \param tp The trip point currently being evaluated.
             */
            void update_turn_around_state( const trajectory::Point::Ptr& tp );

            /**
             * \brief Predicate that indicates whether this trip point is in a turn around critical interval.
             *
             * \param tp The trip point currently being evaluated.
             *
             * \return true if tp is in a critical interval, false otherwise.
             */
            bool is_critical_interval( const trajectory::Point::Ptr& tp );

        public:
            AreaSet area_set;                                       ///> the set of areas around implicit edges where turn around behavior occurs.
    };

    /**
     * \brief A class that detects stop behavior within a trajectory (trip).
     */
    class Stop
    {
        public:

            /**
             * \brief A special Deque with support for detecting stops in trajectories.
             */
            class Deque
            {
                public:
                    friend class Stop;     // so Stop can see private members (state variables) of Deque.
                    using size_type = std::deque<trajectory::CIterator>::size_type;

                    /**
                     * \brief Construct a Stop Deque
                     *
                     * \param detector a reference to the Stop instance to support.
                     */
                    Deque( Stop& detector );

                    /**
                     * \brief Get the point-to-point distance in meters covered by points in the Stop Deque. 
                     *
                     * \return the distance in meters.
                     */
                    double delta_distance() const;

                    /**
                     * \brief Get the distance between the first point and last point in the Stop Deque.
                     *
                     * \return the distance in meters.
                     */
                    double cover_distance() const;

                    /**
                     * \brief Add a point (iterator) to the back (right) of the deque and update the point-to-point
                     * distance.
                     *
                     * \param it An iterator to a trajectory point.
                     */
                    void push_right( const trajectory::CIterator& it );

                    /**
                     * \brief Remove iterators (points) from the front of the deque until the following conditions are met:
                     *
                     * 1.  The distance covered in the deque is under the limit.
                     * 2.  The deque does not become empty.
                     * 3.  The velocity is below the limit and the road is not in the blacklist.
                     *
                     * \return true if the deque is empty (stop condition); false if it is not empty.
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
                     * \param ptptr A constant reference to a point in a trajectory; this point is later in the trip
                     * than the first point in the deque.
                     *
                     * \return true if the time difference is less than or equal to the max_time.
                     */
                    bool under_time( const trajectory::Point::Ptr& ptptr ) const; 

                    /**
                     * \brief Prediate indicating whether the distance covered by the points in the deque is less than 
                     * the parameterized limit.
                     *
                     * Cover distance is used since GPS error can greatly skew the manhattan distance in instances where no actual
                     * movement has taken place.  In cases where the driver is traveling in a straight line, both cover_distance and
                     * manhattan distance will be equivalent.
                     *
                     * \return true if cover distance <= limit, false otherwise.
                     */
                    bool under_distance() const; 
                    
                    /**
                     * \brief Predicate that indicates the speed in this point record is less than the limit.
                     *
                     * Points having speeds greater than or equal to this speed are not considered by the stop detector.
                     *
                     * \param ptptr A constant reference to a point in a trajectory
                     * \return true if the points current speeds is less than the parameterized max speed.
                     */
                    bool under_speed( const trajectory::Point::Ptr& ptptr ) const;

                    /**
                     * \brief Output to a stream details about this Stop Deque.
                     *
                     * \param os The output stream.
                     * \param q The Stop Deque
                     * \return The output stream after printing for chaining.
                     */
                    friend std::ostream& operator<<( std::ostream& os, const Deque& q );

                private:
                    Stop& stop_detector;                    ///< reference to stop to access parameters.
                    std::deque<trajectory::CIterator> q;    ///< deque of iterators to points where between them there is distance.
                    double cumulative_distance;             ///< cumulative distance across points in the deque.

                    /**
                     * \brief Remove the iterator from the front (left) of the deque and update the cumulative distance.
                     *
                     * \return A constant iterator to the point removed from the deque.
                     */
                    trajectory::CIterator pop_left();

                    /**
                     * \brief Return the number of trip points represented in the deque.
                     *
                     * \return The difference between the trip point index of the first last point and first point + 1.
                     */
                    size_type length() const;

                    /**
                     * \brief Return the time period covered by points within the deque.  
                     *
                     * \return the time difference between the first point and last point in microseconds.
                     */
                    uint64_t delta_time() const;

                    /**
                     * \brief Get the trip point index of the point on the front (left) of the deque.
                     *
                     * \return the index.
                     */
                    size_type left_index() const;

                    /**
                     * \brief Get the trip point index of the point on the back (right) of the deque.
                     *
                     * \return the index.
                     */
                    size_type right_index() const;
            };

        private:
            static osm::HighwaySet          excluded_highways;
            uint64_t                        max_time;             ///< in this implementation this is in microseconds and it is converted during construction.
            double                          min_distance;
            double                          max_speed;

            trajectory::Interval::PtrList   critical_intervals;

        public:
            friend class Deque;

            /**
             * \brief Set the OSM highway types where stops should be ignored.
             *
             * This static method will clear the set first.
             *
             * \param excludes a constant set of OSM highway types.
             * \return the size of the set to ignore.
             */
            static std::size_t set_excluded_highways( const osm::HighwaySet& excludes );

            /**
             * \brief Add an OSM highway type to the current exclude set.
             *
             * \param hw an OSM highway to add to the excluded set.
             * \return the size of the set to ignore.
             */
            static std::size_t add_excluded_highway( const osm::Highway& hw );

            /**
             * \brief Predicate that indicates this point should be considered for stop detection.
             *
             * \param pt a constant pointer to a trajectory point.
             * \return true if the road the point is fit to is IMPLICIT (unknown) or it is NOT in the blacklist.
             * Return false if the road the trip point is on is EXPLICIT and fit to a black list road -- we ignore stops on
             * those roads.
             */
            static bool valid_highway( const trajectory::Point::Ptr& pt );

            /**
             * \brief Construct a stop detector.
             *
             * Stop detectors find stops using distance and time, not by speed alone.  If speed exceeds a certain
             * threshold OR it is on an excluded highway type, it will not be considered for stop detection.
             *
             * \param max_time (seconds) the maximum loiter time considered as a stop.
             * \param min_distance (meters) stops not cover more than this distance.
             * \param max_speed (meters/second) only consider trip points whose speed is less than max speed for stops.
             */
            Stop( double max_time, double min_distance, double max_speed );

            /**
             * \brief Find the critical intervals in the trajectory that exhibit stop behavior.
             *
             * \param traj The trajectory to review for stops.
             * \return a list of pointers to intervals; the intervals capture the stop critical intervals.
             */
            trajectory::Interval::PtrList& find_stops( const trajectory::Trajectory& traj );
    };

}

/**
 * \brief A method that builds two intervals, one for the beginning point of a trip and one for the end point of a trip.
 */
class StartEndIntervals
{
    public:
        /**
         * \brief Return a list of two intervals that define the start - end trajectory critical intervals.  Each
         * interval has one point.
         *
         * \param traj The trajector to pull the intervals from.
         * \return a length 2 list of pointers to the start and end trip critical intervals.
         */
        const trajectory::Interval::PtrList& get_start_end_intervals( const trajectory::Trajectory& traj );

    private:
        trajectory::Interval::PtrList intervals;            ///< The start and end trip critical intervals.
};

/**
 * \brief Label each point in a trip with the interval that contains it.
 */
class IntervalMarker
{
    public:
        /**
         * \brief Constructor
         *
         * \param list The list of intervals to use for marking the trajectory.
         */
        IntervalMarker( const std::initializer_list<trajectory::Interval::PtrList> list );

        /**
         * \brief Associate each point in a trajectory with the interval that contains it.  All trip points may not be
         * associated with an interval.
         *
         * \param traj The trajectory to associate.
         */
        void mark_trajectory( trajectory::Trajectory& traj ); 

    private:
        trajectory::Interval::PtrList intervals;
        
        uint32_t critical_interval;
        trajectory::IntervalCPtr iptr;

        void merge_intervals( const std::initializer_list<trajectory::Interval::PtrList> list );
        void mark_trip_point( trajectory::Point::Ptr& tp );
        void set_next_interval();
        static bool compare( trajectory::IntervalCPtr a, trajectory::IntervalCPtr b );
};

#endif
