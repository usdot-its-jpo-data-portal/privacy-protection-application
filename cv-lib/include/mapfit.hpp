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
#ifndef CTES_DI_MAPFIT_HPP
#define CTES_DI_MAPFIT_HPP

#include "names.hpp"
#include "entity.hpp"
#include "trajectory.hpp"
#include "quad.hpp"

#include <functional>
#include <tuple>
#include <unordered_map>
#include <queue>

/**
 * \brief The map matching algorithm to use for our privacy procedures. 
 *
 * We use the following information to match OSM segments to trip points:
 *
 * 1. In some cases, the nearest segment is assumed to match. Near segments are found using a quad tree.
 * 2. When the previous points have been matched to a given segment, that segment is used to define an ecapsulating
 * area. Follow-on points within that area are assumed to match to the encapsulated segment.  The size of the area 
 * that contains the fit edge is controlled by the fit_width_scaling and fit_extension parameters.
 * 3. We use the graph connectivity of the segments to determine follow-on segments, i.e., points that match to a
 * segment are likely to match to connected segments NOT disconnected segments.
 * 4. In some cases, we use directionality of travel and road orientation to determine the correct road to match.
 **/
class MapFitter
{
    public:
        using AreaEdgePair = std::pair<geo::Area::Ptr, geo::EdgeCPtr>;
        using PriorityPair = std::pair<double, AreaEdgePair>;
        using AreaEdgePairList = std::vector<AreaEdgePair>;
        using AreaSet = std::unordered_set<geo::AreaCPtr>;
        using PriorityAreaQueue = std::priority_queue<PriorityPair, std::vector<PriorityPair>, std::function<bool(const PriorityPair&,const PriorityPair&)>>;

        /**
         * \brief Construct a map-matching instance.
         *
         * \param quadtree The quad tree data structure containing the OSM road network to match to.
         * \param fit_width_scaling A scaling factor to apply to the prescribed widths of various types of OSM roads,
         * e.g., 1.0 will use the prescribed width; 1.5 will increase that width by 50%.
         * \param fit_extension The number of meters to extend the bounding box on each end (meters)
         */
        MapFitter( const Quad::CPtr& quadtree, double fit_width_scaling = 1.0, double fit_extension = 5.0);

        /**
         * \brief Fit a trip point to a OSM segment.
         *
         * \param tp the trip point to match to an OSM road segment.
         */
        void fit( trajectory::Point& tp );        

        /**
         * \brief Fit an entire trip to an OSM road network.
         *
         * \param traj the trip to match to the road network stored in the quad tree.
         */
        void fit( trajectory::Trajectory& traj );

    private:
        Quad::CPtr quadtree;

        double fit_width_scaling;                   ///> applied to uniformly to all road type widths.
        double fit_extension;                       ///> distance (in meters) area is extended from ends of edge.

        geo::Area::Ptr current_area;                ///> the area that contained the last traj point or nullptr if no edge matched.
        geo::EdgeCPtr current_edge;                 ///> the edge that matched the last traj point.

        /**
         * \brief If the previous trip point was matched, return true (attempt to use it); otherwise, search the quad tree
         * for the set of nearest OSM edges.  All edges in this set whose encapsulating areas contain the trip point are considered.  
         * The edges that best aligns with the vehicle's direction of travel is selected.  Define the current_area and 
         * current_edge based on the best edge pulled from the quadtree.
         *
         * current_area set to a box surrounding an edge that contains the point OR to
         * nullptr if no such edge exists.
         *
         * \param tp The trip point that needs to be matched to a nearest road.
         * \param edges The list of edges to match to.
         * \return true if a match is made, false otherwise.
         */
        bool set_fit_area( const trajectory::Point& tp );

