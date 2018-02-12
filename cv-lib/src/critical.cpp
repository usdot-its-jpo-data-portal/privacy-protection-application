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
#include <algorithm>

#include "critical.hpp"

namespace Detector {

    /******************************** TurnAround ************************************************/
    TurnAround::TurnAround( size_t max_q_size, double area_width, double max_speed, double heading_delta ) :
        max_q_size{ max_q_size },
        area_width{ area_width },
        max_speed{ max_speed },
        heading_delta{ heading_delta  },
        is_previous_trip_point_fit{ false },
        fit_exit_point{ nullptr },
        is_fit_exit{ false },
        area_q{},
        current_edge{ nullptr },
        interval_list{},
        area_set{}
    {}

    trajectory::Interval::PtrList& TurnAround::find_turn_arounds( const trajectory::Trajectory& traj ) {
        for (auto& tp : traj) {
            update_turn_around_state( tp );
        }

        return interval_list;
    }

    void TurnAround::update_turn_around_state( const trajectory::Point::Ptr& tp ) {
        geo::EdgeCPtr tp_edge = tp->get_fit_edge();

        if (tp->is_explicitly_fit()) {
            // The trip point is fit to an explicit edge.
            // Check if the previous trip point has been fit and set the fit 
            // exit trip point.
            if (!is_previous_trip_point_fit) {
                // The previous trip point has not been fit.
                // Check if there is a heading change between this trip point the
                // the previous fit exit trip point.
                if (is_fit_exit && tp->heading_delta( *fit_exit_point ) >= heading_delta) {
                    // There is a change in fit trajectory headings.
                    // This is a critical interval.
                    interval_list.push_back( std::make_shared<trajectory::Interval>( fit_exit_point->get_index(), tp->get_index(), "ta_fit" ) );
                }

                current_edge = nullptr;
                area_q.clear();
                is_previous_trip_point_fit = true;
            }
            
            is_fit_exit = true;
            fit_exit_point = tp;
        } else if (!current_edge) {
            // There is no previous edge.
            // Set the edge to the fit edge of the trip point, and reset the
            // previous fit trip point flag.
            current_edge = tp_edge;
            is_previous_trip_point_fit = false;
        } else {
            // There is a previous point.
            // Check for a critical interval and check if the edge has changed.
            if (is_critical_interval( tp )) {
                // A turn around was detected.
                // Reset the queue.
                bool first;

                for (auto& atptr : area_q) {
                    if (first) {
                        area_set.insert( atptr->first );
                        first = false;

                        continue;
                    }
                }

                area_q.clear();
            }

            if (current_edge->get_uid() != tp_edge->get_uid()) {
                // The edge has changed.
                // Add the edge to the area queue and update the edge.
                try 
                {
                    geo::Area::Ptr aptr = current_edge->to_area( area_width, 0.0 );
                    area_q.push_front( std::make_shared<AreaIndexPair>( aptr, tp->get_index() ));

                    if (area_q.size() >= max_q_size) {
                        area_q.pop_back();
                    }
                }
                catch (geo::ZeroAreaException)
                {
                    // Zero area.
                    // Don't add the area to the queue.
                }

                current_edge = tp_edge;
            }
        }
    }

    bool TurnAround::is_critical_interval( const trajectory::Point::Ptr& tp ) {
        bool first = true;

        for (auto& atptr : area_q) {
            if (first) {
                first = false;

                continue;
            }

            if (atptr->first->contains( *tp ) && tp->get_speed() < max_speed) {
                area_set.insert( atptr->first );
                interval_list.push_back( std::make_shared<trajectory::Interval>(atptr->second, tp->get_index(), "ta"));

                return true;
            }
        }

        return false; 
    }

    /******************************** Stop::Deque ************************************************/

    /**
     * CTOR for new stop detector deque.
     */
    Stop::Deque::Deque( Stop& detector ) :
        stop_detector{ detector },
        q{},
        cumulative_distance{0.0}
    {
    }

    /**
     * Return the number of iterators that the deque characterizes.
     *
     * NOTE: in this implementation we are saving every point/iterator so length and size should always be the same.
     */
    Stop::Deque::size_type Stop::Deque::length() const
    {
        if (q.empty()) return 0;
        return (*q.back())->get_index() - (*q.front())->get_index() + 1;
    }

    /**
     * Return the time period covered by points within the deque.  This is not meant to be used by the logic that finds
     * stops.
     */
    uint64_t Stop::Deque::delta_time() const
    {
        if (q.empty()) return 0;
        return (*q.back())->get_time() - (*q.front())->get_time();
    }

