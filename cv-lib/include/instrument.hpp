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
#ifndef CTES_DI_INSTRUMENT_HPP
#define CTES_DI_INSTRUMENT_HPP

#include <cstdint>
#include <iostream>

namespace instrument {

    /**
     * \brief A class to gather statistics about a trajectory.
     */
    struct PointCounter 
    {
        uint64_t n_points;                      ///> The number of points in a trajectory.
        uint64_t n_invalid_field_points;        ///> The number of points with invalid field values.
        uint64_t n_invalid_geo_points;          ///> The number of points with invalid position.
        uint64_t n_invalid_heading_points;      ///> The number of points with an invalid heading.
        uint64_t n_error_points;                ///> The number of points with errors.
        uint64_t n_ci_points;                   ///> The number of points in critical intervals.
        uint64_t n_pi_points;                   ///> The number of points in privacy intervals.

        /**
         * \brief Default constructor.
         *
         * All statistics are initialized to 0.
         */
        PointCounter();

        /**
         * \brief Constructor with initialization of count values.
         *
         * \param n_points initial number of point value.
         * \param n_invalid_field_points initial number of points with invalid fields.
         * \param n_invalid_geo_points initial number of points that have invalid geo coordinates.
         * \param n_invalid_heading_points initial number of points that have invalid headings.
         * \param n_error_points initial number of points with other errors.
         * \param n_ci_points initial number of points in a critical interval.
         * \param n_pi_points initial number of points in a privacy interval.
         */
        PointCounter(uint64_t n_points, uint64_t n_invalid_field_points, uint64_t n_invalid_geo_points, uint64_t n_invalid_heading_points, uint64_t n_error_points, uint64_t n_ci_points, uint64_t n_pi_points);

        /**
         * \brief Return a new PointCounter instance = this PointCounter + other PointCounter. Values in each
         * PointCounter are added together.
         *
         * This PointCounter is NOT modified.
         *
         * \param other the PointCounter whose value are to be added to this point counter.
         * \return a NEW PointCounter instance.
         */
        PointCounter operator+( const PointCounter& other) const;
        
        /**
         * \brief Write a PointCounter to an output stream.
         *
         * The form of the output is a comma-delimited list of the various values managed by the PointCounter.
         *
         * \param os the output stream to write the string form of this PointCounter
         * \param point_counter the PointCounter to write.
         * \return the output stream after the write for chaining.
         */
        friend std::ostream& operator<<( std::ostream& os, const PointCounter& point_counter );
    };
}

#endif
