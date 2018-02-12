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
#include "trajectory.hpp"
#include "utilities.hpp"

#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

namespace trajectory {

    Point::Point( const std::string& data, uint64_t time, double lat, double lon, double heading, double speed, uint64_t index ) :
        geo::Location{ lat, lon, index }, 
        data{ data },
        heading{ heading },
        speed{ speed },
        time{ time },
        index{ index },
        fitedge{nullptr},
        critical_interval{ nullptr },
        _private{ false },
        is_hmm_map_match_{ false },
        outdegree{ 0 }
    {}

    Point::Point() :
        Point::Point{ "", 0, 0.0, 0.0, 0.0, 0.0, 0 }
    {
    }

    const std::string& Point::get_data() const
    {
        return data;
    }

    double Point::get_speed() const
    {
        return speed;
    }

    double Point::get_heading() const
    {
        return heading;
    }

    uint64_t Point::get_index() const
    {
        return index;
    }

    uint64_t Point::get_time() const
    {
        return time;
    }

    uint32_t Point::get_out_degree() const
    {
        return outdegree;
    }

    Point& Point::set_out_degree( uint32_t degree ) 
    {
        outdegree = degree;
        return *this;
    }

    double Point::heading_delta( const Point& tp ) const
    {
        return heading_delta( tp.get_heading() );
    }

    double Point::heading_delta( double heading_other ) const
    {
        double delta = std::abs( heading - heading_other );
        return (delta < 180.0) ? delta : 360.0 - delta;
    }

    std::ostream& operator<< ( std::ostream& os, const Point& tp )
    {
        os << tp.index << ",";
        os << tp.uid << ",";
        os << tp.time << ",";
        os << tp.lat << ",";
        os << tp.lon << ",";
        os << tp.latr << ",";
        os << tp.lonr << ",";
        os << tp.heading << ",";
        os << tp.speed << ":";
        if (tp.has_edge()) {
            os << tp.fitedge->get_uid() << ",";
            os << tp.fitedge->get_way_type_index() << ",";
            os << *(tp.fitedge->v1) << ",";
            os << *(tp.fitedge->v2) << ",";
        }

        os << ":";

        return os;
    }

    void Point::set_fit_edge( const geo::EdgeCPtr& eptr )
    {
        //std::cerr << "TP: " << *this << " is fit to: " << *eptr << std::endl;
        fitedge = eptr;
    }

    bool Point::has_edge() const
    {
        return fitedge != nullptr;
    }

    geo::EdgeCPtr Point::get_fit_edge()
    {
        return fitedge;
    }

    IntervalCPtr Point::get_critical_interval() const 
    {
        return critical_interval;
    }

    void Point::set_critical_interval( IntervalCPtr iptr ) 
    {
        critical_interval = iptr;
    }

    bool Point::is_implicitly_fit() const
    {
        return fitedge != nullptr && fitedge->is_implicit();
    }

    bool Point::is_explicitly_fit() const
    {
        return fitedge != nullptr && !fitedge->is_implicit();
    }

    void Point::set_private() 
    {
        _private = true;
    }

    void Point::set_index(uint64_t i) {
        index = i;
    }

    bool Point::is_private() const
    {
        return _private;
    }

    bool Point::is_critical() const
    {
        return critical_interval != nullptr;
    }
     
    double Point::diff_180( double diff )
    {
        if (diff > 180.0) {
            return std::abs(diff - 360.0);

        } else if (diff < -180.0) {
            return diff + 360.0;
        }

        return std::abs(diff);
    }

    double Point::angle_error( double a, double b )
    {
        double b_prime = std::fmod(b+180.0,360.0);

        double diff1 = a-b;
        double diff2 = a-b_prime;

        diff1 = diff_180( diff1 );
        diff2 = diff_180( diff2 );

        return ( diff1 < diff2 ? diff1 : diff2 );
    }

    bool Point::consistent_with( const geo::EdgePtr& eptr ) const
    {
        return consistent_with( *eptr );
    }

    bool Point::consistent_with( const geo::Edge& edge ) const
    {
        return angle_error( heading, edge.bearing() ) < 15.0;
    }

    Interval::Interval( Index left, Index right, std::string aux, Index id ) :
        _left{left},
        _right{right},
        _aux_set{ { aux } },
        _id { id }
    {}

    Interval::Interval( Index left, Index right, Interval::AuxSetPtr aux_set, Index id ) :
        _left{left},
        _right{right},
        _aux_set{ *aux_set },
        _id { id }
    {}

    Index Interval::id() const
    {
        return _id;
    }

    Index Interval::left() const
    {
        return _left;
    }

    Index Interval::right() const
    {
        return _right;
    }

    const Interval::AuxSetPtr Interval::get_aux_set() const
    {
        return std::make_shared<AuxSet>(_aux_set);
    }

    const std::string Interval::get_aux_str() const {
        std::string aux_types = "";
        bool first = true;

        for (auto& aux : _aux_set) {
            if (first) {
                first = false;    
            } else {
                aux_types += ";";
            }

            aux_types += aux;
        }
        
        return aux_types;
    }

    bool Interval::contains( Index value ) const 
    {
        return value >= _left && value < _right;
    }

    bool Interval::is_before( Index value ) const
    {
        return _right <= value;
    }

    std::ostream& operator<< ( std::ostream& os, const Interval& interval )
    {
        std::string aux_types = "";
        bool first = true;

        for (auto& aux : interval._aux_set) 
        {
            if (first) 
            {
                first = false;    
            } else
            {
                aux_types += ", ";
            }

            aux_types += aux;
        }

        return os << "id = " << interval._id << " [" << interval._left << ", " << interval._right << " ) types: { " << aux_types << " }";
    }
}
