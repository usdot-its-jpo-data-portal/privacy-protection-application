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
#ifndef HMM_MM_HPP
#define HMM_MM_HPP

#include "geo.hpp"
#include "geo_data.hpp"

/**
 * \brief Hidden Markov Model Map Matching
 */
namespace hmm_mm {

/**
 * \brief Transition object representing the path and probability between two sequential trace samples.
 */
class Transition {
    public:
        using Ptr = std::shared_ptr<Transition>;

        /**
         * \brief Construct a transition.
         * 
         * \param path pointer to a list of sequential edges representing a path
         * \patam transition_prob probability of the transition
         */
        Transition(geo::EdgeListPtr path, double transition_prob);

        /** \brief Probability of the transition. */
        double transition_prob;

        /** \brief Pointer to a list of sequential edges representing a path. */
        geo::EdgeListPtr path;
};

/**
 * \brief RoadPoint object representing a candidate edge for a sample. This is the core Hidden Markov Model structure, 
containing the emmission and transition probabilities, as well, as the intermediate probabilities (filter and sequence).
 */
class RoadPoint {
    public:
        using Ptr = std::shared_ptr<RoadPoint>;
        
        /** 
         * \brief Construct a road point.
         * 
         * \param edge_ptr pointer to the candidate edge
         * \param fraction fraction representing the part of the edge to which the sample is mapped
         */
        RoadPoint(geo::Edge::Ptr edge_ptr, double fraction);

        /** \brief The underlying GPS point on the edge. */
        OGRPoint geometry;

        /**
         * \brief Get the edge for this road point.
         * 
         * \return a pointer to edge
         */
        geo::Edge::Ptr edge() const;

        /**
         * \brief Get the predecessor candidate road point in the Hidden Markov Model.
         * 
         * \return pointer the predecessor candidate or nullptr if this is the first sample in the model
         */
        Ptr predecessor() const;

        /**
         * \brief Get the transition for this road point.
         * 
         * \return pointer to a Transition or nullptr if this is the first sample in the model
         */
        Transition::Ptr transition() const;

        /**
         * \brief Get the fraction of the edge to which the sample is mapped.
         * 
         * \return the fraction
         */
        double fraction() const;

        /**
         * \brief Get the emission probability of the sample for this road point. This is probability that the sample is mapped to edge.
         * 
         * \return the emission probability
         */
        double emission_prob() const;

        /**
         * \brief Get the filter probability of the sample for this road point.
         * 
         * \return the filter probability
         */
        double filter_prob() const;
    
        /**
         * \brief Get the sequence probability of the sample for this road point.
         * 
         * \return the sequence probability
         */
        double sequence_prob() const;

        /**
         * \brief Get the azimuth (heading or bearing) in degrees from north of the road point.
         * 
         * \return the azimuth
         */
        double azimuth() const;

        /** 
         * \brief Get the sample for the road point. 
         * 
         * \return pointer to the road point sample
         */
        geo_data::Sample::Ptr sample() const;

        /**
         * \brief Set the emission probability of this road point.
         * 
         * \param emission_prob the emission probability
         */
        void emission_prob(double emission_prob);

        /**
         * \brief Set the filter probability of this road point.
         * 
         * \param filter_prob the filter probability
         */
        void filter_prob(double filter_prob); 

        /**
         * \brief Set the sequence probability of this road point.
         * 
         * \param sequence_prob the sequence probability
         */
        void sequence_prob(double sequence_prob);
        
        /**
         * \brief Set the transition probability of this road point.
         * 
         * \param transition pointer the transition probability
         */
        void transition(Transition::Ptr transition);

        /**
         * \brief Set the road point candidate predecessor for this road point.
         * 
         * \param predecessor pointer to the predecessor road point
         */
        void predecessor(Ptr predecessor);