    /**
     * The manhattan distance covered by the points within the deque.
     */
    double Stop::Deque::delta_distance() const
    {
        return cumulative_distance;
    }

    /**
     * Predicate that indicates the speed in this point record is less than the speed where we start considering
     * (collecting points) stop detection properties.
     */
    bool Stop::Deque::under_speed( const trajectory::Point::Ptr& ptptr ) const
    {
        return ptptr->get_speed() < stop_detector.max_speed;
    }

    /**
     * The direct distance from first point to last point within the deque.
     *
     * NOTE: This seems to be a better metric because GPS error could infer a larger manhattan distance when a vehicle
     * is actually not moving at all.
     */
    double Stop::Deque::cover_distance() const
    {
        // must have at least 2 points to have non-zero distance.
        if (q.size()<2) return 0.0;

        trajectory::Point::Ptr f = *q.front();
        trajectory::Point::Ptr b = *q.back();

        return geo::Location::distance( *f, *b );
    }

    /**
     * Predicate indicating whether the deque's INVARIANT would be broken by adding this trip point to it.
     *
     * INVARIANT: The time elapsed from first trip point in deque to last never exceeds max_time.
     */
    bool Stop::Deque::under_time( const trajectory::Point::Ptr& ptptr ) const
    {
        uint64_t time_period = ptptr->get_time() - (*q.front())->get_time();
        return (time_period <= stop_detector.max_time);
    }

    /**
     * Prediate indicating whether the distance covered by the points in the deque is less than the limit at which too
     * much movement has taken place to consider it a stop.
     *
     * Cover distance is used since GPS error can greatly skew the manhattan distance in instances where no actual
     * movement has taken place.  In cases where the driver is traveling in a straight line, both cover_distance and
     * manhattan distance will be equivalent.
     */
    bool Stop::Deque::under_distance() const
    {
        return cover_distance() <= stop_detector.min_distance;
    }

    /**
     * Utility method to generate the interval.
     */
    Stop::Deque::size_type Stop::Deque::left_index() const
    {
        if (!q.empty()) {
            return (*q.front())->get_index();
        } else {
            return 0;
        }
    }

    /**
     * Utility method to generate the interval.
     */
    Stop::Deque::size_type Stop::Deque::right_index() const
    {
        if (!q.empty()) {
            return (*q.back())->get_index();
        } else {
            return 0;
        }
    }

    /**
     * Remove iterators (points) from the front of the deque until the following conditions are met:
     *
     * 1.  The distance covered in the deque is under the limit.
     * 2.  The deque does not become empty.
     * 3.  The velocity is below the limit and the road is not in the blacklist.
     *
     * Return: true if the deque is empty (stop condition); false if it is not empty.
     */
    bool Stop::Deque::unwind()
    {
        // first, make the deque underdistance.
        while ( !q.empty() && !under_distance() ) {
            pop_left();
        } 

        // second, we can remove all points that should not have been placed on the deque in the first place.
        // the reason they are on the deque now is because previous points met the conditions we are checking
        // for here.
        while ( !q.empty() && !(under_speed( *q.front() ) && Stop::valid_highway( *q.front() ) ) ) {
            pop_left();
        }

        return q.empty();
    }

    /**
     * Add a point/iterator to the back (right) of the deque and update the manhattan distance.
     */
    void Stop::Deque::push_right( const trajectory::CIterator& it )
    {
        if ( !q.empty() ) {
            // Compute the distance covered between the last point and the newly added point.
            double dd = geo::Location::distance( *(*it), *(*(q.back())) ); 
            cumulative_distance += dd;
        }

        q.push_back(it);
    }

    void Stop::Deque::reset()
    {
        q.clear();
        cumulative_distance = 0.0;
    }

    /**
     * CAUTION: to be used in conjunction with unwind() don't make this public.
     *
     * Remove the iterator from the front (left) of the deque and update the manhattan distance.
     *
     * If the deque contain one or fewer points or the manhattan distance goes negative this code RESETS the manhattan
     * distance state variable to 0.0.
     */
    trajectory::CIterator Stop::Deque::pop_left()
    {
        // remove the oldest point from the left side of the deque.
        trajectory::CIterator it = q.front();
        q.pop_front();

        if ( q.size() > 1 ) {   // some cumulative distance remains to be tracked.
            // update the cumulative distance by subtracting the straight-line distance between the point
            // removed and the new back of the deque.  We know the deque is not empty.
            cumulative_distance -= geo::Location::distance( *(*it), *(*(q.back())) );

        } else {                // q.size() <= 1, i.e., no distance.

            cumulative_distance = 0.0;
        }

        return it;
    }

