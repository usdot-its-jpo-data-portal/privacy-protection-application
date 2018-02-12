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
#include "mapfit.hpp"
#include "entity.hpp"
#include "utilities.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <iomanip>
#include <iterator>
#include <queue>
#include <unordered_set>
#include <utility>

/******************************** MapFitter ************************************************/

MapFitter::MapFitter( const Quad::CPtr& quadtree, double fit_width_scaling, double fit_extension) :
    quadtree{ quadtree },
    fit_width_scaling{ fit_width_scaling },
    fit_extension{ fit_extension },
    area_set{}
{}

/**
 * Comparator that is used to order the map edge candidates based on how well they align with the current travel
 * direction of the vehicle.  See the code in trajectory.cpp for the details on how this value is computed.
 */
bool MapFitter::compare( const MapFitter::PriorityPair& p1, const MapFitter::PriorityPair& p2 )
{
    // smallest to largest.
    return p1.first > p2.first;
}

void MapFitter::fit( trajectory::Point& tp )
{
    if ( !set_fit_area( tp ) ) {
        // we are LOST... no current area and no edge to fit; could be no map data.
        return;
    }

    // current_area is set.
    if (current_area->contains( tp )) {

        // map matching.
        tp.set_fit_edge( current_edge );

    } else {
        // have a current_area, but it doesn't contain the traj point.  Attempt
        // to fit using the incident edges (uses the connectivity of the
        // roadways).

        // for the below consider the area positioned horizontally (along a line of latitude) and the vehicle moving
        // from left to right.

        if (current_area->outside_edge( 1, tp )) {
            // this trip point is to the left of the area.
            if (!set_fit_area( tp, current_edge->v2)) {
                // The trip point skipped an edge and no needs to hit the quad 
                // tree again
                current_area = nullptr;             // make sure this method doesn't just return.
                set_fit_area( tp );
            } 

        } else if (current_area->outside_edge( 3, tp )) {
            // this trip point is to the right of the area (edge 2).
            if (!set_fit_area( tp, current_edge->v1 )) {
                // The trip point skipped an edge and no needs to hit the quad 
                // tree again
                current_area = nullptr;             // make sure this method doesn't just return.
                set_fit_area( tp );
            } 
        } else {
            // this trip point is within the right and left edge ends of this area.
            // the driver has deviated "above" or "below" the area encapsulating the edge.
            current_area = nullptr;             // make sure this method doesn't just return.
            set_fit_area( tp );
        }

        if (current_area) {
            // There is current fit area for the point.
            // Add the fit edge to the point.
            tp.set_fit_edge( current_edge );
        }
    }
}

void MapFitter::fit( trajectory::Trajectory& traj )
{
    for (auto& tp : traj) {
        fit( *tp );
    }
} 

bool MapFitter::set_fit_area( const trajectory::Point& tp )
{
    if (current_area) return true;

    // don't have a current fit area; hit the quad tree and find one.
    return set_fit_area( tp, quadtree->retrieve_elements( tp ) );
}

bool MapFitter::set_fit_area( const trajectory::Point& tp, const geo::Entity::PtrList& entities )
{
    // assume no match
    bool successful_match = false;

    // an empty priority queue.
    PriorityAreaQueue priority_areas{ compare };

    current_area = nullptr;
    current_edge = nullptr;
        
    geo::EdgeCPtr eptr = nullptr;
    
    for (auto& entity_ptr : entities) {
        if (entity_ptr->get_entity_type() != geo::EntityType::EDGE) {
            // matching only happens with edge types.
            continue;
        }

        eptr = std::static_pointer_cast<const geo::Edge>(entity_ptr);
     
        geo::Area::Ptr aptr = nullptr;

        // build the area that encapsulates this edge using the OSM width information.
        try {
            aptr = eptr->to_area( eptr->get_way_width() * fit_width_scaling, fit_extension );

        } catch (geo::ZeroAreaException) {

            continue;
        }

        if ( aptr->contains( tp ) ) {
            
            // compute the POSITIVE error between the heading of the vehicle and this road segment.
            // this is how we prioritize selection of areas when there are multiple candidates.
            // this method ELIMINATES the effect of having heading and bearing 180 degree out from one another.
            double e = trajectory::Point::angle_error( tp.get_heading(), eptr->bearing() );

            // ordered with LEAST error between heading and bearing at the top of the queue.
            priority_areas.emplace( e, std::make_pair( aptr, eptr ) );
        }
    }

    if (!priority_areas.empty()) {
        // area condiates (edges) were found and the best one is at the top.
        auto& best = priority_areas.top();
        current_area = best.second.first;
        current_edge = best.second.second;
        area_set.insert(current_area);
        successful_match = true;
    }

    return successful_match;
}