        /**
         * \brief Attempt to find the edge incident to shared_vertex that best matches the provided point. All
         * incident edges whose encapsulating areas contain the trip point are considered.  The edges that best aligns with the 
         * vehicle's direction of travel is selected.  Define the current_area and current_edge based on the best edge 
         * pulled from the quadtree.
         *
         * current_area set to a box surrounding an edge that contains the point OR to
         * nullptr if no such edge exists.
         *
         * \param tp The trip point that needs to be matched to a nearest road.
         * \param shared_vertex the vertex whose incident edges are to be used for matching.
         * \return true if a match is made, false otherwise.
         */
        bool set_fit_area( const trajectory::Point& tp, const geo::Vertex::Ptr shared_vertex);

        /**
         * \brief Attempt to find the edge from the set of entities provided that best matches the provided point. All
         * edges whose encapsulating areas contain the trip point are considered.  The edges that best aligns with the 
         * vehicle's direction of travel is selected.  Define the current_area and current_edge based on the best edge 
         * pulled from the quadtree.
         *
         * current_area set to a box surrounding an edge that contains the point OR to
         * nullptr if no such edge exists.
         *
         * \param tp The trip point that needs to be matched to a nearest road.
         * \param edges The list of edges to match to.
         * \return true if a match is made, false otherwise.
         */
        bool set_fit_area( const trajectory::Point& tp, const geo::Entity::PtrList& edges );

        static bool compare( const PriorityPair& p1, const PriorityPair& p2 );

    public:
        AreaSet area_set;
};

/**
 * \brief For privacy protection purposes, we infer roads when an OSM segment cannot be explicitly matched. This normally
 * happens in parking lots or structures.  It also happens when the trajectory travel is outside of the mapped area.
 * You could infer an entire road network based on trips traveled.
 */
class ImplicitMapFitter
{
    public:
        using AreaSet = std::unordered_set<geo::AreaCPtr>;

        /**
         * \brief Construct and Implicit Map Fitter.
         *
         * \param num_sectors the compass rose is divided into this number of equally sized sectors.
         * \param min_fit_points use no less than this many points to infer a road segment.
         */
        ImplicitMapFitter( uint32_t num_sectors = 36, uint32_t min_fit_points = 50 );


        /**
         * \brief Implicitly fit a trip point (make up a road).
         *
         * \param tp the trip point to match to an OSM road segment.
         */
        void fit( trajectory::Point& tp );

        /**
         * \brief Implicitly fit an entire trip (make up a set of roads).
         *
         * \param traj the trajectory to fit.
         */
        void fit( trajectory::Trajectory& traj );

    private:

        uint64_t next_edge_id;                          ///< The UID to use for the next implicit edge.
        uint32_t num_sectors;                           ///< The number of divisions of the compass rose to determine when to change implicit edge.
        double sector_size;                             ///< Size in degrees of a sector.
        uint32_t min_fit_points;                        ///< Use no less than this many points to infer a road segment.

        // state vars.
        uint32_t current_sector;                        ///< The compass rose sector the trajectory is currently in (between 0 and num_sectors - 1)
        uint32_t num_fit_points;                        ///< number of points implicitly fit so far.
        geo::EdgeCPtr current_eptr;                     ///< The implicit edge currently being "built."

        uint32_t get_sector( const trajectory::Point& tp ) const;
        bool is_edge_change( uint32_t heading_group ) const;

    public:
        geo::EdgeCPtrSet edge_set;                      ///< The complete set of implicit edges.
        AreaSet area_set;                               ///< The complete set of areas built from the implicit edges.
};

/**
 * \brief Annotate a trip with a cumulative count of intersection outdegree.
 */
class IntersectionCounter
{
    public:

        /**
         * \brief Construct an IntersectionCounter.
         */
        IntersectionCounter();

        /**
         * \brief Annotate traj with its cumulative intersection outdegree counts.
         */
        void count_intersections( trajectory::Trajectory& traj );

    private:
        geo::EdgeCPtr current_eptr;
        geo::Vertex::Ptr last_vertex_ptr;
        uint32_t cumulative_outdegree;

        uint32_t current_count( trajectory::Point& tp );

};

#endif