    std::ostream& operator<<( std::ostream& os, const Stop::Deque& q )
    {
        os  << "stop deque: [" << q.left_index() << "," << q.right_index() << "]"
            << " length: " << q.length() 
            << " size: "   << q.q.size() 
            << " ddist: "  << q.delta_distance() 
            << " cdist: "  << q.cover_distance() 
            << " dtime: "  << q.delta_time();
        return os;
    }

    /******************************** Stop ************************************************/

    osm::HighwaySet Stop::excluded_highways{ 
        { osm::Highway::MOTORWAY,
        osm::Highway::TRUNK,
        osm::Highway::PRIMARY,
        osm::Highway::MOTORWAY_LINK,
        osm::Highway::TRUNK_LINK,
        osm::Highway::PRIMARY_LINK }
    };

    std::size_t Stop::set_excluded_highways( const osm::HighwaySet& excludes )
    {
        excluded_highways.clear();
        excluded_highways.insert( excludes.begin(), excludes.end() );
        return excluded_highways.size();
    }

    std::size_t Stop::add_excluded_highway( const osm::Highway& highway )
    {
        excluded_highways.insert( highway );
        return excluded_highways.size();
    }

    /**
     * Predicate that returns true if the road the point is fit to is IMPLICIT (unknown) or it is NOT in the blacklist.
     * Return false if the road the trip point is on is EXPLICIT and fit to a black list road -- we ignore stops on
     * those roads.
     */
    bool Stop::valid_highway( const trajectory::Point::Ptr& ptptr )
    {
        if (ptptr->is_explicitly_fit()) {           // this trip point has an OSM way type.
            
            osm::Highway hw = ptptr->get_fit_edge()->get_way_type();
            auto it = excluded_highways.find( hw );

            return (it == excluded_highways.end());   // not found in blacklist, so this point is on a road that can't be ignored.
        }

        // if the trip point is not explicitly fit then we assume we should check for stops.
        return true;
    }

    Stop::Stop( double max_time, double min_distance, double max_speed ) :
        max_time {static_cast<uint64_t>( max_time * 1000000 )},
        min_distance {min_distance},
        max_speed {max_speed},
        critical_intervals{}
    {
    }

    /**
     * Find the critical intervals in the provided trajectory that exhibit stop behavior.
     *
     * NOTE: const reference to container will only allow const_iterators...
     */
    trajectory::Interval::PtrList& Stop::find_stops( const trajectory::Trajectory& traj )
    {
        Stop::Deque q{*this};

        // made a const_iterator so we can keep the traj const-correctness.
        trajectory::CIterator t_it = traj.begin();

        while ( t_it != traj.end() ) {                  // outer loop.

            // only investigate trip points that are under max_speed and on the right kind of roads.
            if ( q.under_speed( *t_it ) && valid_highway( *t_it ) ) {

                q.push_right( t_it );                   // this should always be the first point into the q.
                ++t_it;                                 // we work with a non-empty q and a point in hand in the second phase.

                while ( t_it != traj.end() ) {          // inner loop to maximize the deque's invariant time condition.

                    if ( q.under_time( *t_it ) ) {      // time of current point - oldest point in deque <= max_time.

                        
                        q.push_right( t_it );           // invariant continues to hold based on the check just done.
                        ++t_it;                         // new point in hand.

                    } else {                            // this point will break the invariant condition, so check for distance.

                        if ( q.under_distance() ) {     // distance covered in the deque <= minimum distance parameter; CI detected.

                            // critical interval to save.  The entire deque, so it is empty.
                            trajectory::IntervalPtr ciptr =  std::make_shared<trajectory::Interval>( trajectory::Interval{ q.left_index(), q.right_index(), "stop" } );
                            critical_intervals.push_back( ciptr );
                            q.reset();                  // prepare for next critical interval.
                            break;                      // t_it is a valid point that needs checking in outer loop for new interval.

                        } else {                        // "over distance"

                            // remove points from front of deque; could empty deque.
                            if (q.unwind()) break;

                        }
                    }
                }  

            } else {                                    // speed >= max_speed OR on black listed highway; just skip.
                ++t_it;                                 // new point in hand.
            }
        }                                               // outer loop end.

        // could have a non-empty deque... decision: since the deque has not satisfied the conditions we IGNORE this
        // interval.  It should be made up using the begin and end point use anyway.
        return critical_intervals;
    }
}

/******************************** StartEndIntervals ************************************************/
    
const trajectory::Interval::PtrList& StartEndIntervals::get_start_end_intervals( const trajectory::Trajectory& traj ) 
{
    if (intervals.size() == 2) {
        return intervals;
    }
   
    trajectory::Index second_to_last_index = traj.size() > 0 ? traj.size() - 1 : 0;
    intervals.push_back( std::make_shared<trajectory::Interval>( 0, 1, "start_pt" ) );
    intervals.push_back( std::make_shared<trajectory::Interval>( second_to_last_index, second_to_last_index + 1, "end_pt" ) );

    return intervals;
} 

