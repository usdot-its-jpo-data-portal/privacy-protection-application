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
#include "instrument.hpp"

namespace instrument {
    PointCounter::PointCounter() :
        n_points(0),
        n_invalid_field_points(0),
        n_invalid_geo_points(0),
        n_invalid_heading_points(0),
        n_error_points(0),
        n_ci_points(0),
        n_pi_points(0)
    {}

    PointCounter::PointCounter(uint64_t n_points, uint64_t n_invalid_field_points, uint64_t n_invalid_geo_points, uint64_t n_invalid_heading_points, uint64_t n_error_points, uint64_t n_ci_points, uint64_t n_pi_points) : 
        n_points(n_points),
        n_invalid_field_points(n_invalid_field_points),
        n_invalid_geo_points(n_invalid_geo_points),
        n_invalid_heading_points(n_invalid_heading_points),
        n_error_points(n_error_points),
        n_ci_points(n_ci_points),
        n_pi_points(n_pi_points)
    {}

   PointCounter PointCounter::operator+(const PointCounter& other) const {
        PointCounter point_counter(n_points + other.n_points, n_invalid_field_points + other.n_invalid_field_points, n_invalid_geo_points + other.n_invalid_geo_points, n_invalid_heading_points + other.n_invalid_heading_points, n_error_points + other.n_error_points, n_ci_points + other.n_ci_points, n_pi_points + other.n_pi_points);      
        return point_counter;
    }

    std::ostream& operator<<(std::ostream& os, const PointCounter& point_counter) {
        return os << point_counter.n_points <<  "," << point_counter.n_invalid_field_points << "," << point_counter.n_invalid_geo_points << "," << point_counter.n_invalid_heading_points << "," << point_counter.n_error_points << "," << point_counter.n_ci_points << "," << point_counter.n_pi_points;
    }
}
