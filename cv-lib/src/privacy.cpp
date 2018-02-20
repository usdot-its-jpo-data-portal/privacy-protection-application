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
#include "privacy.hpp"

#include <algorithm>
#include <cmath>

/******************************** PrivacyIntervalFinder ************************************************/
PrivacyIntervalFinder::PrivacyIntervalFinder( double min_dd, double min_md, uint32_t min_out_degree, double max_dd, double max_md, uint32_t max_out_degree, double dd_rand, double md_rand, double out_degree_rand ) :
    min_dd{ min_dd },
    min_md{ min_md },
    min_out_degree{ min_out_degree },
    max_dd{ max_dd },
    max_md{ max_md },
    max_out_degree{ max_out_degree },
    dd_rand{ (max_dd - min_dd) * dd_rand },
    md_rand{ (max_md - min_md) * md_rand },
    out_degree_rand{ (max_out_degree - min_out_degree) * out_degree_rand },
    curr_ciptr{ nullptr },
    init_priv_point{ nullptr },
    md{ 0.0 },
    out_degree{ 0 },
    interval_start{ 0 },
    last_pi_end{ 0 },
    interval_list{}
{}

bool PrivacyIntervalFinder::is_edge_change(geo::EdgeCPtr a, geo::EdgeCPtr b) const {
    if ((a->is_implicit() && !b->is_implicit()) || (!a->is_implicit() && b->is_implicit())) {
        return true;
    }

    return a->get_uid() != b->get_uid();
}

const trajectory::Interval::PtrList& PrivacyIntervalFinder::find_intervals( trajectory::Trajectory& traj ) 
{
    for (curr_tp_it = traj.begin(); curr_tp_it != traj.end(); ++curr_tp_it) 
    {
        update_intervals( curr_tp_it, traj );
    }

    return interval_list;
}

void PrivacyIntervalFinder::update_intervals( const TrajectoryIterator tp_it, trajectory::Trajectory& traj ) 
{
    trajectory::Point::Ptr tp = *tp_it;
    trajectory::IntervalCPtr ciptr = tp->get_critical_interval();
    trajectory::Index index = tp->get_index();

    if (!curr_ciptr) 
    {
        // The previous trip point was not within a critical interval.
        // Check if the current trip point is within a critical interval.
        if (ciptr) 
        {
            // The current trip point is within a critical interval.
            // Update the critical interval state and check the trip point 
            // index.
            curr_ciptr = ciptr;
            
            if (index > 0 && tp->get_index() > last_pi_end)
            {
                // This is not the first trip point and there is at least one
                /// point prior to this that is not within a privacy interval.
                // Find privacy points prior to the start of this interval.
                find_interval( std::reverse_iterator<trajectory::Trajectory::iterator>( tp_it ), std::reverse_iterator<trajectory::Trajectory::iterator>( traj.begin() ) );
            }
        }
    } 
    else if (!ciptr) 
    {
        // The current trip point is no longer within a critical interval.
        // Update the critical interval state and check the trip point.
        curr_ciptr = nullptr;

        if (index + 1 < traj.size()) 
        {
            // This is not the last trip point.
            // Find privacy points after the start of this interval.
            find_interval( tp_it, traj.end() );
        } 
    }
    else if (ciptr->id() != curr_ciptr->id())
    {
        // The trip point has changed critical intervals without being in
        // a non-critical interval (interval to interval).
        // Update the critical interval state.
        curr_ciptr = ciptr;
    }
}

/******************************Forward PI Routines*****************************/

void PrivacyIntervalFinder::find_interval( const TrajectoryIterator start, const TrajectoryIterator end )
{
    trajectory::Point::Ptr tp = *start;
    init_priv_point = tp;
    rand_min_md = (md_rand * static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) + min_md;
    rand_min_dd = (dd_rand * static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) + min_dd;
    rand_min_out_degree = static_cast <uint32_t> ((out_degree_rand * static_cast <float> (rand()) / static_cast <float> (RAND_MAX))) + min_out_degree;

    // Reset the distance state, out degree state, and start index state.
    md = 0.0;
    out_degree = tp->get_out_degree();
    interval_start = tp->get_index();

    trajectory::Index interval_end = interval_start;
    geo::EdgeCPtr eptr = tp->get_fit_edge();
    TrajectoryIterator edge_start = start;
    TrajectoryIterator last;

    for (auto tp_it = start; tp_it != end; ++tp_it)
    {
        tp = *tp_it;
        last = tp_it;

        // Set the end of the interval to the current trip point.
        interval_end = tp->get_index();

        if (tp->get_critical_interval()) 
        {
            // The trip point ran into another crtiical interval.
            // Everything up to this point is a privacy interval.
            last_pi_end = interval_end;
            curr_tp_it += (interval_end - interval_start) - 1; 
            interval_list.push_back( std::make_shared<trajectory::Interval>( interval_start, interval_end, "forward:ci" ) );
		
          	return;
        }

        geo::EdgeCPtr tp_eptr = tp->get_fit_edge();

        if (is_edge_change(tp_eptr, eptr)) 
        {
            // The edge changed.
            // Handle the change by looking at the current and previous edges.
            if (handle_edge_change( edge_start, tp_it, eptr ))
            {
                return;
            }
        
            // Update the local edge state.
            edge_start = tp_it;
            eptr = tp_eptr;
        }
    }

    // There are no more trip points.
    // Check if the distance metric has been met on the current edge.
    trajectory::Index edge_end = find_interval_end( edge_start, last );

    if (edge_end != interval_end) 
    {
        last_pi_end = edge_end;
        curr_tp_it += (edge_end - interval_start) - 1; 
        interval_list.push_back( std::make_shared<trajectory::Interval>( interval_start, edge_end, "forward:max_md" ) );
    }
    else 
    {
        curr_tp_it += (interval_end - interval_start) - 1; 
        interval_list.push_back( std::make_shared<trajectory::Interval>( interval_start, interval_end, "forward:end" ) );
    }
}

