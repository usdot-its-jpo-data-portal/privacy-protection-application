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
#ifndef CTES_DI_TRIPPOINT_HPP
#define CTES_DI_TRIPPOINT_HPP

#include "names.hpp"
#include "entity.hpp"

#include <tuple>

namespace trajectory {

    // advance declaration
    class Interval;

    using IntervalPtr = std::shared_ptr<Interval>;
    using IntervalCPtr = std::shared_ptr<const Interval>;
    using StreamPtr = std::shared_ptr<std::istream>;


    /**
     * \brief A location record from a trip file.
     */
    class Point : public geo::Location
    {
        public:
            using Ptr = std::shared_ptr<Point>;

            /**
             * \brief Convert an angular difference into the interval [0,180]
             *
             * \param diff a difference between two angles.
             * \return the difference as a value in the interval [0,180]
             */
            static double diff_180( double diff );

            /**
             * \brief Compute the difference between two angles.
             *
             * \param a an angle in degrees.
             * \param b an angle in degrees.
             * \return the different between a and b in the range [0,180].
             */
            static double angle_error( double a, double b );

            /**
             * \brief Default Constructor.
             */
            Point();

            /**
             * \brief Construct a point.
             *
             * \param data the line in the file used to define this point.
             * \param time the time when this point was measured in microseconds.
             * \param lat the point's latitude
             * \param lon the point's longitude
             * \param heading the heading at the time.
             * \param speed the speed (m/s) at the time.
             * \param index the 0-based index number of this point in the trip.
             */
            Point( const std::string& data, uint64_t time, double lat, double lon, double heading, double speed, uint64_t index );

            /**
             * \brief Get the original record data that was used to define this point.
             *
             * \return the original record data as a constant reference.
             */
            const std::string& get_data() const;

            /**
             * \brief Output stream "printer" for a point.
             *
             * \param os the output stream where the point should be printed.
             * \param tp the trip point to be printed.
             * \return the output stream for chaining.
             */
            friend std::ostream& operator<< ( std::ostream& os, const Point& tp );

            /**
             * \brief Compute the difference between this Point and the provided Point.
             *
             * \param tp The trip point (heading) to compare against this instance (heading).
             * \return the difference in headings in the range [0,180]
             */
            double heading_delta( const Point& tp ) const;

            /**
             * \brief Compute the difference between this Point and the provided Point.
             *
             * \param heading_other The heading to compare against this instance's heading.
             * \return the difference in headings in the range [0,180]
             */
            double heading_delta( double heading_other ) const;

            /**
             * \brief Get heading.
             *
             * \return heading.
             */
            double get_heading() const;

            /**
             * \brief Get speed.
             *
             * \return speed.
             */
            double get_speed() const;

            /**
             * \brief Get index.
             *
             * \return index.
             */
            uint64_t get_index() const;

            /**
             * \brief Get time.
             *
             * \return time.
             */
            uint64_t get_time() const;

            /**
             * \brief Get the out degree of this point. Outdegree is a vertex's degree - 1. The default outdegree is 0.
             *
             * \return out degree as a positive integer.
             */
            uint32_t get_out_degree() const;

            /**
             * \brief Set the out degree of this point to a non-default value.
             *
             * \return A reference to this point after the degree has been set for chaining.
             */
            Point&  set_out_degree( uint32_t degree );

            /**
             * \brief A predicate indicating this point has a map matched edge.
             *
             * \return true if this point has been matched to an edge, false otherwise.
             */
            bool has_edge() const;

            /**
             * \brief Set a matched edge to this point.
             *
             * \param eptr a shared pointer to the map edge this point is matched to.
             */
            void set_fit_edge( const geo::EdgeCPtr& eptr );

            /**
             * \brief Get the edge this point has been matched to.
             *
             * \return a shared pointer to the matched edge; the edge is constant.
             */
            geo::EdgeCPtr get_fit_edge();

            /**
             * \brief Get the critical interval associated with this point; this may be nullptr in which case it is not
             * part of a critical interval.
             *
             * \return a shared pointer to the critical interval; nullptr if no critical interval has been assigned.
             */
            IntervalCPtr get_critical_interval() const;

            /**
             * \brief Set the critical interval associated, or containing, this point.
             *
             * \param iptr A shared pointer to the critical interval.
             */
            void set_critical_interval(IntervalCPtr iptr);
        
            /**
             * \brief Predicate that indicates the point has been matched to an edge and the edge is an implicit edge
             * (not from map data).
             *
             * \return true if matched and implicit, false otherwise.
             */
            bool is_implicitly_fit() const;

            /**
             * \brief Predicate that indicates the point has been matched to an edge and the edge is an explicit edge
             * (map data).
             *
             * \return true if matched and explicit, false otherwise.
             */
            bool is_explicitly_fit() const;

            /**
             * \brief Mark this point as being part of a privacy interval.
             */
            void set_private();

            /**
             * \brief Set the index of this point within the trip. A default trip point has index 0.
             *
             * \param i the new index for this trip point.
             */
            void set_index(uint64_t i);

