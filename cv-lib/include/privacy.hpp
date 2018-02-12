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
#ifndef CTES_DI_PRIVACY_HPP
#define CTES_DI_PRIVACY_HPP

#include "names.hpp"
#include "entity.hpp"
#include "instrument.hpp"
#include "trajectory.hpp"
#include "quad.hpp"

#include <iterator>

/**
 * \brief Define the intervals that "hide" critical intervals. Instances of this class determine the portion of a trajectory extended from the ends
 * of the previously defined critical intervals to protect those locations.  This is done using intersection out degree,
 * direct distance, and manhattan distance.
 */
class PrivacyIntervalFinder
{
    public:
        using TrajectoryIterator = trajectory::Trajectory::iterator;
        using RevTrajectoryIterator = trajectory::Trajectory::reverse_iterator;

        /**
         * \brief Construct a PrivacyIntervalFinder
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
         * \brief Find the privacy intervals in the provided trajectory.
         *
         * \param traj The trajectory to search for privacy intervals.
         * \return A list of the intervals that define the privacy intervals.
         */
        const trajectory::Interval::PtrList& find_intervals( trajectory::Trajectory& traj );

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
        trajectory::IntervalCPtr curr_ciptr;
        trajectory::Point::Ptr init_priv_point;
        double md;
        uint32_t out_degree;
        trajectory::Index interval_start;
        trajectory::Index last_pi_end;
        trajectory::Interval::PtrList interval_list;
	    TrajectoryIterator curr_tp_it;

        bool is_edge_change(geo::EdgeCPtr a, geo::EdgeCPtr b) const;
        void update_intervals( const TrajectoryIterator tp_it, trajectory::Trajectory& traj );
        void find_interval( const TrajectoryIterator start, const TrajectoryIterator end );
        void find_interval( const RevTrajectoryIterator start, const RevTrajectoryIterator end );
        trajectory::Index find_interval_end( const TrajectoryIterator start, const TrajectoryIterator end );
        trajectory::Index find_interval_end( const RevTrajectoryIterator start, const RevTrajectoryIterator end );
        bool handle_edge_change( const TrajectoryIterator curr, const TrajectoryIterator prev, const geo::EdgeCPtr& eptr );
        bool handle_edge_change( const RevTrajectoryIterator curr, const RevTrajectoryIterator prev, const geo::EdgeCPtr& eptr );
};

/**
 * \brief The privacy intervals identified may intersect other privacy intervals or critical intervals.  This class
 * deals with intersection while marking a trajectory.
 */
class PrivacyIntervalMarker
{
    public:

        /**
         * \brief Construct a PrivacyIntervalMarker
         *
         * \param list The list of privacy intervals to mark.
         */
        PrivacyIntervalMarker( const std::initializer_list<trajectory::Interval::PtrList> list );

        /**
         * \brief Mark a trajectory using the privacy intervals set in the constructor.
         *
         * \param traj The trajectory to mark.
         */
        void mark_trajectory( trajectory::Trajectory& traj ); 

    private:
        trajectory::Interval::PtrList intervals;
        
        uint32_t privacy_interval;
        trajectory::IntervalCPtr iptr;

        void merge_intervals( const std::initializer_list<trajectory::Interval::PtrList> list );
        void mark_trip_point( trajectory::Point::Ptr& tp );
        void set_next_interval();
        static bool compare( trajectory::IntervalCPtr a, trajectory::IntervalCPtr b );
};

/**
 * \brief Remove critical and privacy intervals from a trajectory.
 */
class DeIdentifier
{
    public:
        /**
         * \brief Remove the marked privacy and critical interval points from traj returning the new trajectory.
         *
         * \param traj The marked trajectory.
         * \return The de-identified trajectory.
         */
        const trajectory::Trajectory& de_identify( const trajectory::Trajectory& traj );

        /**
         * \brief Remove the marked privacy and critical interval points from traj returning the new trajectory. Count
         * the number of records in privacy and critical intervals.
         *
         * \param traj The marked trajectory.
         * \param point_counter a statistics aggregator.
         * \return The de-identified trajectory.
         */
        const trajectory::Trajectory& de_identify( const trajectory::Trajectory& traj,  instrument::PointCounter& point_counter);
    private:
        trajectory::Trajectory new_traj;
};

#endif
