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
#include "bsmp1.hpp"
#include "utilities.hpp"

#include <fstream>

namespace BSMP1 {
    BSMP1CSVTrajectoryFactory::BSMP1CSVTrajectoryFactory() :
        index_(0),
        line_number_(0)
        {}

    const std::string BSMP1CSVTrajectoryFactory::make_uid(const std::string& line) {
        StrVector parts = string_utilities::split(line, ',');

        if (parts.size() != kNFields) {
            throw std::out_of_range("BSMP1 CSV: Could not extract UID -> invalid number of fields");
        }

        return parts[0] + "_" + parts[1];
    }

    trajectory::Point::Ptr BSMP1CSVTrajectoryFactory::make_point(const std::string& line) {
        StrVector parts = string_utilities::split(line, ',');

        if (parts.size() != kNFields) {
            throw std::out_of_range("BSMP1 CSV: invalid number of fields");
        }

        double lat = std::stod(parts[7]);

        if (lat > 80.0 || lat < -84.0) {
            throw std::out_of_range("BSMP1 CSV: bad latitude: " + parts[7]);
        }

        double lon = std::stod(parts[8]);

        if (lon >= 180.0 || lon <= -180.0) {
            throw std::out_of_range("BSMP1 CSV: bad longitude: " + parts[8]);
        }

        if (lat == 0.0 && lon == 0.0) {
            throw std::out_of_range("BSMP1 CSV: equator point");
        }

        double heading = std::stod(parts[11]);

        if (heading > 360.0 || heading < 0.0) {
            throw std::out_of_range("BSMP1 CSV: bad heading: " + parts[11]);
        }

        double speed = std::stod(parts[10]);
        uint64_t gentime = std::stoull(parts[3]);

        return std::make_shared<trajectory::Point>(line, gentime, lat, lon, heading, speed, index_++);
    }

    trajectory::Point::Ptr BSMP1CSVTrajectoryFactory::make_point(const std::string& line, instrument::PointCounter& point_counter) {
        StrVector parts = string_utilities::split(line, ',');

        if (parts.size() != kNFields) {
            point_counter.n_invalid_field_points++;
            throw std::out_of_range("BSMP1 CSV: invalid number of fields");
        }

        double lat = std::stod(parts[7]);

        if (lat > 80.0 || lat < -84.0) {
            point_counter.n_invalid_geo_points++;
            throw std::out_of_range("BSMP1 CSV: bad latitude: " + parts[7]);
        }

        double lon = std::stod(parts[8]);

        if (lon >= 180.0 || lon <= -180.0) {
            point_counter.n_invalid_geo_points++;
            throw std::out_of_range("BSMP1 CSV: bad longitude: " + parts[8]);
        }

        if (lat == 0.0 && lon == 0.0) {
            point_counter.n_invalid_geo_points++;
            throw std::out_of_range("BSMP1 CSV: equator point");
        }

        double heading = std::stod(parts[11]);

        if (heading > 360.0 || heading < 0.0) {
            point_counter.n_invalid_heading_points++;
            throw std::out_of_range("BSMP1 CSV: bad heading: " + parts[11]);
        }

        double speed = std::stod(parts[10]);
        uint64_t gentime = std::stoull(parts[3]);

        return std::make_shared<trajectory::Point>(line, gentime, lat, lon, heading, speed, index_++);
    }
    
    const trajectory::Trajectory BSMP1CSVTrajectoryFactory::make_trajectory(const std::string& input) {
        std::string line;
        trajectory::Trajectory traj;
        std::ifstream file(input);

        if (file.fail()) {
            throw std::invalid_argument("Could not open BSMP1 CSV file: " + input);
        }

        if (!std::getline(file, line)) {
            throw std::invalid_argument("BSMP1 CSV: " + input + " missing header!");
        }

        if (!std::getline(file, line)) {
            throw std::invalid_argument("BSMP1 CSV: " + input + " is empty!");
        }

        uid_ = make_uid(line); 
        
        do {
            line_number_++;
    
            try {
                traj.push_back(make_point(line));
            } catch (std::exception&) {
                continue;
            }
        } while (std::getline(file, line));

        file.close();

        // NRVO / copy elision.
        return traj;
    }

    const trajectory::Trajectory BSMP1CSVTrajectoryFactory::make_trajectory(const std::string& input, instrument::PointCounter& point_counter) {
        std::string line;
        trajectory::Trajectory traj;
        std::ifstream file(input);

        if (file.fail()) {
            throw std::invalid_argument("Could not open BSMP1 CSV file: " + input);
        }

        if (!std::getline(file, line)) {
            throw std::invalid_argument("BSMP1 CSV: " + input + " missing header!");
        }

        if (!std::getline(file, line)) {
            throw std::invalid_argument("BSMP1 CSV: " + input + " is empty!");
        }

        uid_ = make_uid(line); 
        
        do {
            point_counter.n_points++;
            line_number_++;
    
            try {
                traj.push_back(make_point(line, point_counter));
            } catch (std::exception&) {
                continue;
            }
        } while (std::getline(file, line));

        file.close();

        // NRVO // copy elision
        return traj;
    }

    const std::string BSMP1CSVTrajectoryFactory::get_uid() const {
        return uid_;
    }

    BSMP1CSVTrajectoryWriter::BSMP1CSVTrajectoryWriter(const std::string& output) :
        output_(output)
        {}

    void BSMP1CSVTrajectoryWriter::write_trajectory(const trajectory::Trajectory& traj, const std::string& uid, bool strip_cr) const {
        std::string output_file_path;

        if (output_.empty()) {
            output_file_path = uid + ".csv";
        } else {
            output_file_path = output_ + "/" + uid + ".csv";
        }

        std::ofstream os(output_file_path, std::ofstream::trunc);

        if (os.fail()) {
            throw std::invalid_argument("Could not open BSMP1 CSV output file: " + output_file_path);
        }

        os << kCSVHeader << std::endl;

        for (auto& tp : traj) {
            std::string data = tp->get_data();

            if (strip_cr && !data.empty() && data[data.size() - 1] == '\r') {
                data.erase(data.size() - 1);
            }

            os << data << std::endl;
        }

        os.close();
    }
}
