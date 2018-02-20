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
#ifndef CTES_DI_ERROR_HPP
#define CTES_DI_ERROR_HPP

#include "instrument.hpp"
#include "trajectory.hpp"

/**
 * \brief GPS measurements are sometime inaccurate. When the inaccuracies manifest as specific points (North or South
 * Pole), they are easy to detect and throw out.  Sometimes inaccuracies manifest as non-specific points.  This class
 * seeks to detect those anomolous points. We see these points often at the beginning of trips when the GPS is
 * attempting to lock onto a signal.  Our method uses a running average.
 */
class ErrorCorrector
{
    public:

        /**
         * \brief Construct an ErrorCorrector that will operate using a given sample size.
         *
         * \param sample_size The number of points to use for detecting anomolies.
         */
        ErrorCorrector(uint64_t sample_size, std::shared_ptr<instrument::PointCounter> pc = nullptr);

        /**
         * \brief Examine sample_size points from the beginning and ending of the traj and remove those points that were
         * found to be inaccurate.  When points are removed, the trip is re-indexed.
         *
         * \param The trajectory to examine.
         * \param The UID of the trajectory.
         */
        void correct_error(trajectory::Trajectory& traj, const std::string& uid);

        /**
         * \brief Examine sample_size points from the beginning and ending of the traj and remove those points that were
         * found to be inaccurate.  When points are removed, the trip is re-indexed.  A count of the erroneous points is
         * stored in the point_counter.
         *
         * \param The trajectory to examine.
         * \param The UID of the trajectory.
         */
        // void correct_error(trajectory::Trajectory& traj, const std::string& uid, instrument::PointCounter& point_counter);

    private:
        void remove_points(trajectory::Trajectory& traj, uint64_t start, uint64_t end, const std::string& uid);
        // void remove_points(trajectory::Trajectory& traj, uint64_t start, uint64_t end, const std::string& uid, instrument::PointCounter& point_counter);
        void correct_indices(trajectory::Trajectory& traj);

        uint64_t sample_size_;
        bool is_explicit_edge_;
        geo::EdgeCPtr current_eptr_;
        geo::EdgeCPtr previous_eptr_;
        std::vector<trajectory::Point> edge_pts_;
        std::shared_ptr<instrument::PointCounter> pc_;
};

#endif