/**
 * Attempt to find the edge that best fits the provided point by hitting the
 * quadtree.  Define the area (box) based on the best edge pulled from the
 * quadtree.
 *
 * current_area set to a box surrounding an edge that contains the point OR to
 * nullptr if no such edge exists.
 *
 * NOTE: The closest edge may not produce a box that contains the point.
 * NOTE: There could be MORE THAN ONE edge whose encapsulating areas contain the
 * trip point -- need a way to select the best one.
 *
 */
bool MapFitter::set_fit_area( const trajectory::Point& tp, const geo::Vertex::Ptr shared_vertex)
{
    bool successful_match = false;

    // an empty priority queue.
    PriorityAreaQueue priority_areas{ compare };

    current_area = nullptr;
    current_edge = nullptr;
    
    // Vertex of the non-shared portion of a candidate edge for the the fit.
    // used to prioritize the fit based on heading
    geo::Vertex::Ptr next_vertex = nullptr;

    for (auto& eptr : shared_vertex->get_incident_edges()) {
        geo::Area::Ptr aptr = nullptr;

        // build the area that encapsulates this edge using the OSM width information.
        try {
            aptr = eptr->to_area( eptr->get_way_width() * fit_width_scaling, fit_extension );

        } catch (geo::ZeroAreaException) {

            continue;
        }

        if ( aptr->contains( tp ) ) {

            // compute the POSITIVE error between the heading of the vehicle and this road segment.
            // this is how we prioritize selection of areas when there are multiple candidates.
            // this method ELIMINATES the effect of having heading and bearing 180 degree out from one another.
        
            if (eptr->v2 == shared_vertex) {
                next_vertex = eptr->v1; 
            } else {
                next_vertex = eptr->v2; 
            }
           
            double next_bearing = geo::Location::bearing(tp.lat, tp.lon, next_vertex->lat, next_vertex->lon);
            double e = trajectory::Point::angle_error( tp.get_heading(), next_bearing);

            // ordered with LEAST error between heading and bearing at the top of the queue.
            priority_areas.emplace( e, std::make_pair( aptr, eptr ) );
        }
    }


    if (!priority_areas.empty()) {
        // area condiates (edges) were found and the best one is at the top.
        auto& best = priority_areas.top();
        current_area = best.second.first;
        current_edge = best.second.second;
        area_set.insert(current_area);
        successful_match = true;
    }

    return successful_match;
}

/******************************** ImplicitMapFitter ************************************************/
ImplicitMapFitter::ImplicitMapFitter( uint32_t num_sectors, uint32_t min_fit_points  ) :
    next_edge_id{ 0 },
    num_sectors{ num_sectors },
    sector_size{ 360.0 / num_sectors },
    min_fit_points{ min_fit_points },
    current_sector{ 0 },
    num_fit_points{ 0 },
    current_eptr{},
    edge_set{},
    area_set{}
{}

uint32_t ImplicitMapFitter::get_sector( const trajectory::Point& tp ) const
{
    // 360 / Y.  This is 
    // double/double converted to int (floored by conversion).
    uint32_t sector = static_cast<uint32_t>(std::floor(tp.get_heading() / sector_size));

    // in case of headings greater than or equal to 360.
    return sector % num_sectors;
}

/**
 * Predicate that determines when we need a new implicit area.
 *
 * Heading must have changed to new sector AND a sufficient number of point needs to be collected.
 *
 */
bool ImplicitMapFitter::is_edge_change ( uint32_t sector ) const
{
    return ( current_sector != sector && num_fit_points > min_fit_points );
}

