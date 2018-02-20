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
#include "error.hpp"

#include <algorithm>
#include <cmath>

ErrorCorrector::ErrorCorrector(uint64_t sample_size, std::shared_ptr<instrument::PointCounter> pc ) 
    : sample_size_(sample_size)
    , is_explicit_edge_(false)
    , current_eptr_(nullptr)
    , pc_{ pc }
{}

void ErrorCorrector::correct_error(trajectory::Trajectory& traj, const std::string& uid) {
    if (traj.size() <= 1) {
        return;
    }

    remove_points(traj, 0, sample_size_, uid);

    if (traj.size() <= sample_size_) {
        correct_indices(traj);
        return;
    }

    remove_points(traj, traj.size() - sample_size_, traj.size(), uid);
    correct_indices(traj);
}

// void ErrorCorrector::correct_error(trajectory::Trajectory& traj, const std::string& uid, instrument::PointCounter& point_counter) {
//     if (traj.size() <= 1) {
//         return;
//     }
// 
//     remove_points(traj, 0, sample_size_, uid, point_counter);
// 
//     if (traj.size() <= sample_size_) {
//         correct_indices(traj);
// 
//         return;
//     }
// 
//     remove_points(traj, traj.size() - sample_size_, traj.size(), uid, point_counter);
// 
//     correct_indices(traj);
// }

// void ErrorCorrector::remove_points(trajectory::Trajectory& traj, uint64_t start, uint64_t end, const std::string& uid) {
//     std::vector<double> lats;
//     std::vector<double> lons;
// 
//     for (uint64_t i = start; i < traj.size() && i < end; ++i) {
//         lats.push_back(traj[i]->lat);
//         lons.push_back(traj[i]->lon);
//     }
// 
//     std::sort(lats.begin(), lats.end());
//     std::sort(lons.begin(), lons.end());
// 
//     uint64_t med_index = sample_size_ / 2;
//     double med_lat = lats[med_index];
//     double med_lon = lons[med_index];
// 
//     double time_est = (static_cast<double>(lats.size()) / 2.0) * 0.1;
// 
//     for (uint64_t i = start; i < traj.size() && i < end;) {
//         double distance = geo::Location::distance(traj[i]->lat, traj[i]->lon, med_lat, med_lon);
// 
//         // 44.7 m/s = 100 mph (a heuristic)
//         if (distance / time_est > 44.7) {
//             traj.erase(traj.begin() + i);
//         } else {
//             ++i;
//         }
//     }
// }

void ErrorCorrector::remove_points(trajectory::Trajectory& traj, uint64_t start, uint64_t end, const std::string& uid ) {
    std::vector<double> lats;
    std::vector<double> lons;

    for (uint64_t i = start; i < traj.size() && i < end; ++i) {
        lats.push_back(traj[i]->lat);
        lons.push_back(traj[i]->lon);
    }

    std::sort(lats.begin(), lats.end());
    std::sort(lons.begin(), lons.end());

    uint64_t med_index = sample_size_ / 2;
    double med_lat = lats[med_index];
    double med_lon = lons[med_index];
    double time_est = (static_cast<double>(lats.size()) / 2.0) * 0.1;

    for (uint64_t i = start; i < traj.size() && i < end;) {
        double distance = geo::Location::distance(traj[i]->lat, traj[i]->lon, med_lat, med_lon);

        if (distance / time_est > 44.7) {
            traj.erase(traj.begin() + i);
            if ( pc_ != nullptr ) pc_->n_error_points++;
        } else {
            ++i;
        }
    }
}

void ErrorCorrector::correct_indices(trajectory::Trajectory& traj) {
    for (uint64_t i = 0; i < traj.size(); ++i) {
        traj[i]->set_index(i);
    }
}