        /**
         * \brief Set the sample for road point candidate.
         * 
         * \param sample pointer to a Sample object
         */
        void sample(geo_data::Sample::Ptr sample);
    private:
        geo::Edge::Ptr edge_ptr_;
        geo_data::Sample::Ptr sample_ = nullptr;
        double fraction_;
        double azimuth_;
        double emission_prob_ = -1.0;
        double filter_prob_ = 0.0;
        double sequence_prob_ = -std::numeric_limits<double>::infinity();
        double transition_prob_ = -1.0;
        Transition::Ptr transition_ = nullptr;
        Ptr predecessor_ = nullptr;
};


using RoadPointMap = std::unordered_map<long, RoadPoint::Ptr>;
using RoadPointList = std::vector<RoadPoint::Ptr>;
using RoadPointSet = std::unordered_set<RoadPoint::Ptr>;

/**
 * \brief Emission state for a sample a some time (t) in the Hidden Markov Model.
 */
class EmissionState {
    public:
        using Ptr = std::shared_ptr<EmissionState>;    

        /** 
         * \brief Construct an EmissionState object.
         * 
         * \param sample pointer to underlying sample, can be a nullptr if this is the predecessor of the first sample
         */
        EmissionState(geo_data::Sample::Ptr sample = nullptr);

        /** \brief Road point candidates of for the emission state. */
        RoadPointSet candidates;

        /** \brief Sample for the emission state. */
        geo_data::Sample::Ptr sample;
};

/**
 * \brief Mark used for routing.
 */
class Mark {
    public:
        using Ptr = std::shared_ptr<Mark>;

        Mark(geo::Edge::Ptr mark_edge, geo::Edge::Ptr predecessor, double cost, double bounding_cost);

        geo::Edge::Ptr mark_edge;
        geo::Edge::Ptr predecessor;
        double cost;
        double bounding_cost;
};

/**
 * \brief Comparison Mark objects used in routing.
 */
class MarkComparison {
    public:
        bool operator() (const Mark::Ptr& lhs, const Mark::Ptr& rhs) const {
            return lhs->cost > rhs->cost;
        }
};

/**
 * \brief Static class for find the lowest cost (shortest time) path between two sequential samples in the Hidden Markov Model.
 */
class Router {
    public:
        using TargetRoadPointMap = std::unordered_map<RoadPoint::Ptr, geo::EdgeListPtr>;

        static constexpr float kHeuristicSpeed = 130.0;
        static constexpr double kHeuristicPriority = 1.0;

        /** \brief Djikstra's */
        static void route(RoadPoint::Ptr source, const RoadPointSet& targets, double max, TargetRoadPointMap& out); 
        static double time_cost(geo::Edge::Ptr edge);
        static double time_cost(geo::Edge::Ptr edge, double fraction) ;

        /** \brief Get the cost of a route in shortes time. */
        static double route_cost(RoadPoint::Ptr start, RoadPoint::Ptr end, geo::EdgeList& path);

    private:
        using RoadPointSetPtr = std::shared_ptr<RoadPointSet>;
        using EdgeTargetMap = std::unordered_map<geo::Edge::Ptr, RoadPointSetPtr>;
        using EdgeMarkMap = std::unordered_map<geo::Edge::Ptr, Mark::Ptr>;
        using RoadPointMarkMap = std::unordered_map<RoadPoint::Ptr, Mark::Ptr>;
        using MarkRoadPointMap = std::unordered_map<Mark::Ptr, RoadPoint::Ptr>;
};

/**
 * \brief Road map for finding road point candidates and routes between them.
 */
class RoadMap {
    public:
        using Ptr = std::shared_ptr<RoadMap>;
        using RouteTransitionMap = std::unordered_map<RoadPoint::Ptr, Transition::Ptr>;
        using RouteTransitionMapPtr = std::shared_ptr<RouteTransitionMap>;
        using TransitionMap = std::unordered_map<RoadPoint::Ptr, RouteTransitionMapPtr>;
    
        /**
         * \brief Construct a RoadMap object. 
         * 
         * \param sigma the emission probability distribution sigma constant
         * \param lambda the transition probability distribution lambda constant
         * \param radius for candidate road point queries in meters
         * \param distance max distance for routing in meters
         * \param shorten_turns flag to check for turns when computing the transition when routing
         */
        RoadMap(double sigma = 10.0, double lambda = 0.0, double radius = 200.0, double distance = 15000.0, bool shorten_turns = true);
        ~RoadMap();

        /** 
         * \brief Get the sigma for the emission probability distribution constant.
         * 
         * \param the sigma value
         */
        void sigma(double sigma);
    
        /**
         * \brief Get the number of edges in the quad tree. 
         * 
         * \return the number of edges (features)
         */
        int n_quad_features() const;