bool PrivacyIntervalFinder::handle_edge_change( const TrajectoryIterator prev, const TrajectoryIterator curr, const geo::EdgeCPtr& eptr )
{
    double edge_distance;
    double direct_distance;
    trajectory::Index interval_end = 0;

    trajectory::Point::Ptr prev_tp = *prev;
    trajectory::Point::Ptr curr_tp = *curr;

    // Get the direct distance after changing to this edge.
    direct_distance = init_priv_point->distance_to( *curr_tp );

    // Determine the previous edge type.
    if (!prev_tp->is_explicitly_fit())
    {
        // The previous edge was implicit.
        // Check if the max distance metric is met by traversing the edge.
        edge_distance = eptr->length();
    
        if (edge_distance + md >= max_md || direct_distance >= max_dd )
        {
            // This distance metric was met by tarversing the edge.
            // Find the interval end within the edge and add the 
            // interval to the list.
            interval_end = find_interval_end( prev, curr );
            last_pi_end = interval_end;
            curr_tp_it += (interval_end - interval_start) - 1;
            interval_list.push_back( std::make_shared<trajectory::Interval>( interval_start, interval_end, "forward:max_dist" ) );

            return true;
        }
    }
    else
    {
        // The previous edge was explicit.
        // Compute the out degree from the traversing the fit edge.
        uint32_t edge_out_degree = curr_tp->get_out_degree() - out_degree;
       
        // Check if the current edge is explicit or implicit.
        if (curr_tp->is_explicitly_fit()) 
        {
            // The current edge is explicit.
            // The edge distance is the length of the edge.
            edge_distance = eptr->length();
        }
        else
        {
            // The current edge is not explicit.
            // The edge distance is the trajectory's traversal on the previous
            // edge.
            edge_distance = prev_tp->distance_to( *curr_tp );
        }
        
        // Check the conditions for a privacy interval.
        if (edge_distance + md >= rand_min_md && direct_distance >= rand_min_dd && edge_out_degree >= rand_min_out_degree) 
        {
            // The min metrics were met by by traversing the edge.
            // Because the out degree metric can only be met after traversing
            // the edge, the interval end must be the current trip point.
            last_pi_end = curr_tp->get_index();
            curr_tp_it += (last_pi_end - interval_start) - 1;
            interval_list.push_back( std::make_shared<trajectory::Interval>( interval_start, last_pi_end, "forward:min" ) );

            return true;
        }
        else if (edge_distance + md >= max_md || direct_distance >= max_dd) 
        {
            // A max distance metric was met by the traversal.
            // Find the interval within the edge and add the interval
            // to the list.
            interval_end = find_interval_end( prev, curr );
            last_pi_end = interval_end;
            curr_tp_it += (last_pi_end - interval_start) - 1;
            interval_list.push_back( std::make_shared<trajectory::Interval>( interval_start, interval_end, "forward:max_dist" ) );
        
            return true;
        }
        else if (edge_out_degree >= max_out_degree) 
        {
            // The max out degree metric was met by the traversal.
            // Set the interval.
            last_pi_end = curr_tp->get_index();
            curr_tp_it += (last_pi_end - interval_start) - 1;
            interval_list.push_back( std::make_shared<trajectory::Interval>( interval_start, last_pi_end, "forward:max_out_degree" ) );
 
            return true;
        }
    }

    // No metrics were met.
    // Update the Manhatten distance with the edge distance.
    md += edge_distance;

    return false;
}

