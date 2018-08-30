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
#include <iostream>

#include "cvdi.hpp"

namespace cvdi {

AreaFitter::AreaFitter( double fit_width_scaling, double fit_extension, uint32_t num_sectors, uint32_t min_fit_points  ) :
    fit_width_scaling_{fit_width_scaling},
    fit_extension_{ fit_extension },
    next_edge_id{ 0 },
    num_sectors{ num_sectors },
    sector_size{ 360.0 / num_sectors },
    min_fit_points{ min_fit_points },
    current_sector{ 0 },
    num_fit_points{ 0 },
    current_implicit_edge_{nullptr},
    current_matched_edge_{nullptr},
    current_matched_area_{nullptr}
{}

uint32_t AreaFitter::get_sector( geo_data::Sample::Ptr sample ) const
{
    // 360 / Y.  This is 
    // double/double converted to int (floored by conversion).
    uint32_t sector = static_cast<uint32_t>(std::floor(sample->azimuth() / sector_size));

    // in case of headings greater than or equal to 360.
    return sector % num_sectors;
}

/**
 * Predicate that determines when we need a new implicit area.
 *
 * Heading must have changed to new sector AND a sufficient number of point needs to be collected.
 *
 */
bool AreaFitter::is_edge_change ( uint32_t sector ) const
{
    return ( current_sector != sector && num_fit_points > min_fit_points );
}

void AreaFitter::fit ( geo_data::Sample::Ptr sample )
{
    geo::Edge::Ptr matched_edge = sample->matched_edge();

    if (matched_edge != nullptr) {
        // This point is matched to an edge.
        if (current_matched_area_ == nullptr || matched_edge->id() != current_matched_edge_->id()) {
            current_matched_area_ = std::make_shared<geo::Area>(matched_edge->line_string, matched_edge->width() * fit_width_scaling_, fit_extension_);

            if (current_matched_area_->is_valid()) {
                current_matched_edge_ = matched_edge;
                explicit_edge_set.insert(matched_edge); 
            }
        }

        if (current_matched_edge_ != nullptr && current_matched_area_->is_within(sample->point)) {
            // the point is within the area edge
            sample->fit_edge(matched_edge);
            sample->is_explicit_fit(true);

            // reset implicit state
            current_implicit_edge_ = nullptr;
            num_fit_points = 0;
        
            return;
        }
    }

    // this point is NOT explicitly fit.

    if (current_implicit_edge_ == nullptr) {
        // Initialize the implicitly fit edge.
        current_sector = get_sector( sample );

        // pointer to new implicit edge.
        current_implicit_edge_ = std::make_shared<geo::Edge>( next_edge_id );
        current_implicit_edge_->line_string.setNumPoints(2);
        current_implicit_edge_->line_string.setPoint(0, sample->point.getX(), sample->point.getY());
        current_implicit_edge_->line_string.setPoint(1, sample->point.getX(), sample->point.getY());
        implicit_edge_set.insert( current_implicit_edge_ );
        ++next_edge_id;
        num_fit_points = 1;
        sample->fit_edge( current_implicit_edge_ );
        sample->is_explicit_fit(false);

    } else {            // previous point was implicitly fit.

        int sector = get_sector( sample );

        if (is_edge_change( sector )) {
            // implicit edge change criteria met, reset and build a new implicit
            // edge.
            // finalize the previous implicit area/edge.
            current_implicit_edge_->line_string.setPoint(1, sample->point.getX(), sample->point.getY());

            // pointer to new implicit edge starting where the old one left off.
            current_implicit_edge_ = std::make_shared<geo::Edge>( next_edge_id );
            current_implicit_edge_->line_string.setNumPoints(2);
            current_implicit_edge_->line_string.setPoint(0, sample->point.getX(), sample->point.getY());
            current_implicit_edge_->line_string.setPoint(1, sample->point.getX(), sample->point.getY());
            implicit_edge_set.insert( current_implicit_edge_ );
            ++next_edge_id;
            num_fit_points = 1;
            current_sector = sector;
        } else {
            // implicit edge is still valid, update the current implicit edge.
            current_implicit_edge_->line_string.setPoint(1, sample->point.getX(), sample->point.getY());
            ++num_fit_points;
        }

        sample->fit_edge( current_implicit_edge_ );
        sample->is_explicit_fit(false);
    }
}

void AreaFitter::fit ( geo_data::Sample::Trace& trace )
{
    for (auto& sample : trace) {
        if (!sample->is_valid()) {
            continue;
        }

        fit( sample );
    }

    // I am storing pointers to all the implicit edges and those pointers are built as they sit in the set.
    // Here we are taking all those fully built implicit edges and creating the associated areas from them.
    // so we can plot in KML.
    geo::Area::Ptr area;

    for (auto& eptr : implicit_edge_set) {
        area = std::make_shared<geo::Area>(eptr->line_string, 10.0); 
        
        if (area->is_valid()) {
            implicit_area_set.insert( area );
        }
    }

    for (auto& eptr : explicit_edge_set) {
        area = std::make_shared<geo::Area>(eptr->line_string, eptr->width() * fit_width_scaling_, fit_extension_); 

        if (area->is_valid()) {
            explicit_area_set.insert( area );
        }
    }
}

IntersectionCounter::IntersectionCounter() :
    current_eptr{nullptr},
    is_last_edge_{false},
    last_edge_{0},
    cumulative_outdegree{ 0 }
{}

void IntersectionCounter::count_intersections( geo_data::Sample::Trace& trace )
{
    for (auto& sample : trace) {
        // each trip point is annotated with the cumulative intersection count.
        uint32_t d = current_count( sample );
        sample->out_degree( d );
    }
}

uint32_t IntersectionCounter::edge_out_degree(geo::Edge::Ptr edge) {
    uint32_t out_degree = 0;
    geo::Edge::Ptr successor = edge->successor();
    geo::Edge::Ptr next = successor;
    geo::Edge::Ptr neighbor = nullptr;

    // count number of neighbor edges of succesor
    while (next != nullptr) {
        out_degree++; 
        neighbor = next->neighbor();
        next = successor == neighbor ? nullptr : neighbor;
    }

    return out_degree > 1 ? out_degree : 0;
}

uint32_t IntersectionCounter::current_count( geo_data::Sample::Ptr sample )
{
    long shared_vertex;
    bool is_shared_vertex = false;
    geo::Edge::Ptr matched_edge = sample->matched_edge();

    if ( matched_edge == nullptr || !sample->is_explicit_fit()) {
        // This point is not matched to an OSM segment, keep the previous outdegree count.
        return cumulative_outdegree;
    }

    if (current_eptr) {
        // the counter was working with an edge previously.
        if ( current_eptr->road()->id() != matched_edge->road()->id() ) {
            // the counter was working with an edge previously.
            // edge change. update degree.
   
            // since we don't track direction of travel, just check all four
            // cases of vertex matches.

            if ( current_eptr->source() == matched_edge->source() || current_eptr->source() == matched_edge->target()) {
                shared_vertex = current_eptr->source();
                is_shared_vertex = true;
            } else if ( current_eptr->target() == matched_edge->source() || current_eptr->target() == matched_edge->target()) {
                shared_vertex = current_eptr->target();
                is_shared_vertex = true;
            }

            if ( is_shared_vertex ) {
                // we have a vertex usable to update the cumulative outdegree count.
                if ( !is_last_edge_ || last_edge_ != matched_edge->road()->id() ) {
                    // Case 1: Update with this shared vertex since we did not establish a previous shared vertex.
                    // Case 2: Update with this shared vertex since it is different from the previous shared vertex.
                    // In both cases, this is a new intersection.
                    cumulative_outdegree += edge_out_degree(current_eptr);
                    last_edge_ = current_eptr->road()->id();
                    is_last_edge_ = true;
                } // otherwise pathelogical case: we are seeing the last vertex AGAIN.

            } // otherwise case: we've been fit to a disconnected edge; bad data?

            current_eptr = matched_edge;

        } // otherwise case: on the same edge; no update to degree.

    } else {
        // current_eptr == nullptr, since we have an explicitly fit edge.  doesn't change the cumulative degree.
        current_eptr = matched_edge;
    }

    return cumulative_outdegree;
}

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

geo_data::Interval::PtrList& TurnAround::find_turn_arounds( const geo_data::Sample::Trace& trace ) {
    for (auto& sample : trace) {
        if (!sample->is_valid()) {
            continue;
        }

        update_turn_around_state( sample );
    }

    return interval_list;
}

void TurnAround::update_turn_around_state( geo_data::Sample::Ptr sample ) {
    geo::Edge::Ptr fit_edge = sample->fit_edge();

    if (sample->is_explicit_fit()) {
        // The trip point is fit to an explicit edge.
        // Check if the previous trip point has been fit and set the fit 
        // exit trip point.
        if (!is_previous_trip_point_fit) {
            // The previous trip point has not been explicitly fit.
            // Check if there is a heading change between this trip point the
            // the previous fit exit trip point.
            if (is_fit_exit && geo::Spatial::heading_delta( fit_exit_point->azimuth(), sample->azimuth()) >= heading_delta) {
                // There is a change in fit trajectory headings.
                // This is a critical interval.
                interval_list.push_back( std::make_shared<geo_data::Interval>( fit_exit_point->index(), sample->index(), "ta_fit", kCriticalIntervalType ) );
            }

            current_edge = nullptr;
            area_q.clear();
            is_previous_trip_point_fit = true;
        }
        
        is_fit_exit = true;
        fit_exit_point = sample;
    } else if (!current_edge) {
        // There is no previous edge.
        // Set the edge to the fit edge of the trip point, and reset the
        // previous fit trip point flag.
        current_edge = fit_edge;
        is_previous_trip_point_fit = false;
    } else {
        // There is a previous point.
        // Check for a critical interval and check if the edge has changed.
        if (is_critical_interval( sample )) {
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

        if (current_edge->id() != fit_edge->id()) {
            // The edge has changed.
            // Add the edge to the area queue and update the edge.
            geo::Area::Ptr aptr = std::make_shared<geo::Area>(current_edge->line_string, area_width, 0.0);
            
            if (aptr->is_valid()) {
                area_q.push_front( std::make_shared<AreaIndexPair>( aptr, sample->index() ));

                if (area_q.size() >= max_q_size) {
                    area_q.pop_back();
                }
            }

            current_edge = fit_edge;
        }
    }
}

bool TurnAround::is_critical_interval( geo_data::Sample::Ptr sample ) {
    bool first = true;

    for (auto& atptr : area_q) {
        if (first) {
            first = false;

            continue;
        }

        if (atptr->first->is_within( sample->point ) && sample->speed() < max_speed) {
            area_set.insert( atptr->first );
            interval_list.push_back( std::make_shared<geo_data::Interval>(atptr->second, sample->index(), "ta", kCriticalIntervalType));

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
    return (*q.back())->index() - (*q.front())->index() + 1;
}

/**
 * Return the time period covered by points within the deque.  This is not meant to be used by the logic that finds
 * stops.
 */
uint64_t Stop::Deque::delta_time() const
{
    if (q.empty()) return 0;
    return (*q.back())->timestamp() - (*q.front())->timestamp();
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
bool Stop::Deque::under_speed( geo_data::Sample::Ptr ptptr ) const
{
    return ptptr->speed() < stop_detector.max_speed;
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

    geo_data::Sample::Ptr f = *q.front();
    geo_data::Sample::Ptr b = *q.back();

    return spatial_.distance(f->point, b->point);
}

/**
 * Predicate indicating whether the deque's INVARIANT would be broken by adding this trip point to it.
 *
 * INVARIANT: The time elapsed from first trip point in deque to last never exceeds max_time.
 */
bool Stop::Deque::under_time( geo_data::Sample::Ptr ptptr ) const
{
    uint64_t time_period = static_cast<uint64_t>(ptptr->timestamp() - (*q.front())->timestamp());
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
        return (*q.front())->index();
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
        return (*q.back())->index();
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
void Stop::Deque::push_right( const geo_data::Sample::Trace::const_iterator& it )
{
    if ( !q.empty() ) {
        // Compute the distance covered between the last point and the newly added point.
        double dd = spatial_.distance( (*it)->point, (*(q.back()))->point ); 
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
geo_data::Sample::Trace::const_iterator Stop::Deque::pop_left()
{
    // remove the oldest point from the left side of the deque.
    geo_data::Sample::Trace::const_iterator it = q.front();
    q.pop_front();

    if ( q.size() > 1 ) {   // some cumulative distance remains to be tracked.
        // update the cumulative distance by subtracting the straight-line distance between the point
        // removed and the new back of the deque.  We know the deque is not empty.
        cumulative_distance -= spatial_.distance( (*it)->point, (*(q.back()))->point );

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

Stop::HighwaySet Stop::excluded_highways { 
    { 
        101, // motorway
        104, // trunc
        106, // primary
        102, // motorway link
        105, // trunk link
        107 // primary link 
    }
};

geo_data::Index Stop::set_excluded_highways( const Stop::HighwaySet& excludes )
{
    excluded_highways.clear();
    excluded_highways.insert( excludes.begin(), excludes.end() );
    return excluded_highways.size();
}

geo_data::Index Stop::add_excluded_highway( long highway )
{
    excluded_highways.insert( highway );
    return excluded_highways.size();
}

/**
 * Predicate that returns true if the road the point is fit to is IMPLICIT (unknown) or it is NOT in the blacklist.
 * Return false if the road the trip point is on is EXPLICIT and fit to a black list road -- we ignore stops on
 * those roads.
 */
bool Stop::valid_highway( geo_data::Sample::Ptr ptptr )
{
    if (ptptr->is_explicit_fit()) {           // this trip point has an OSM way type.
        
        long hw = ptptr->fit_edge()->type();
        auto it = excluded_highways.find( hw );

        return (it == excluded_highways.end());   // not found in blacklist, so this point is on a road that can't be ignored.
    }

    // if the trip point is not explicitly fit then we assume we should check for stops.
    return true;
}

Stop::Stop( uint64_t max_time, double min_distance, double max_speed ) :
    max_time { 1000 * max_time },
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
geo_data::Interval::PtrList& Stop::find_stops( const geo_data::Sample::Trace& trace )
{
    Stop::Deque q{*this};

    // made a const_iterator so we can keep the traj const-correctness.
    geo_data::Sample::Trace::const_iterator t_it = trace.begin();

    while ( t_it != trace.end() ) {                  // outer loop.

        // only investigate trip points that are under max_speed and on the right kind of roads.
        if ( q.under_speed( *t_it ) && valid_highway( *t_it ) ) {

            q.push_right( t_it );                   // this should always be the first point into the q.
            ++t_it;                                 // we work with a non-empty q and a point in hand in the second phase.

            while ( t_it != trace.end() ) {          // inner loop to maximize the deque's invariant time condition.

                if ( q.under_time( *t_it ) ) {      // time of current point - oldest point in deque <= max_time.

                    
                    q.push_right( t_it );           // invariant continues to hold based on the check just done.
                    ++t_it;                         // new point in hand.

                } else {                            // this point will break the invariant condition, so check for distance.

                    if ( q.under_distance() ) {     // distance covered in the deque <= minimum distance parameter; CI detected.

                        // critical interval to save.  The entire deque, so it is empty.
                        geo_data::Interval::Ptr ciptr =  std::make_shared<geo_data::Interval>( geo_data::Interval{ q.left_index(), q.right_index(), "stop", kCriticalIntervalType } );
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

/******************************** StartEndIntervals ************************************************/
    
const geo_data::Interval::PtrList& StartEndIntervals::get_start_end_intervals( const geo_data::Sample::Trace& trace ) 
{
    if (intervals.size() == 2) {
        return intervals;
    }
   
    geo_data::Index second_to_last_index = trace.size() > 0 ? trace.size() - 1 : 0;
    intervals.push_back( std::make_shared<geo_data::Interval>( 0, 1, "start_pt", kCriticalIntervalType ) );
    intervals.push_back( std::make_shared<geo_data::Interval>( second_to_last_index, second_to_last_index + 1, "end_pt", kCriticalIntervalType ) );

    return intervals;
} 

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

bool PrivacyIntervalFinder::is_edge_change(geo::Edge::Ptr a, geo::Edge::Ptr b) const {
    bool a_implicit = a->type() == -1;
    bool b_implicit = a->type() == -1;

    if ((a_implicit && !b_implicit) || (!a_implicit && b_implicit)) {
        return true;
    }

    return a->id() != b->id();
}

const geo_data::Interval::PtrList& PrivacyIntervalFinder::find_intervals( geo_data::Sample::Trace& trace ) 
{
    for (curr_tp_it = trace.begin(); curr_tp_it != trace.end(); ++curr_tp_it) 
    {
        update_intervals( curr_tp_it, trace );
    }

    return interval_list;
}

void PrivacyIntervalFinder::update_intervals( const TrajectoryIterator tp_it, geo_data::Sample::Trace& trace ) 
{
    geo_data::Sample::Ptr sample = *tp_it;
    geo_data::Interval::Ptr ciptr = sample->interval();
    geo_data::Index index = sample->index();

    if (curr_ciptr == nullptr) 
    {
        // The previous trip point was not within a critical interval.
        // Check if the current trip point is within a critical interval.
        if (ciptr != nullptr) 
        {
            // The current trip point is within a critical interval.
            // Update the critical interval state and check the trip point 
            // index.
            curr_ciptr = ciptr;
            
            if (index > 0 && sample->index() > last_pi_end)
            {
                // This is not the first trip point and there is at least one
                /// point prior to this that is not within a privacy interval.
                // Find privacy points prior to the start of this interval.
                find_interval( std::reverse_iterator<geo_data::Sample::Trace::iterator>( tp_it ), std::reverse_iterator<geo_data::Sample::Trace::iterator>( trace.begin() ) );
            }
        }
    } 
    else if (ciptr == nullptr) 
    {
        // The current trip point is no longer within a critical interval.
        // Update the critical interval state and check the trip point.
        curr_ciptr = nullptr;

        if (index + 1 < trace.size()) 
        {
            // This is not the last trip point.
            // Find privacy points after the start of this interval.
            find_interval( tp_it, trace.end() );
        } 
    }
    else if (ciptr != curr_ciptr)
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
    geo_data::Sample::Ptr sample = *start;
    init_priv_point = sample;
    rand_min_md = (md_rand * static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) + min_md;
    rand_min_dd = (dd_rand * static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) + min_dd;
    rand_min_out_degree = static_cast <uint32_t> ((out_degree_rand * static_cast <float> (rand()) / static_cast <float> (RAND_MAX))) + min_out_degree;

    // Reset the distance state, out degree state, and start index state.
    md = 0.0;
    out_degree = sample->out_degree();
    interval_start = sample->index();

    geo_data::Index interval_end = interval_start;
    geo::Edge::Ptr eptr = sample->fit_edge();
    TrajectoryIterator edge_start = start;
    TrajectoryIterator last;

    for (auto tp_it = start; tp_it != end; ++tp_it)
    {
        sample = *tp_it;
        last = tp_it;

        // Set the end of the interval to the current trip point.
        interval_end = sample->index();

        if (sample->interval() != nullptr) 
        {
            // The trip point ran into another crtiical interval.
            // Everything up to this point is a privacy interval.
            last_pi_end = interval_end;
            curr_tp_it += (interval_end - interval_start) - 1; 
            interval_list.push_back( std::make_shared<geo_data::Interval>( interval_start, interval_end, "forward:ci", kPrivacyIntervalType ) );
		
          	return;
        }

        geo::Edge::Ptr tp_eptr = sample->fit_edge();

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
    geo_data::Index edge_end = find_interval_end( edge_start, last );

    if (edge_end != interval_end) 
    {
        last_pi_end = edge_end;
        curr_tp_it += (edge_end - interval_start) - 1; 
        interval_list.push_back( std::make_shared<geo_data::Interval>( interval_start, edge_end, "forward:max_dist", kPrivacyIntervalType ) );
    }
    else 
    {
        curr_tp_it += (interval_end - interval_start) - 1; 
        interval_list.push_back( std::make_shared<geo_data::Interval>( interval_start, interval_end, "forward:end", kPrivacyIntervalType ) );
    }
}

bool PrivacyIntervalFinder::handle_edge_change( const TrajectoryIterator prev, const TrajectoryIterator curr, geo::Edge::Ptr eptr )
{
    double edge_distance;
    double direct_distance;
    geo_data::Index interval_end = 0;

    geo_data::Sample::Ptr prev_tp = *prev;
    geo_data::Sample::Ptr curr_tp = *curr;

    // Get the direct distance after changing to this edge.
    direct_distance = spatial_.distance(init_priv_point->point, curr_tp->point);

    // Determine the previous edge type.
    if (!prev_tp->is_explicit_fit())
    {
        // The previous edge was implicit.
        // Check if the max distance metric is met by traversing the edge.
        edge_distance = spatial_.length(eptr->line_string);
    
        if (edge_distance + md >= max_md || direct_distance >= max_dd )
        {
            // This distance metric was met by tarversing the edge.
            // Find the interval end within the edge and add the 
            // interval to the list.
            interval_end = find_interval_end( prev, curr );
            last_pi_end = interval_end;
            curr_tp_it += (interval_end - interval_start) - 1;
            interval_list.push_back( make_interval(interval_start, interval_end, "forward:max_dist", direct_distance, edge_distance + md, curr_tp->out_degree() - out_degree) );

            return true;
        }
    }
    else
    {
        // The previous edge was explicit.
        // Compute the out degree from the traversing the fit edge.
        uint32_t edge_out_degree = curr_tp->out_degree() - out_degree;
       
        // Check if the current edge is explicit or implicit.
        if (curr_tp->is_explicit_fit()) 
        {
            // The current edge is explicit.
            // The edge distance is the length of the edge.
            edge_distance = spatial_.length(eptr->line_string);
        }
        else
        {
            // The current edge is not explicit.
            // The edge distance is the trajectory's traversal on the previous
            // edge.
            edge_distance = spatial_.distance(prev_tp->point, curr_tp->point);
        }
        
        // Check the conditions for a privacy interval.
        if (edge_distance + md >= rand_min_md && direct_distance >= rand_min_dd && edge_out_degree >= rand_min_out_degree) 
        {
            // The min metrics were met by by traversing the edge.
            // Because the out degree metric can only be met after traversing
            // the edge, the interval end must be the current trip point.
            last_pi_end = curr_tp->index();
            curr_tp_it += (last_pi_end - interval_start) - 1;
            interval_list.push_back( make_interval(interval_start, last_pi_end, "forward:min", direct_distance, edge_distance + md, edge_out_degree) );

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
            interval_list.push_back( make_interval(interval_start, interval_end, "forward:max_dist", direct_distance, edge_distance + md, edge_out_degree) );
        
            return true;
        }
        else if (edge_out_degree >= max_out_degree) 
        {
            // The max out degree metric was met by the traversal.
            // Set the interval.
            last_pi_end = curr_tp->index();
            curr_tp_it += (last_pi_end - interval_start) - 1;
            interval_list.push_back( make_interval(interval_start, last_pi_end, "forward:max_out_degree", direct_distance, edge_distance + md, edge_out_degree) );
 
            return true;
        }
    }

    // No metrics were met.
    // Update the Manhatten distance with the edge distance.
    md += edge_distance;

    return false;
}

geo_data::Index PrivacyIntervalFinder::find_interval_end( const TrajectoryIterator start, const TrajectoryIterator end ) 
{
    double edge_distance;
    double direct_distance;
    geo_data::Sample::Ptr curr_sample;

    geo_data::Sample::Ptr sample = *start;

    for (auto tp_it = std::next( start, 1 ); tp_it != end; ++tp_it)
    {
        curr_sample = *(tp_it);
        edge_distance = spatial_.distance(sample->point, curr_sample->point);
        direct_distance = spatial_.distance(init_priv_point->point, curr_sample->point);
        
        if (md + edge_distance > max_md || direct_distance > max_dd) 
        {
            return curr_sample->index();
        }
    }
    
    return (*end)->index();
}

/******************************Backward PI Routines****************************/

void PrivacyIntervalFinder::find_interval( const RevTrajectoryIterator start, const RevTrajectoryIterator end ) 
{
    geo_data::Sample::Ptr sample = *start;
    init_priv_point = sample;
    rand_min_md = (md_rand * static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) + min_md;
    rand_min_dd = (dd_rand * static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) + min_dd;
    rand_min_out_degree = static_cast <uint32_t> ((out_degree_rand * static_cast <float> (rand()) / static_cast <float> (RAND_MAX))) + min_out_degree;

    // Reset the distance state, out degree state, and start index state.
    md = 0.0;
    out_degree = sample->out_degree();
    interval_start = sample->index();

    geo_data::Index interval_end = interval_start;
    geo::Edge::Ptr eptr = sample->fit_edge();
    RevTrajectoryIterator edge_start = start;
    RevTrajectoryIterator last;

    for (auto tp_it = start; tp_it != end; ++tp_it)
    {
        sample = *tp_it;
        last = tp_it;

        // Set the end of the interval to the current trip point.
        interval_end = sample->index();

        if (sample->interval() != nullptr) 
        {
            // The trip point ran into another crtiical interval.
            // Everything up to this point is a privacy interval.
            interval_list.push_back( std::make_shared<geo_data::Interval>( interval_end, interval_start + 1, "backward:ci", kPrivacyIntervalType ) );
            
            return;
        }

        if (sample->index() == last_pi_end)
        {
            // The trip point ran into another privacy interval.
            // Everything up to this point is a privacy interval.
            interval_list.push_back( std::make_shared<geo_data::Interval>( interval_end, interval_start + 1, "backward:pi", kPrivacyIntervalType ) );

            return;
        }
    
        geo::Edge::Ptr tp_eptr = sample->fit_edge();

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
    geo_data::Index edge_end = find_interval_end( edge_start, last );
    
    if (interval_end != edge_end)
    {
        interval_list.push_back( std::make_shared<geo_data::Interval>( edge_end, interval_start + 1, "backward:max_dist", kPrivacyIntervalType ) );
    }
    else
    {
        interval_list.push_back( std::make_shared<geo_data::Interval>( interval_end, interval_start + 1, "backward:end", kPrivacyIntervalType ) );
    }
}

bool PrivacyIntervalFinder::handle_edge_change( const RevTrajectoryIterator prev, const RevTrajectoryIterator curr, geo::Edge::Ptr eptr )
{
    double edge_distance;
    double direct_distance;
    geo_data::Index interval_end = 0;

    geo_data::Sample::Ptr prev_tp = *prev;
    geo_data::Sample::Ptr curr_tp = *curr;

    // Get the direct distance after changing to this edge.
    direct_distance = spatial_.distance(init_priv_point->point, curr_tp->point);

    // Determine the previous edge type.
    if (!prev_tp->is_explicit_fit())
    {
        // The previous edge was implicit.
        // Check if the max distance metric is met by traversing the edge.
        edge_distance = spatial_.length(eptr->line_string);
    
        if (edge_distance + md >= max_md || direct_distance >= max_dd)
        {
            // This distance metric was met by tarversing the edge.
            // Find the interval end within the edge and add the 
            // interval to the list.
            interval_end = find_interval_end( prev, curr );
            interval_list.push_back( make_interval(interval_end, interval_start + 1, "backward:max_dist", direct_distance, edge_distance + md, out_degree - curr_tp->out_degree()) );

            return true;
        }
    }
    else
    {
        // The previous edge was explicit.
        // Compute the out degree from the traversing the fit edge.
        uint32_t edge_out_degree = out_degree - curr_tp->out_degree();
       
        // Check if the current edge is explicit or implicit.
        if (curr_tp->is_explicit_fit()) 
        {
            // The current edge is explicit.
            // The edge distance is the length of the edge.
            edge_distance = spatial_.length(eptr->line_string);
        }
        else
        {
            // The current edge is not explicit.
            // The edge distance is the trajectory's traversal on the previous
            // edge.
            edge_distance = spatial_.distance(prev_tp->point, curr_tp->point);
        }
        
        // Check the conditions for a privacy interval.
        if (edge_distance + md >= rand_min_md && direct_distance >= rand_min_dd && edge_out_degree >= rand_min_out_degree) 
        {
            // The min metrics were met by by traversing the edge.
            // Because the out degree metric can only be met after traversing
            // the edge, the interval end must be the current trip point.
            interval_end = curr_tp->index();
            interval_list.push_back( make_interval(interval_end, interval_start + 1, "backward:min", direct_distance, edge_distance + md, edge_out_degree) );

            return true;
        }
        else if (edge_distance + md >= max_md || direct_distance >= max_dd) 
        {
            // A max distance metric was met by the traversal.
            // Find the interval within the edge and add the interval
            // to the list.
            interval_end = find_interval_end( prev, curr );
            interval_list.push_back( make_interval(interval_end, interval_start + 1, "backward:max_dist", direct_distance, edge_distance + md, edge_out_degree) );
        
            return true;
        }
        else if (edge_out_degree >= max_out_degree) 
        {
            // The max out degree metric was met by the traversal.
            // Set the interval.
            interval_list.push_back( make_interval(curr_tp->index(), interval_start + 1, "backward:max_out_degree", direct_distance, edge_distance + md, edge_out_degree) );
 
            return true;
        }
    }

    // No metrics were met.
    // Update the Manhatten distance with the edge distance.
    md += edge_distance;

    return false;
}

geo_data::Index PrivacyIntervalFinder::find_interval_end( const RevTrajectoryIterator start, const RevTrajectoryIterator end )
{
    double edge_distance;
    double direct_distance;
    geo_data::Sample::Ptr curr_tp;

    geo_data::Sample::Ptr tp = *start;

    for (auto tp_it = std::next( start, 1 ); tp_it != end; ++tp_it)
    {
        curr_tp = *(tp_it);
        edge_distance = spatial_.distance(tp->point, curr_tp->point);
        direct_distance = spatial_.distance(init_priv_point->point, curr_tp->point);
        
        if (md + edge_distance > max_md || direct_distance > max_dd) 
        {
            return curr_tp->index();
        }
    }
    
    return (*end)->index();
}

geo_data::Interval::Ptr PrivacyIntervalFinder::make_interval(geo_data::Index start, geo_data::Index end, const std::string& tag, double dd, double mand, uint32_t od) {
    std::stringstream ss;
    ss << tag << "::" << dd << "::" << mand << "::" << od;
    
    return std::make_shared<geo_data::Interval>(start, end, ss.str(), kPrivacyIntervalType);
}

PointCounter::PointCounter() :
    n_points(0),
    n_invalid_field_points(0),
    n_invalid_geo_points(0),
    n_invalid_heading_points(0),
    n_ci_points(0),
    n_pi_points(0)
{}

PointCounter::PointCounter(uint64_t n_points, uint64_t n_invalid_field_points, uint64_t n_invalid_geo_points, uint64_t n_invalid_heading_points, uint64_t n_ci_points, uint64_t n_pi_points) : 
    n_points(n_points),
    n_invalid_field_points(n_invalid_field_points),
    n_invalid_geo_points(n_invalid_geo_points),
    n_invalid_heading_points(n_invalid_heading_points),
    n_ci_points(n_ci_points),
    n_pi_points(n_pi_points)
{}

PointCounter PointCounter::operator+(const PointCounter& other) const {
    PointCounter point_counter(n_points + other.n_points, n_invalid_field_points + other.n_invalid_field_points, n_invalid_geo_points + other.n_invalid_geo_points, other.n_invalid_heading_points + n_invalid_heading_points, n_ci_points + other.n_ci_points, n_pi_points + other.n_pi_points);      
    return point_counter;
}

std::ostream& operator<<(std::ostream& os, const PointCounter& point_counter) {
    return os << point_counter.n_points <<  "," << point_counter.n_invalid_field_points << "," << point_counter.n_invalid_geo_points << "," << point_counter.n_invalid_heading_points << "," << point_counter.n_ci_points << "," << point_counter.n_pi_points;
}

void count_points(const geo_data::Sample::Trace& raw_trace, PointCounter& counter_out) {
    geo_data::SampleError error_type;
    uint32_t interval_type;

    for (auto& sample : raw_trace) {
        counter_out.n_points++;

        if (!sample->is_valid()) {
            error_type = sample->error_type();
    
            if (error_type == geo_data::SampleError::FIELD) {
                counter_out.n_invalid_field_points++;
            } else if (error_type == geo_data::SampleError::GEO) {
                counter_out.n_invalid_geo_points++;
            } else if (error_type == geo_data::SampleError::HEADING) {
                counter_out.n_invalid_heading_points++;
            }
        } else if (sample->interval() != nullptr) {
            interval_type = sample->interval()->type();
    
            if (interval_type == kCriticalIntervalType) {
                counter_out.n_ci_points++;
            } else if (interval_type == kPrivacyIntervalType) {
                counter_out.n_pi_points++;
            }
        }
    }
}

}
