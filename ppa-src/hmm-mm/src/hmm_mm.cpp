#include "hmm_mm.hpp"

#include <algorithm>
#include <queue>
#include <sstream>
#include <utility>

#include <rapidjson/istreamwrapper.h>
#include <gdal/cpl_conv.h>
#include <GeographicLib/GeodesicLine.hpp>

// TEMP
#include <iostream>

namespace hmm_mm {

RoadPoint::RoadPoint(geo::Edge::Ptr edge_ptr, double fraction) :
    edge_ptr_(edge_ptr),
    fraction_(fraction)
{
    geo::Spatial spatial;
    spatial.interpolate(edge_ptr->line_string, fraction, geometry);
    azimuth_ = spatial.azimuth(edge_ptr->line_string, fraction);
}

geo::Edge::Ptr RoadPoint::edge() const {
    return edge_ptr_;
}

RoadPoint::Ptr RoadPoint::predecessor() const {
    return predecessor_;
}

Transition::Ptr RoadPoint::transition() const {
    return transition_;
}

double RoadPoint::fraction() const {
    return fraction_;
}

double RoadPoint::emission_prob() const {
    return emission_prob_;
}

double RoadPoint::filter_prob() const {
    return filter_prob_;
}

double RoadPoint::sequence_prob() const {
    return sequence_prob_;
}

double RoadPoint::azimuth() const {
    return azimuth_;
}

void RoadPoint::emission_prob(double emission_prob) {
    emission_prob_ = emission_prob;
}

void RoadPoint::filter_prob(double filter_prob) {
    filter_prob_ = filter_prob;
}

void RoadPoint::sequence_prob(double sequence_prob) {
    sequence_prob_ = sequence_prob;
}

void RoadPoint::transition(Transition::Ptr transition) {
    transition_ = transition;
}

void RoadPoint::predecessor(RoadPoint::Ptr predecessor) {
    predecessor_ = predecessor;
}

void RoadPoint::sample(geo_data::Sample::Ptr sample) {
    sample_ = sample;
}

geo_data::Sample::Ptr RoadPoint::sample() const {
    return sample_;
}
        
EmissionState::EmissionState(geo_data::Sample::Ptr sample) :
    sample(sample)
{}


Mark::Mark(geo::Edge::Ptr mark_edge, geo::Edge::Ptr predecessor, double cost, double bounding_cost) :
    mark_edge(mark_edge),
    predecessor(predecessor),
    cost(cost),
    bounding_cost(bounding_cost)
{}

void Router::route(RoadPoint::Ptr source, const RoadPointSet& targets, double max, TargetRoadPointMap& out) {
    double start_cost;
    double start_bound;
    double reach_cost;
    double reach_bound;
    double target_fraction;
    double successor_cost;
    double successor_bound;
    geo::Edge::Ptr successor;
    geo::Edge::Ptr next;
    geo::Edge::Ptr neighbor;
    RoadPoint::Ptr target;
    Mark::Ptr reach;
    Mark::Ptr mark;
    Mark::Ptr start;
    Mark::Ptr current;
    RoadPointSetPtr road_points;
    MarkComparison comp;
    EdgeTargetMap target_edges;
    std::vector<Mark::Ptr> priorities;
    EdgeMarkMap entries; 
    RoadPointMarkMap finishes;
    MarkRoadPointMap reaches;
    MarkRoadPointMap starts;
    double source_fraction = source->fraction();
    geo::Edge::Ptr source_edge = source->edge();

    for (auto& road_point : targets) {
        EdgeTargetMap::const_iterator it = target_edges.find(road_point->edge());

        if (it == target_edges.end()) {
            target_edges[road_point->edge()] = std::make_shared<RoadPointSet>();
        }

        target_edges[road_point->edge()]->insert(road_point);
    }

    std::make_heap(priorities.begin(), priorities.end(), comp);
    start_cost = time_cost(source_edge, 1.0 - source_fraction);
    start_bound = source_edge->length() * (1.0 - source_fraction);

    EdgeTargetMap::const_iterator target_it = target_edges.find(source_edge);
    
    if (target_it != target_edges.end()) {
        for (auto& target : *(target_edges[source_edge])) {
            target_fraction = target->fraction();

            if (target_fraction < source_fraction) {
                continue;
            }

            reach_cost = start_cost - time_cost(source_edge, 1.0 - target_fraction);
            reach_bound = start_cost - source_edge->length() * (1.0 - target_fraction);
            
            reach = std::make_shared<Mark>(source_edge, nullptr, reach_cost, reach_bound);
            reaches[reach] = target;
            starts[reach] = source;
            priorities.push_back(reach);
            std::push_heap(priorities.begin(), priorities.end(), comp);
        }
    }

    EdgeMarkMap::const_iterator entry_it = entries.find(source_edge);

    if (entry_it == entries.end()) {
        start = std::make_shared<Mark>(source_edge, nullptr, start_cost, start_bound);
        entries[source_edge] = start;
        starts[start] = source;
        priorities.push_back(start);
        std::push_heap(priorities.begin(), priorities.end(), comp);
    } else if (start_cost < entries[source_edge]->cost) {
        start = std::make_shared<Mark>(source_edge, nullptr, start_cost, start_bound); 
        entries[source_edge] = start;
        starts[start] = source;
        // TODO This is different from the reference implementation,
        // but I think it is correct. The RI removes the newly created start 
        // instead of the existing start, which doesn't make sense if they 
        // are just adding it back to the PQ.
        std::vector<Mark::Ptr>::const_iterator mark_it = std::find(priorities.begin(), priorities.end(), entry_it->second);
    
        priorities.erase(mark_it);
        priorities.push_back(start);
        std::push_heap(priorities.begin(), priorities.end(), comp);
    }

    // Djikstra's

    while (!priorities.empty()) {
        if (target_edges.empty()) {
            break;
        }

        current = priorities.front();
        std::pop_heap(priorities.begin(), priorities.end(), comp);
        priorities.pop_back();

        if (current->bounding_cost > max) {
            break;
        }

        MarkRoadPointMap::const_iterator reaches_it = reaches.find(current);
        
        if (reaches_it != reaches.end()) {
            target = reaches[current];
           
            RoadPointMarkMap::const_iterator finishes_it = finishes.find(target);
    
            if (finishes_it != finishes.end()) {
                continue;
            } 
           
            finishes[target] = current; 

            road_points = target_edges[current->mark_edge];
            road_points->erase(target);
            
            if (road_points->empty()) {
                target_edges.erase(current->mark_edge);
            }

            continue;
        }
        
        successor = current->mark_edge->successor();
        next = successor;   
        
        while (next != nullptr) {
            successor_cost = current->cost + time_cost(next); 
            successor_bound = current->bounding_cost + next->length();

            EdgeTargetMap::const_iterator target_it = target_edges.find(next);
    
            if (target_it != target_edges.end()) {
                for (auto& target : *(target_edges[next])) {
                    reach_cost = successor_cost - time_cost(next, 1.0 - target->fraction());
                    reach_bound = successor_bound - next->length() * (1.0 - target->fraction());
                    reach = std::make_shared<Mark>(next, current->mark_edge, reach_cost, reach_bound);
                    reaches[reach] = target;
                    priorities.push_back(reach);
                    std::push_heap(priorities.begin(), priorities.end(), comp);
                }
            }

            EdgeMarkMap::const_iterator entry_it = entries.find(next);

            if (entry_it == entries.end()) {
                mark = std::make_shared<Mark>(next, current->mark_edge, successor_cost, successor_bound);
                entries[next] = mark; 
                priorities.push_back(mark);
                std::push_heap(priorities.begin(), priorities.end(), comp);
            }

            neighbor = next->neighbor();
            next = successor == neighbor ? nullptr : neighbor;
        }
    }
    
    for (auto& target : targets) {
        RoadPointMarkMap::const_iterator finishes_it = finishes.find(target);

        if (finishes_it == finishes.end()) {
            // add target to path
            out[target] = nullptr;
        } else {
            mark = finishes[target];
            geo::EdgeListPtr path = std::make_shared<std::vector<geo::Edge::Ptr>>();
           
            while (mark != nullptr) { 
                path->push_back(mark->mark_edge);
                start = mark;
                mark = mark->predecessor != nullptr ? entries[mark->predecessor] : nullptr;
            }

            out[target] = path;
        }        
    }
}

double Router::time_cost(geo::Edge::Ptr edge) {
    // TODO constants are not linking for some reason
    //return edge->length() * 3.6 / std::min(static_cast<float>(edge->maxspeed()), kHeuristicSpeed) * std::max(kHeuristicPriority, static_cast<double>(edge->priority())); 
    return edge->length() * 3.6 / std::min(static_cast<float>(edge->maxspeed()), 130.0f) * std::max(1.0, static_cast<double>(edge->priority())); 
}

double Router::time_cost(geo::Edge::Ptr edge, double fraction) {
    return time_cost(edge) * fraction;
}

double Router::route_cost(RoadPoint::Ptr start, RoadPoint::Ptr end, geo::EdgeList& path) {
    double value = time_cost(start->edge(), 1.0 - start->fraction());

    for (int i = path.size() - 2; i >= 0; --i) {
        value += time_cost(path[i]);
    }

    value -= time_cost(end->edge(), 1.0 - end->fraction());

    return value;
}

Transition::Transition(geo::EdgeListPtr path, double transition_prob) :
    path(path),
    transition_prob(transition_prob)
{}

RoadMap::RoadMap(double sigma, double lambda, double radius, double distance, bool shorten_turns) :
    sig2_(std::pow(sigma, 2)),
    sqrt_2pi_sig2_(std::sqrt(2.0 * geo::Spatial::kPi * sig2_)),
    lambda_(lambda),
    radius_(radius),
    distance_(distance),
    shorten_turns_(shorten_turns)
{
    bounds_ptr_ = new CPLRectObj;
    bounds_ptr_->minx = -180.0;
    bounds_ptr_->maxx = 180.0;
    bounds_ptr_->miny = -90.0;
    bounds_ptr_->maxy = 90.0;
    quad_tree_ptr_ = CPLQuadTreeCreate(bounds_ptr_, nullptr);
}

RoadMap::~RoadMap() {
    if (quad_tree_ptr_ != nullptr) {
        CPLQuadTreeDestroy(quad_tree_ptr_);
    }   

    if (bounds_ptr_ != nullptr) {
        delete bounds_ptr_;
    }
}

void RoadMap::sigma(double sigma) {
    sig2_ = std::pow(sigma, 2);
    sqrt_2pi_sig2_ = std::sqrt(2.0 * geo::Spatial::kPi * sigma);
}

int RoadMap::n_quad_features() const {
    int a, b, c, d;
    
    CPLQuadTreeGetStats(quad_tree_ptr_, &a, &b, &c, &d);

    return a;
}

void RoadMap::construct(geo_data::RoadReader& road_reader) {
    geo::Road::Ptr road_ptr = nullptr;
    geo::Edge::Ptr prev_edge_ptr, last_edge_ptr;
    geo::EdgeListPtr edge_list_ptr, successor_list_ptr;
    geo::EdgeListMap edge_list_map;
    long source, edge_id;
    int prev_edge;

    // put all roads in the quad tree and map all edges to their ids
    while (true) {
        road_ptr = road_reader.next_road();

        if (!road_ptr) {
            break;
        }

        if (!road_ptr->is_valid() || road_ptr->is_excluded()) {
            continue;
        }

        // map road ids to roads
        QuadRoadMap::const_iterator it = road_map_.find(road_ptr->id());

        if (it == road_map_.end()) {
            // first time seeing this road 
            // add it to map and quad tree
            // NOTE: we must map the road_ptr here to make sure it's
            // persistent. Otherwise the underlying pointer will get
            // clobbered.
            road_map_[road_ptr->id()] = road_ptr;
            CPLQuadTreeInsertWithBounds(quad_tree_ptr_, road_ptr.get(), &(road_ptr->bounds));
        }

        // map each edge source to a list of edges with the same source
        // each edge has a source and target
        // the source is the node id representing the incoming node of the edge
        // the target is the node id representing the outgoing node of the edge
        // each source node can have multiple source's, so we map the source to a 
        // list of edges with that source
        for (auto& e_ptr : geo::split_road(road_ptr)) {
            edge_map_[e_ptr->id()] = e_ptr;
            source = e_ptr->source();
            geo::EdgeListMap::const_iterator it = edge_list_map.find(source);

            if (it == edge_list_map.end()) {
                edge_list_ptr = std::make_shared<geo::EdgeList>();
                edge_list_map[source] = edge_list_ptr;
            } else {
                edge_list_ptr = it->second;
            }

            edge_list_ptr->push_back(e_ptr);
        }
    }

    // build the graph
    // each edge has a neighbor and successor edge
    // the current edge's neighbor edge is the next edge in the list or 
    // the first edge if the current edge is last edge in the list
    // the current edge's successor is the first edge in list of edges
    // mapped to the current edge's target, or nothing if the current edge's
    // target is not in the map
    for (auto& e : edge_list_map) {
        edge_list_ptr = e.second;

        for (int i = 1; i < edge_list_ptr->size(); ++i) {
            prev_edge = i - 1;
            prev_edge_ptr = edge_list_ptr->at(prev_edge);
            
            // set this edge's neighbor to the current edge in the list
            prev_edge_ptr->neighbor(edge_list_ptr->at(i));

            // find the edge list for the previous edge's target
            geo::EdgeListMap::const_iterator it = edge_list_map.find(prev_edge_ptr->target());

            if (it == edge_list_map.end()) {
                // no edge list for previous edge target
                // no successor
                prev_edge_ptr->successor(nullptr); 
            } else {
                // successor is the first edge of the previous edge list target
                successor_list_ptr = it->second;
                prev_edge_ptr->successor(successor_list_ptr->at(0)); 
            }
        }

        // set the last edge's neighbor to the first edge
        last_edge_ptr = edge_list_ptr->at(edge_list_ptr->size() - 1);        
        last_edge_ptr->neighbor(edge_list_ptr->at(0));
        geo::EdgeListMap::const_iterator it = edge_list_map.find(last_edge_ptr->target());

        // set the last edge's successor to the first edge of the edge's 
        // target edge list, or nothing is the target is not in the list
        if (it == edge_list_map.end()) {
            last_edge_ptr->successor(nullptr);  
        } else {
            successor_list_ptr = it->second;
            last_edge_ptr->successor(successor_list_ptr->at(0));
        }
    }
}

void RoadMap::radius(const OGRPoint& point, double radius, RoadPointSet& out) {
    EdgeDistanceMap neighbors;
    CPLRectObj bounds;
    OGRPoint tmp;
    geo::Edge::Ptr edge_ptr;
    void **results;
    long id;
    int n_results;
    double d;
    double f;
    
    spatial.rect_for_radius(point, radius, bounds); 
    results = CPLQuadTreeSearch(quad_tree_ptr_, &bounds, &n_results);
        
    for (int i = 0; i < n_results; ++i) {
        geo::Road* r_ptr = static_cast<geo::Road*>(results[i]);
        id = r_ptr->id();

        // find the intercept value for the point on the road
        f = spatial.intercept(r_ptr->line_string, point);

        // get the point on the road
        if (!spatial.interpolate(r_ptr->line_string, r_ptr->length(), f, tmp)) {
            continue;
        }

        // get the distance from projection to point 
        d = spatial.distance(tmp, point);  

        // include everything withing the radius
        if (d < radius) {
            neighbors[id] = f;
        }
    }

    CPLFree(results);
    set_road_points_(neighbors, out);
}

void RoadMap::nearest(const OGRPoint& point, RoadPointSet& out) {
    EdgeDistanceMap neighbors;
    CPLRectObj bounds;
    OGRPoint tmp;
    geo::Edge::Ptr edge_ptr;
    void **results;
    int n_results;
    double radius = 100.0; 
    double min = std::numeric_limits<double>::max();
    double f;
    double d;
    long id;

    do {
        spatial.rect_for_radius(point, radius, bounds); 
        results = CPLQuadTreeSearch(quad_tree_ptr_, &bounds, &n_results);

        for (int i = 0; i < n_results; ++i) {
            geo::Road* r_ptr = static_cast<geo::Road*>(results[i]);
            id = r_ptr->id();

            // find the intercept value for the point on the road
            f = spatial.intercept(r_ptr->line_string, point);
           
            // get the point on the road
            if (!spatial.interpolate(r_ptr->line_string, r_ptr->length(), f, tmp)) {
                continue;
            }
          
            // get the distance from projection to point 
            d = spatial.distance(tmp, point);  

            // not greater than min; won't pass on first iteration
            if (d > min) {
                continue;
            }

            // clear candidates on new min; always passes on first iteration
            if (d < min) {
                min = d;
                neighbors.clear();
            } 

            // map road id to the intersection value of the closest point
            neighbors[id] = f;
        }

        radius = radius * 2.0;
        CPLFree(results);
    } while (neighbors.empty());

    set_road_points_(neighbors, out);
}

void RoadMap::minset(const RoadPointSet& road_points, RoadPointSet& out) {
    RoadPointMap map;
    std::unordered_map<long, int> misses;
    std::unordered_set<long> removes;
    geo::Edge::Ptr successor;
    geo::Edge::Ptr next;
    geo::Edge::Ptr neighbor;
    long id;
    
    for (auto& road_point : road_points) {
        map[road_point->edge()->id()] = road_point;
        misses[road_point->edge()->id()] = 0;  
    } 

    for (auto& road_point : road_points) {
        successor = road_point->edge()->successor();
        next = successor;
        id = road_point->edge()->id();

        while (next != nullptr) {
            RoadPointMap::const_iterator it = map.find(next->id());

            if (it == map.end()) {
                misses[id] = static_cast<int>(misses[id] + 1); 
            } 

            if (it != map.end() && spatial.round(map[next->id()]->fraction()) == 0) {
                removes.emplace(next->id());
                misses[id] = static_cast<int>(misses[id] + 1); 
            }
            
            neighbor = next->neighbor();
            next = successor == neighbor ? nullptr : neighbor;
        }
    }

    for (auto& road_point : road_points) {
        id = road_point->edge()->id();
        RoadPointMap::const_iterator map_it = map.find(id);
        std::unordered_set<long>::const_iterator removes_it = removes.find(id);

        if (map_it != map.end() && removes_it == removes.end() && spatial.round(road_point->fraction()) == 1 && misses[id] == 0) {
            removes.emplace(id);
        } 
    }

    for (auto id : removes) {
        map.erase(id);
    }

    for (auto& e : map) {
        out.insert(e.second);
    }
}

void RoadMap::candidates(geo_data::Sample::Ptr sample, const RoadPointSet& predecessors, RoadPointSet& out) {
    double dz;
    double da;
    double emission_prob;
    bool is_better_distance;
    bool is_better_forward_heading;
    bool is_better_backward_heading;
    bool is_better_azimuth;
    RoadPointSet road_points;
    RoadPointSet min_points;
    RoadPointMap map;
    RoadPoint::Ptr road_point;

    radius(sample->point, radius_, road_points); 
    minset(road_points, min_points);

    if (!predecessors.empty()) {
        for (auto& road_point : min_points) {
            map[road_point->edge()->id()] = road_point; 
        }
    
        // check the road point for each predecessor
        for (auto& predecessor : predecessors) {
            RoadPointMap::const_iterator it = map.find(predecessor->edge()->id());
    
            if (it == map.end() || map[predecessor->edge()->id()]->edge() == nullptr) {
                continue;
            }

            road_point = map[predecessor->edge()->id()];
            is_better_distance = spatial.distance(road_point->geometry, predecessor->geometry) < std::sqrt(sig2_);
            is_better_forward_heading = (road_point->edge()->heading() == geo::Heading::FORWARD && road_point->fraction() < predecessor->fraction());
            is_better_backward_heading = (road_point->edge()->heading() == geo::Heading::BACKWARD && road_point->fraction() > predecessor->fraction());

            if (is_better_distance && (is_better_forward_heading || is_better_backward_heading)) {
                min_points.erase(road_point);
                // we need a new road point here to avoid issues when
                // updating the filter probability
                min_points.insert(std::make_shared<RoadPoint>(predecessor->edge(), predecessor->fraction()));
            }
        }
    } 

    for (auto& road_point : min_points) {
        dz = spatial.distance(sample->point, road_point->geometry); 
        emission_prob = 1.0 / sqrt_2pi_sig2_ * std::exp((-1.0) * dz * dz / (2.0 * sig2_));
       
        if (!std::isnan(sample->azimuth())) {
            is_better_azimuth = sample->azimuth() > road_point->azimuth();            
            da = is_better_azimuth ? std::min(sample->azimuth() - road_point->azimuth(), 360.0 - (sample->azimuth() - road_point->azimuth())) : std::min(road_point->azimuth() - sample->azimuth(), 360.0 - (road_point->azimuth() - sample->azimuth())); 

            emission_prob = emission_prob * std::max(1E-2, 1.0 / sqrt_2pi_sigA_ * std::exp((-1.0) * da * da / (2.0 * sigA_)));
        }

        road_point->emission_prob(emission_prob);
        out.insert(road_point);
    }     
}

void RoadMap::transitions(const EmissionState& predecessor_state, const EmissionState& candidate_state, TransitionMap& out) {
    RoadPointSet targets;
    Router::TargetRoadPointMap route;
    geo::EdgeListPtr path;
    RoadPoint::Ptr start;
    RoadPoint::Ptr end;
    RouteTransitionMapPtr candidate_route_map;
    double bound;
    double beta;
    double transition_prob;
    double fraction_diff;
    long time_d;
    int penul_edge_index;

    for (auto& road_pt : candidate_state.candidates) {
        targets.insert(road_pt); 
    } 

    time_d = ((candidate_state.sample->timestamp() - predecessor_state.sample->timestamp()) / 1000) * 100;
    bound = std::max(1000.0, std::min(distance_, static_cast<double>(time_d))); 

    for (auto& pred_road_pt : predecessor_state.candidates) {
        route.clear();
        Router::route(pred_road_pt, targets, bound, route);
        
        candidate_route_map = std::make_shared<RouteTransitionMap>();

        for (auto& cand_road_pt : candidate_state.candidates) {
            path = route[cand_road_pt];

            if (path == nullptr) {
                continue;
            }

            start = pred_road_pt;
            end = cand_road_pt;

            if (shorten_turns_ && path->size() >= 2) {
                penul_edge_index = path->size() - 2;

                if (path->back()->road()->id() == path->at(penul_edge_index)->road()->id() && path->back()->id() != path->at(penul_edge_index)->id()) {
                    if (path->size() > 2) {
                        start = std::make_shared<RoadPoint>(path->at(penul_edge_index), 1.0 - start->fraction());
                        path->erase(path->end() - 1);
                    } else {
                        if (start->fraction() < 1.0 - end->fraction()) {
                            end = std::make_shared<RoadPoint>(path->back(), std::min(1.0, 1.0 - end->fraction() + (5.0 / path->at(0)->length())));
                            path->erase(path->end() - 2);
                        } else {
                            start = std::make_shared<RoadPoint>(path->at(penul_edge_index), std::max(0.0, 1.0 - start->fraction() - (5.0 / path->at(penul_edge_index)->length())));
                            path->erase(path->end() - 1);
                        }
                    }
                }  
            }


            beta = lambda_ == 0.0 ? std::max(1.0, static_cast<double>(candidate_state.sample->timestamp() - predecessor_state.sample->timestamp())) / 1000.0 : 1.0 / lambda_;
            transition_prob = (1.0 / beta) * std::exp((-1.0) * Router::route_cost(start, end, *path) / beta);

            (*candidate_route_map)[cand_road_pt] = std::make_shared<Transition>(path, transition_prob);
        } 

        out[pred_road_pt] = candidate_route_map;
    }
}

void RoadMap::set_road_points_(const EdgeDistanceMap& neighbors, RoadPointSet& out) {
    geo::Edge::Ptr edge_ptr;

    for (auto& neighbor : neighbors) {
        edge_ptr = edge_map_[neighbor.first * 2];
        out.insert(std::make_shared<RoadPoint>(edge_ptr, neighbor.second));        
        geo::EdgeMap::const_iterator it = edge_map_.find(neighbor.first * 2 + 1);

        if (it != edge_map_.end()) {
            edge_ptr = edge_map_[neighbor.first * 2 + 1];
            out.insert(std::make_shared<RoadPoint>(edge_ptr, 1.0 - neighbor.second));        
        }
    }
}

Matcher::Matcher(double min_distance, long min_time, int k, long t) :
    min_distance_(min_distance),
    min_time_(min_time),
    k_(k),
    t_(t)
{}

RoadPoint::Ptr Matcher::estimate() {
    RoadPoint::Ptr kEstimate = nullptr;

    if (states_.empty()) {
        return kEstimate;
    }
    
    for (auto& candidate : states_.back()->candidates) {
        if (kEstimate == nullptr || candidate->filter_prob() > kEstimate->filter_prob()) {
           kEstimate = candidate;
        } 
    }

    return kEstimate;
}

geo_data::Sample::Ptr Matcher::state_sample() {
    if (states_.empty()) {
        return nullptr;
    }

    return states_.back()->sample;
}

void Matcher::remove(RoadPoint::Ptr candidate, int index) {
    int i = index;
    RoadPoint::Ptr c = candidate;
    RoadPoint::Ptr predecessor;

    while (i >= 0) {
        counters_.erase(c);    
        states_[i]->candidates.erase(c);
       
        predecessor = c->predecessor(); 
        
        if (predecessor == nullptr) {
            return;
        }

        counters_[predecessor] = counters_[predecessor] - 1;
        
        if (counters_[predecessor] == 0) {
            c = predecessor;
            i--;
        } else {
            return;
        }
    }
}

void Matcher::update_state_vector(EmissionState::Ptr state) {
    EmissionState::Ptr last;
    EmissionState::Ptr first;
    RoadPointSet deletes;
    RoadPoint::Ptr kEstimate;

    if (state->candidates.empty()) {
        return;
    }
   
    kEstimate = estimate();

    for (auto& candidate : state->candidates) {
        counters_[candidate] = 0;
       
        if (candidate->predecessor() == nullptr) { 
            candidate->predecessor(kEstimate);  
        }
            
        if (candidate->predecessor() != nullptr) {
            counters_[candidate->predecessor()] = counters_[candidate->predecessor()] + 1; 
        }
    } 
    
    if (!states_.empty()) {
        last = states_.back();

        for (auto& candidate : last->candidates) {
            if (counters_[candidate] == 0) {
                deletes.insert(candidate);
            }
        }

        for (auto& candidate : deletes) {
            remove(candidate, states_.size() - 1);            
        }
    }

    states_.push_back(state);
    
    // not executed by default
    // TODO not tested
    while ((t_ > 0 && state->sample->timestamp() - states_.front()->sample->timestamp() > t_) || (k_ > 0 && states_.size() > k_ + 1)) {
        first = states_.front();
        states_.erase(states_.begin());

        for (auto& candidate: first->candidates) {
            counters_.erase(candidate);
        }

        for (auto& candidate : states_.front()->candidates) {
            candidate->predecessor(nullptr);
        }
    } 
}

void Matcher::map_match(RoadMap& road_map, geo_data::Sample::Trace& trace) {
    geo_data::Sample::Ptr sample;
    geo_data::Sample::Ptr previous;
    EmissionState::Ptr predecessor_state; 
    EmissionState::Ptr candidate_state; 
    RoadPointSet candidates;
    RoadMap::TransitionMap transition_map;
    RoadPoint::Ptr kEstimate;
    Transition::Ptr transition;
    double sequence_prob;
    double norm_sum;

    geo_data::Sample::Trace::const_iterator sample_it = trace.begin(); 

    while (sample_it != trace.end()) {
        // new model
        states_.clear();
        counters_.clear();

        while (sample_it != trace.end()) {
            sample = *sample_it;

            if (!sample->is_valid()) {
                ++sample_it;

                continue;
            }

            previous = state_sample();

            if (previous != nullptr && (road_map.spatial.distance(sample->point, previous->point) < std::max(0.0, min_distance_) || (sample->timestamp() - previous->timestamp() < std::max(0l, min_time_)))) {
                ++sample_it;

                continue;
            } 

            if (states_.empty()) {
                predecessor_state = std::make_shared<EmissionState>();
            } else {
                predecessor_state = states_.back();
            }

            candidate_state = std::make_shared<EmissionState>(sample);
            EmissionState tmp_state(sample);
            road_map.candidates(sample, predecessor_state->candidates, tmp_state.candidates);

            norm_sum = 0.0;

            if (!predecessor_state->candidates.empty()) {
                transition_map.clear();
                road_map.transitions(*predecessor_state, tmp_state, transition_map);

                for (auto& cand_road_point : tmp_state.candidates) {
                    cand_road_point->sequence_prob(-std::numeric_limits<double>::infinity());

                    for (auto& pred_road_pt : predecessor_state->candidates) {
                        RoadMap::RouteTransitionMap::const_iterator it = transition_map[pred_road_pt]->find(cand_road_point);

                        if (it == transition_map[pred_road_pt]->end()) {
                            continue;
                        }
        
                        transition = transition_map[pred_road_pt]->at(cand_road_point);
        
                        if (transition->transition_prob == 0.0) {
                            continue;
                        }

                        cand_road_point->filter_prob(cand_road_point->filter_prob() + (transition->transition_prob * pred_road_pt->filter_prob()));
                        sequence_prob = pred_road_pt->sequence_prob() + std::log10(transition->transition_prob) + std::log10(cand_road_point->emission_prob());

                        if (sequence_prob > cand_road_point->sequence_prob()) {
                            cand_road_point->predecessor(pred_road_pt);
                            cand_road_point->transition(transition);
                            cand_road_point->sequence_prob(sequence_prob);
                        }
                    } 

                    if (cand_road_point->filter_prob() == 0.0) {
                        continue;
                    }    

                    cand_road_point->filter_prob(cand_road_point->filter_prob() * cand_road_point->emission_prob());
                    norm_sum += cand_road_point->filter_prob();

                    candidate_state->candidates.insert(cand_road_point);
                }
            }

            if (!tmp_state.candidates.empty() && candidate_state->candidates.empty() && !predecessor_state->candidates.empty()) {
                // candidates and predecessors, but no transistions
                // HMM transition, break the model but use current sample as 
                // the start

                break;
            }

            if (candidate_state->candidates.empty() || predecessor_state->candidates.empty()) {
                for (auto& cand_road_pt : tmp_state.candidates) {
                    if (cand_road_pt->emission_prob() == 0.0) {
                        continue;
                    }
                        
                    norm_sum += cand_road_pt->emission_prob();
                    cand_road_pt->filter_prob(cand_road_pt->emission_prob()); 
                    cand_road_pt->sequence_prob(std::log10(cand_road_pt->emission_prob()));
                    candidate_state->candidates.insert(cand_road_pt);
                }
            }

            if (candidate_state->candidates.empty()) {
                // no candidates
                // HMM no roads found
                ++sample_it;

                break; 
            }

            for (auto& result_road_pt : candidate_state->candidates) {
                result_road_pt->filter_prob(result_road_pt->filter_prob() / norm_sum);
                result_road_pt->sample(sample);
            }

            update_state_vector(candidate_state);
            ++sample_it;
        }

        if (states_.empty()) {
            continue;
        }

        kEstimate = estimate();

        for (long i = states_.size() - 1; i >= 0; --i) {
            if (kEstimate != nullptr) {
                kEstimate->sample()->matched_edge(kEstimate->edge()); 
                kEstimate = kEstimate->predecessor();
            } 
        } 
    }
}

}