trajectory::Index PrivacyIntervalFinder::find_interval_end( const TrajectoryIterator start, const TrajectoryIterator end ) 
{
    double edge_distance;
    double direct_distance;
    trajectory::Point::Ptr curr_tp;

    trajectory::Point::Ptr tp = *start;

    for (auto tp_it = std::next( start, 1 ); tp_it != end; ++tp_it)
    {
        curr_tp = *(tp_it);
        edge_distance = tp->distance_to( *curr_tp );
        direct_distance = init_priv_point->distance_to( *curr_tp );
        
        if (md + edge_distance > max_md || direct_distance > max_dd) 
        {
            return curr_tp->get_index();
        }
    }
    
    return (*end)->get_index();
}

/******************************Backward PI Routines****************************/

void PrivacyIntervalFinder::find_interval( const RevTrajectoryIterator start, const RevTrajectoryIterator end ) 
{
    trajectory::Point::Ptr tp = *start;
    init_priv_point = tp;
    rand_min_md = (md_rand * static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) + min_md;
    rand_min_dd = (dd_rand * static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) + min_dd;
    rand_min_out_degree = static_cast <uint32_t> ((out_degree_rand * static_cast <float> (rand()) / static_cast <float> (RAND_MAX))) + min_out_degree;

    // Reset the distance state, out degree state, and start index state.
    md = 0.0;
    out_degree = tp->get_out_degree();
    interval_start = tp->get_index();

    trajectory::Index interval_end = interval_start;
    geo::EdgeCPtr eptr = tp->get_fit_edge();
    RevTrajectoryIterator edge_start = start;
    RevTrajectoryIterator last;

    for (auto tp_it = start; tp_it != end; ++tp_it)
    {
        tp = *tp_it;
        last = tp_it;

        // Set the end of the interval to the current trip point.
        interval_end = tp->get_index();

        if (tp->get_critical_interval()) 
        {
            // The trip point ran into another crtiical interval.
            // Everything up to this point is a privacy interval.
            interval_list.push_back( std::make_shared<trajectory::Interval>( interval_end, interval_start + 1, "backward:ci" ) );
            
            return;
        }

        if (tp->get_index() == last_pi_end)
        {
            // The trip point ran into another privacy interval.
            // Everything up to this point is a privacy interval.
            interval_list.push_back( std::make_shared<trajectory::Interval>( interval_end, interval_start + 1, "backward:pi" ) );

            return;
        }
    
        geo::EdgeCPtr tp_eptr = tp->get_fit_edge();

        if (is_edge_change(tp_eptr, eptr)) 
        {
            // The edge changed.
            // Handle the change by looking at the current and previous edges.
            if (handle_edge_change( edge_start, tp_it, eptr ))
            {
                return;
            }
        
            // Update the local edge state.
            edge_start = tp_it;
            eptr = tp_eptr;
        }
    }

    // There are no more trip points.
    // Check if the distance metrics have been met on the current edge.
    trajectory::Index edge_end = find_interval_end( edge_start, last );
    
    if (interval_end != edge_end)
    {
        interval_list.push_back( std::make_shared<trajectory::Interval>( edge_end, interval_start + 1, "backward:max_md" ) );
    }
    else
    {
        interval_list.push_back( std::make_shared<trajectory::Interval>( interval_end, interval_start + 1, "backward:end" ) );
    }
}