            /**
             * \brief Predicate indicating this point is part of a privacy interval.
             *
             * \return true if this point is part of a privacy interval.
             */
            bool is_private() const;

            /**
             * \brief Predicate indicating this point is part of a critical interval.
             *
             * \return true if this point is part of a critical interval.
             */
            bool is_critical() const;

            /**
             * \brief Predicate indicating the heading of this point and the direction of the provided edge are
             * "consistent." Consistency is defined as being < 15.0 degrees of separation.
             *
             * \param eptr A pointer to the edge to be compared.
             *
             * \return true if heading - edge bearing < 15 degrees, false otherwise.
             */
            bool consistent_with( const geo::EdgePtr& eptr ) const;

            /**
             * \brief Predicate indicating the heading of this point and the direction of the provided edge are
             * "consistent." Consistency is defined as being < 15.0 degrees of separation.
             *
             * \param eptr A constant reference to the edge to be compared.
             *
             * \return true if heading - edge bearing < 15 degrees, false otherwise.
             */
            bool consistent_with( const geo::Edge& edge ) const;

        private:
            std::string data;               //> all the data so we don't need fields for every piece.
            double heading;
            double speed;
            uint64_t time;
            uint64_t index;

            geo::EdgeCPtr fitedge;
            IntervalCPtr critical_interval;
            bool _private;
            bool is_hmm_map_match_;
            uint32_t outdegree;
    };

    using Trajectory  = std::vector<Point::Ptr>;
    using Index       = Trajectory::size_type;
    using Iterator    = Trajectory::iterator;
    using CIterator   = Trajectory::const_iterator;
    using RevIterator = Trajectory::reverse_iterator;

    /**
     * \brief An integer-based interval class to be used with trajectories.
     */
    class Interval
    {
        public:
            using PtrList = std::vector<IntervalCPtr>;
            using AuxSet = std::unordered_set<std::string>;
            using AuxSetPtr = std::shared_ptr<AuxSet>;

            /**
             * \brief Construct the interval [left,right]
             *
             * \param left the lower bound  of the interval.
             * \param right the upper bound of the interval.
             * \param aux a description of the interval.
             * \param id an identifier for the interval.
             */
            Interval( Index left, Index right, std::string aux = "", Index id = 0 );

            /**
             * \brief Construct the interval [left,right]
             *
             * \param left the infimum of the interval.
             * \param right the supremum of the interval.
             * \param aux a description of the interval.
             * \param id an identifier for the interval.
             */
            Interval( Index left, Index right, AuxSetPtr aux_set_ptr, Index id = 0);

            /**
             * \brief Get the interval id.
             *
             * \return The id of the interval as a std::vector::size_type 
             */
            Index id() const;

            /**
             * \brief Get the upper bound of the interval.
             *
             * \return The interval upper bound as a std::vector::size_type 
             */
            Index right() const;

            /**
             * \brief Get the lower bound of the interval.
             *
             * \return The interval lower bound as a std::vector::size_type 
             */
            Index left() const;

            /**
             * \brief Get the auxiliary data set as a pointer.
             *
             * \return the set of strings that represent the auxiliary data.
             */
            const AuxSetPtr get_aux_set() const;

            /**
             * \brief Get the auxiliary data set as a single semicolon delimited string.
             *
             * \return the auxiliary data set as one string.
             */
            const std::string get_aux_str(void) const;

            /**
             * \brief Predicate: interval upper bound <= value 
             *
             * \return true if interval upper bound <= value.
             */
            bool is_before( Index value ) const;

            /**
             * \brief Predicate: value in [lower bound, upper bound); this check uses a right open interval.
             *
             * \return true if lower bound <= value < upper bound, false otherwise.
             */
            bool contains( Index value ) const;

            /**
             * \brief write the string form (id = x [lower bound, upper bound) types: { <aux data> }) of an Interval to
             * an output stream.
             *
             * \param os the output stream.
             * \param interval the Interval to write.
             *
             * \return the output stream for chaining.
             */
            friend std::ostream& operator<< ( std::ostream& os, const Interval& interval );

        private:
            Index _left;
            Index _right;
            AuxSet _aux_set;
            Index _id;
    };

    /**
     * \brief Abstract base class for classes that must create a trajectory from a file.
     */
    class TrajectoryFactory {
        public:

        /**
         * \brief Pure virtual methods for making a trajectory install from a file.
         *
         * \param input the trip file that contains the point records.
         */
        virtual const Trajectory make_trajectory(const std::string& input) = 0;

        /**
         * \brief Return the current trajectory unique identifier.
         *
         * \return the UID as a string.
         */
        virtual const std::string get_uid(void) const = 0;
    };

    /**
     * \brief Abstract base class for classes that write trajectories to a file.
     */
    class TrajectoryWriter {
        public:

        /**
         * \brief Write a trajectory to a file named based on the trajectories unique id (uid).
         *
         * \param trajectory the trajectory to write.
         * \param uid the trajectories UID -- this will be used to name the output file.
         * \param strip_cr flag to signal carriage returns should be removed.
         */
        virtual void write_trajectory(const Trajectory& trajectory, const std::string& uid, bool strip_cr) const = 0;
    };

}
#endif