void ImplicitMapFitter::fit ( trajectory::Point& tp )
{
    if (tp.is_explicitly_fit()) {
        // This point is fit to an explicit edge.

        if (current_eptr != nullptr) {
            // reset the implicit fitter.
            // An edge is associated with the previous implicit fitting.

            // reset the implicit fitting state variables.
            current_eptr = nullptr;
            num_fit_points = 0;

        } // otherwise, this point and previous point were explicitly fit.

        return;
    }

    // this point is NOT explicitly fit.

    if (current_eptr == nullptr) {
        // Initialize the implicitly fit edge.
        current_sector = get_sector( tp );

        // pointer to new implicit edge.
        geo::Vertex v{ tp };
        current_eptr = std::make_shared<geo::Edge>( v, v, next_edge_id, false );
        edge_set.insert( current_eptr );
        ++next_edge_id;
        num_fit_points = 1;
        tp.set_fit_edge( current_eptr );

    } else {            // previous point was implicitly fit.

        int sector = get_sector( tp );

        if (is_edge_change( sector )) {
            // implicit edge change criteria met, reset and build a new implicit
            // edge.
            // finalize the previous implicit area/edge.
            current_eptr->v2->update_location( tp );

            // pointer to new implicit edge starting where the old one left off.
            current_eptr = std::make_shared<geo::Edge>( tp, tp, next_edge_id, false );
            edge_set.insert( current_eptr );
            ++next_edge_id;
            num_fit_points = 1;
            current_sector = sector;
        } else {
            // implicit edge is still valid, update the current implicit edge.
            current_eptr->v2->update_location( tp );
            ++num_fit_points;
        }

        tp.set_fit_edge( current_eptr );
    }
}

void ImplicitMapFitter::fit ( trajectory::Trajectory& traj )
{
    for (auto& tp : traj) {
        fit( *tp );
    }

    // I am storing pointers to all the implicit edges and those pointers are built as they sit in the set.
    // Here we are taking all those fully built implicit edges and creating the associated areas from them.
    // so we can plot in KML.
    for (auto& eptr : edge_set) {
        try 
        {
            area_set.insert( eptr->to_area( 10.0, 0.0 ) );
        }
        catch (geo::ZeroAreaException) 
        {
            continue;
        }
    }
}

IntersectionCounter::IntersectionCounter() :
    current_eptr{},
    last_vertex_ptr{},
    cumulative_outdegree{ 0 }
{}

void IntersectionCounter::count_intersections( trajectory::Trajectory& traj )
{
    for (auto& tp : traj) {
        // each trip point is annotated with the cumulative intersection count.
        unsigned int d = current_count( *tp );
        tp->set_out_degree( d );
    }
}

unsigned int IntersectionCounter::current_count( trajectory::Point& tp )
{
    geo::Vertex::Ptr shared_vertex_ptr = nullptr;

    if ( !tp.is_explicitly_fit() ) {
        // This point is not fit to an OSM segment, keep the previous outdegree count.
        return cumulative_outdegree;
    }

    // This point is fit to a road and we have an edge to work with.
    geo::EdgeCPtr tp_edge = tp.get_fit_edge();

    if (current_eptr) {
        // the counter was working with an edge previously.
        if ( current_eptr->get_uid() != tp_edge->get_uid() ) {
            // the counter was working with an edge previously.
            // edge change. update degree.
   
            // since we don't track direction of travel, just check all four
            // cases of vertex matches.

            if (( current_eptr->v1->uid == tp_edge->v1->uid ) ||
                    ( current_eptr->v1->uid == tp_edge->v2->uid ) ) {
                shared_vertex_ptr = current_eptr->v1;
            } else if (( current_eptr->v2->uid == tp_edge->v1->uid ) ||
                    ( current_eptr->v2->uid == tp_edge->v2->uid )) {
                shared_vertex_ptr = current_eptr->v2;
            }
            // the counter was working with an edge previously.

            // that this method will ignore...

            if ( shared_vertex_ptr ) {
                // we have a vertex usable to update the cumulative outdegree count.

                if ( !last_vertex_ptr || last_vertex_ptr->uid != shared_vertex_ptr->uid ) {
                    // Case 1: Update with this shared vertex since we did not establish a previous shared vertex.
                    // Case 2: Update with this shared vertex since it is different from the previous shared vertex.
                    // In both cases, this is a new intersection.
                    cumulative_outdegree += shared_vertex_ptr->outdegree();
                    last_vertex_ptr = shared_vertex_ptr;
                } // otherwise pathelogical case: we are seeing the last vertex AGAIN.

            } // otherwise case: we've been fit to a disconnected edge; bad data?

            current_eptr = tp_edge;

        } // otherwise case: on the same edge; no update to degree.

    } else {
        // current_eptr == nullptr, since we have an explicitly fit edge.  doesn't change the cumulative degree.
        current_eptr = tp_edge;
    }

    return cumulative_outdegree;
}