bool PrivacyIntervalFinder::handle_edge_change( const RevTrajectoryIterator prev, const RevTrajectoryIterator curr, const geo::EdgeCPtr& eptr )
{
    double edge_distance;
    double direct_distance;
    trajectory::Index interval_end = 0;

    trajectory::Point::Ptr prev_tp = *prev;
    trajectory::Point::Ptr curr_tp = *curr;

    // Get the direct distance after changing to this edge.
    direct_distance = init_priv_point->distance_to( *curr_tp );

    // Determine the previous edge type.
    if (!prev_tp->is_explicitly_fit())
    {
        // The previous edge was implicit.
        // Check if the max distance metric is met by traversing the edge.
        edge_distance = eptr->length();
    
        if (edge_distance + md >= max_md || direct_distance >= max_dd)
        {
            // This distance metric was met by tarversing the edge.
            // Find the interval end within the edge and add the 
            // interval to the list.
            interval_end = find_interval_end( prev, curr );
            interval_list.push_back( std::make_shared<trajectory::Interval>( interval_end, interval_start + 1, "backward:max_dist" ) );

            return true;
        }
    }
    else
    {
        // The previous edge was explicit.
        // Compute the out degree from the traversing the fit edge.
        uint32_t edge_out_degree = out_degree - curr_tp->get_out_degree();
       
        // Check if the current edge is explicit or implicit.
        if (curr_tp->is_explicitly_fit()) 
        {
            // The current edge is explicit.
            // The edge distance is the length of the edge.
            edge_distance = eptr->length();
        }
        else
        {
            // The current edge is not explicit.
            // The edge distance is the trajectory's traversal on the previous
            // edge.
            edge_distance = prev_tp->distance_to( *curr_tp );
        }
        
        // Check the conditions for a privacy interval.
        if (edge_distance + md >= rand_min_md && direct_distance >= rand_min_dd && edge_out_degree >= rand_min_out_degree) 
        {
            // The min metrics were met by by traversing the edge.
            // Because the out degree metric can only be met after traversing
            // the edge, the interval end must be the current trip point.
            interval_end = curr_tp->get_index();
            interval_list.push_back( std::make_shared<trajectory::Interval>( interval_end, interval_start + 1, "backward:min" ) );

            return true;
        }
        else if (edge_distance + md >= max_md || direct_distance >= max_dd) 
        {
            // A max distance metric was met by the traversal.
            // Find the interval within the edge and add the interval
            // to the list.
            interval_end = find_interval_end( prev, curr );
            interval_list.push_back( std::make_shared<trajectory::Interval>( interval_end, interval_start + 1, "backward:max_md" ) );
        
            return true;
        }
        else if (edge_out_degree >= max_out_degree) 
        {
            // The max out degree metric was met by the traversal.
            // Set the interval.
            interval_list.push_back( std::make_shared<trajectory::Interval>( curr_tp->get_index(), interval_start + 1, "backward:max_out_degree" ) );
 
            return true;
        }
    }

    // No metrics were met.
    // Update the Manhatten distance with the edge distance.
    md += edge_distance;

    return false;
}

trajectory::Index PrivacyIntervalFinder::find_interval_end( const RevTrajectoryIterator start, const RevTrajectoryIterator end )
{
    double edge_distance;
    double direct_distance;
    trajectory::Point::Ptr curr_tp;

    trajectory::Point::Ptr tp = *start;

    for (auto tp_it = std::next( start, 1 ); tp_it != end; ++tp_it)
    {
        curr_tp = *(tp_it);
        edge_distance = tp->distance_to( *curr_tp );
        direct_distance = init_priv_point->distance_to( *curr_tp );
        
        if (md + edge_distance > max_md || direct_distance > max_dd) 
        {
            return curr_tp->get_index();
        }
    }
    
    return (*end)->get_index();
}

/***************************Privacy Interval Marker****************************/
PrivacyIntervalMarker::PrivacyIntervalMarker( const std::initializer_list<trajectory::Interval::PtrList> list ) :
    privacy_interval{ 0 },
    iptr{ nullptr }
{
    merge_intervals( list );
    set_next_interval();
}

bool PrivacyIntervalMarker::compare( trajectory::IntervalCPtr a, trajectory::IntervalCPtr b )
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

void PrivacyIntervalMarker::merge_intervals( const std::initializer_list<trajectory::Interval::PtrList> list )  
{
    trajectory::Interval::PtrList sorted_intervals;

    // Combine all the interval lists into one list.
    for (auto& interval_list : list)
    {
        sorted_intervals.insert( sorted_intervals.end(), interval_list.begin(), interval_list.end() );
    }

    // Check there are 1 or less intervals.
    if (sorted_intervals.size() <= 0) {
        // There are one or less interval.
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

void PrivacyIntervalMarker::set_next_interval()
{
    if (privacy_interval >= intervals.size()) 
    {   
        iptr = nullptr;

        return;
    }
    
    iptr = intervals[privacy_interval++];
}

void PrivacyIntervalMarker::mark_trajectory( trajectory::Trajectory& traj ) 
{
    for (auto& tp : traj) {
        mark_trip_point( tp );
    }
}

void PrivacyIntervalMarker::mark_trip_point( trajectory::Point::Ptr& tp ) 
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
        // Set the interval as private for the trip point.
        tp->set_private();

        return;
    }

    // The trip point is before the interval.
    // Check the next trip point.
}

/****************************DeIdentifier**************************************/
const trajectory::Trajectory& DeIdentifier::de_identify( const trajectory::Trajectory& traj )
{
    for (auto& tp : traj)
    {
        if (tp->is_critical() || tp->is_private())
        {
            continue;
        }

        new_traj.push_back(tp);
    }

    return new_traj;
}

const trajectory::Trajectory& DeIdentifier::de_identify( const trajectory::Trajectory& traj, std::shared_ptr<instrument::PointCounter> point_counter )
{
    for (auto& tp : traj)
    {
        if (tp->is_critical())
        {
            point_counter->n_ci_points++;           
 
            continue;
        }

        if (tp->is_private())
        {
            point_counter->n_pi_points++;           

            continue;
        }

        new_traj.push_back(tp);
    }

    return new_traj;
}