/******************************** IntervalMarker ************************************************/
IntervalMarker::IntervalMarker( const std::initializer_list<trajectory::Interval::PtrList> list ) :
    critical_interval{ 0 },
    iptr{ nullptr }
{
    merge_intervals( list );
    set_next_interval();
}

bool IntervalMarker::compare( trajectory::IntervalCPtr a, trajectory::IntervalCPtr b )
{
    trajectory::Index a_left = a->left();
    trajectory::Index b_left = b->left();

    if (a_left < b_left) 
    {
        return true;
    } 
    else if (a_left == b_left) 
    {
        return a->right() < b->right();
    }

    return false;
}

void IntervalMarker::merge_intervals( const std::initializer_list<trajectory::Interval::PtrList> list )  
{
    trajectory::Interval::PtrList sorted_intervals;

    // Combine all the interval lists into one list.
    for (auto& interval_list : list)
    {
        sorted_intervals.insert( sorted_intervals.end(), interval_list.begin(), interval_list.end() );
    }

    // Check there are 1 or less intervals.
    if (sorted_intervals.size() <= 0) {
        // There are no intervals.
        // No need to sort or merge.
        return;
    }

    if (sorted_intervals.size() == 1) {
        // There is one interval.
        // No need to sort or merge.
        intervals.push_back(sorted_intervals[0]);         

        return;
    }

    // There are two or more intervals.
    // Sort the intervals.
    std::sort(sorted_intervals.begin(), sorted_intervals.end(), compare);

    // Try to merge the intervals.

    // Index the number of merge intervals.
    trajectory::Index n_merged = 0;
    
    // Save the first internal state.
    auto first = sorted_intervals.begin();
    trajectory::Index start = (*first)->left();
    trajectory::Index end = (*first)->right();
    trajectory::Interval::AuxSetPtr aux_set_ptr = (*first)->get_aux_set();
        
    // Go through the rest of the intervals.
    for (auto it = std::next(first, 1); it != sorted_intervals.end(); ++it) 
    {
        trajectory::Index next_start = (*it)->left();
        trajectory::Index next_end = (*it)->right();
        trajectory::Interval::AuxSetPtr next_aux_set_ptr = (*it)->get_aux_set();

        if (next_start <= end) 
        {
            // This interval starts within the saved interval.
            // Update the aux set with this interval's aux set.
            aux_set_ptr->insert( next_aux_set_ptr->begin(), next_aux_set_ptr->end() ); 

            if (next_end > end)
            {
                // This interval extends past the saved interval.
                // Set the saved end to this interval end.
                end = next_end;
            }
        } 
        else 
        {
            // This interval starts after the saved interval.
            // Add the saved interval to the list and update the 
            // saved interval state.
            intervals.push_back( std::make_shared<trajectory::Interval>( start, end, aux_set_ptr, n_merged ) );
            n_merged++;

            start = next_start;
            end = next_end;
            aux_set_ptr = next_aux_set_ptr;
        }
    }
    
    // The intevals were exhausted.
    // Add the removing saved interval to the list.
    intervals.push_back( std::make_shared<trajectory::Interval>( start, end, aux_set_ptr, n_merged ) );
}

void IntervalMarker::set_next_interval()
{
    if (critical_interval >= intervals.size()) 
    {   
        iptr = nullptr;

        return;
    }
    
    iptr = intervals[critical_interval++];
}

void IntervalMarker::mark_trajectory( trajectory::Trajectory& traj ) 
{
    for (auto& tp : traj) {
        mark_trip_point( tp );
    }
}

void IntervalMarker::mark_trip_point( trajectory::Point::Ptr& tp ) 
{
    if (!iptr) 
    {
        // There are no more intervals.
        // Nothing can be done for this trip point.
        return;
    }
    
    while (iptr->is_before( tp->get_index() ))
    {
        // The trip point is after the end of the interval.
        // Find the next interval that contains the trip point.
        set_next_interval();

        if (!iptr)
        {
            // There are no more intervals.
            // Nothing can be done for this trip point.
            return;
        }
    }
        
    // The trip point is before or within the interval.
    // Check if it is within.
    if (iptr->contains( tp->get_index() )) 
    {
        // The trip point is within the interval.
        // Set critical interval for the trip point.
        tp->set_critical_interval( iptr );

        return;
    }

    // The trip point is before the interval.
    // Check the next trip point.
}