        /**
         * \brief Construct the road map from a geo_data::RoadReader. 
         * 
         * \param road_reader the road reader
         */
        void construct(geo_data::RoadReader& road_reader);

        /**
         * \brief Get the nearest candidate road points for a sample point.
         * 
         * \param point sample point
         * \param out the resulting candidate road points
         */
        void nearest(const OGRPoint& point, RoadPointSet& out);

        /** 
         * \brief Get the nearest candidate road points within the radius for a sample point.
         *  
         * \param point the sample point
         * \param radius the radius in meters
         * \param out the resulting candidate road points
         */
        void radius(const OGRPoint& point, double radius, RoadPointSet& out);

        /**
         * \brief Get the minimal set of road points from an existing road point candidate set. Removes points that are within a very short (GPS error) distance of each other.
         * 
         * \param road_points the original candidate road point set
         * \param out the resulting minimal set of road points
         */
        void minset(const RoadPointSet& road_points, RoadPointSet& out);

        /** 
         * \brief Find the candidate road point set for a sample. Checks predecessor's candidates in the Hidden Markov Models. If the predecessor road points map to the same edge and have better heading and distance, then the predecessor road point is used.
         *
         * \param sample pointer to the sample 
         * \param predecessor set of road point candidates from the predecessor sample 
         * \param out the resulting candidate road points for the sample
         */ 
        void candidates(geo_data::Sample::Ptr sample, const RoadPointSet& predecessors, RoadPointSet& out); 

        /**
         * \brief Find the transitions between two emission states. This is an all pairs comparison between road points in the predecessor states and road points in the current candidate state.
         * 
         * \param predecessor_state the predecessor emission state
         * \param candidate_state the candidate emission state
         * \param out the resulting transitions as a map of candidate road points to route transitions
         */
        void transitions(const EmissionState& predecessor_state, const EmissionState& candidate_state, TransitionMap& out);

        /** \brief The geo::Spatial object used in the road map functions. */
        geo::Spatial spatial;
    private:
        using QuadRoadMap = std::unordered_map<long, geo::Road::Ptr>;
        using EdgeDistanceMap = std::unordered_map<long, double>;

        void set_road_points_(const EdgeDistanceMap& neighbors, RoadPointSet& out);

        static constexpr double sigA_ = std::pow(10.0, 2);
        static constexpr double sqrt_2pi_sigA_ = std::sqrt(2.0 * geo::Spatial::kPi * sigA_);
        
        double sig2_;
        double sqrt_2pi_sig2_;
        double lambda_;
        double radius_;
        double distance_;
        bool shorten_turns_;
        CPLRectObj *bounds_ptr_ = nullptr;
        CPLQuadTree *quad_tree_ptr_ = nullptr;
        QuadRoadMap road_map_;
        geo::EdgeMap edge_map_;
        OGREnvelope bounds_;
};

using MatchedResults = std::list<RoadPoint::Ptr>;

/** 
 * \brief Hidden Markov Model Map Matcher for sequential GPS traces.
 */
class Matcher {
    public:
        /**
         * \brief Construct a Matcher object.
         * 
         * \param min_distance minimum distance, in meters, between samples in the model
         * \param min_time minimum time, in seconds, between samples in the model
         * \param k not used
         * \param t not used
         */
        Matcher(double min_distance, long min_time, int k = -1, long t = -1);

        /** 
         * \brief Map match samples in a sequential GPS trace. Sets the matched edge for each geo_data::Sample object until the end of trace is reached, or a break in the Hidden Markov Model occurs.
         * 
         * \param road_map the geo_data::RoadMap object to used for candidates and transitions.
         * \param trace the sample GPS trace
         */
        void map_match(RoadMap& road_map, geo_data::Sample::Trace& trace);
    private:        
        using StateVector = std::vector<EmissionState::Ptr>;
        using CounterMap = std::unordered_map<RoadPoint::Ptr, long>;

        RoadPoint::Ptr estimate();
        geo_data::Sample::Ptr state_sample();
        void remove(RoadPoint::Ptr candidate, int index);
        void update_state_vector(EmissionState::Ptr state);

        StateVector states_; 
        CounterMap counters_;
        double min_distance_;
        long min_time_;
        int k_;
        long t_;
        std::size_t sample_index_;
};

}

#endif
