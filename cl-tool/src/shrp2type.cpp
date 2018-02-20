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
#include "shrp2type.hpp"
#include "utilities.hpp"

#include <fstream>
#include <regex>
#include <iterator>

namespace track_types {

    SHRP2Reader::SHRP2Reader( const Config::DIConfig& conf, std::shared_ptr<instrument::PointCounter> counter )
        : conf_{ conf }
        , counter_{ counter }
        , index_{0}
    {}

    /**
     * \brief Build a unique file ID using the path of the file and set the current uid. If there are no numbers in the path or the path is
     * empty this will return UID_UNKNOWN.
     *
     * For the SHRP2 data the UID fields are embedded in the file name.
     *
     * filename: File_ID_<NBR>.csv
     * enclosing directory: VehicleID_<NBR>_DriverID_<NBR>
     */
    const std::string SHRP2Reader::make_uid(const std::string& filepath) {

        std::string s, uid;
        auto nbr_end = std::sregex_iterator();
        std::regex nbr_regex{"([0-9]+)"};

        // TODO: Unix specific.
        StrVector pieces = string_utilities::split( filepath, '/' );

        // second to last piece should have VID and DID.
        if ( pieces.size() > 1 ) {
            s = pieces.at( pieces.size()-2 );
            auto nbr_begin = std::sregex_iterator(s.begin(), s.end(), nbr_regex);
            for (std::sregex_iterator i = nbr_begin; i != nbr_end; ++i) {
                if ( !uid.empty() ) uid += "_";
                uid += (*i).str();
            }
        }

        // last piece will be the filename and have the file ID.
        if ( pieces.size() > 0 ) {
            s = pieces.back();
            auto nbr_begin = std::sregex_iterator(s.begin(), s.end(), nbr_regex);
            for (std::sregex_iterator i = nbr_begin; i != nbr_end; ++i) {
                if ( !uid.empty() ) uid += "_";
                uid +=  (*i).str();
            }
        } 

        if ( uid.empty() ) {			// no pieces.
            uid = "UNKNOWN";
        }

        return "UID_" + uid;
    }

    trajectory::Point::Ptr SHRP2Reader::make_point( const std::string& line ) {
        StrVector parts = string_utilities::split(line, ',');
        double lat, lon, heading, speed;
        int64_t gentime;

        if (parts.size() != conf_.num_fields) {
            if ( counter_ != nullptr ) counter_->n_invalid_field_points++;
            throw std::out_of_range("Trajectory record has incorrect number of fields: " + parts.size() ); 
        }

        try {
            // vtti.latitude [36] : degrees and decimal seconds. ddd.sssssssss. North positive, South negative.. 998xx.xxxx - invalid; 999xx.xxxx - unavailable.
            lat = std::stod(parts[conf_.lat_field_idx]);
        } catch ( std::exception& e ) {
            if ( counter_ != nullptr ) counter_->n_invalid_field_points++;
            throw std::invalid_argument( uid_ + ": invalid latitude '" + parts[conf_.lat_field_idx] + "'" );
        }

        try {
            lon = std::stod(parts[conf_.lon_field_idx]);
        } catch ( std::exception& e ) {
            if ( counter_ != nullptr ) counter_->n_invalid_field_points++;
            throw std::invalid_argument( uid_ + ": invalid longitude '" +parts[conf_.lon_field_idx] + "'");
        }

        try {
            // vtti.heading_gps [32]: in degrees, e.g, 0 = NORTH, 270 = west.
            heading = std::stod(parts[conf_.heading_field_idx]);
        } catch ( std::exception& e ) {
            if ( counter_ != nullptr ) counter_->n_invalid_field_points++;
            throw std::invalid_argument( uid_ + ": invalid heading '" +parts[conf_.heading_field_idx] + "'");
        }

        try {
            // vtti.speed_gps [53]: Convert KPH to MS
            speed = std::stod(parts[conf_.speed_field_idx]) * 1000.0/3600.0;
        } catch ( std::exception& e ) {
            if ( counter_ != nullptr ) counter_->n_invalid_field_points++;
            throw std::invalid_argument( uid_ + ": invalid speed '" +parts[conf_.speed_field_idx] + "'");
        }

        try {
            // vtti.timestamp [2]: in milliseconds from the beginning of the trip, e.g., 20400 = 20.4 seconds into trip.
            // milliseconds to microseconds
            gentime = std::stoull(parts[conf_.gentime_field_idx]) * 1000;
        } catch ( std::exception& e ) {
            if ( counter_ != nullptr ) counter_->n_invalid_field_points++;
            throw std::invalid_argument( uid_ + ": invalid gentime '" +parts[conf_.gentime_field_idx] + "'");
        }

        if (lat > 80.0 || lat < -84.0) {
            if ( counter_ != nullptr ) counter_->n_invalid_geo_points++;
            throw std::out_of_range( uid_ + ": trajectory has bad latitude: " + parts[conf_.lat_field_idx]);
        }

        // vtti.longitude [41]: degrees and decimal seconds. ddd.sssssssss. [-180,+180]. East positive, West negative.
        if (lon >= 180.0 || lon <= -180.0) {
            if ( counter_ != nullptr ) counter_->n_invalid_geo_points++;
            throw std::out_of_range( uid_ + ": trajectory has bad longitude: " + parts[conf_.lon_field_idx]);
        }

        if (lat == 0.0 && lon == 0.0) {
            if ( counter_ != nullptr ) counter_->n_invalid_geo_points++;
            throw std::out_of_range( uid_ + ": trajectory has equator position.");
        }

        if (heading > 360.0 || heading < 0.0) {
            if ( counter_ != nullptr ) counter_->n_invalid_heading_points++;
            throw std::out_of_range( uid_ + ": Trajectory has invalid heading: " + parts[conf_.heading_field_idx]);
        }

        // If any of the required fields cannot be converted ( empty string throws ), then the line will be skipped.
        return std::make_shared<trajectory::Point>(line, gentime, lat, lon, heading, speed, index_++);
    }
    
    const trajectory::Trajectory SHRP2Reader::make_trajectory(const std::string& infilename) {

        std::string line;
        trajectory::Trajectory traj;

        std::ifstream file(infilename);

        if (file.fail()) {
            throw std::invalid_argument("Could not open trajectory file: " + infilename);
        }

        if (!std::getline(file, line)) {
            throw std::invalid_argument("Trajectory file missing header or empty!");
        }

        if (!std::getline(file, line)) {
            throw std::invalid_argument("Trajectory file header only - empty!");
        }

        // UID in SHRP2 from the path to file.
        uid_ = make_uid( infilename ); 
        
        do {
            if ( counter_ ) counter_->n_points++;
            try {
                traj.push_back(make_point(line));
            } catch (std::out_of_range& e) {
                // std::cerr << e.what() << '\n';
                continue;
            } catch (std::invalid_argument& e) {
                // std::cerr << e.what() << '\n';
                continue;
            } catch (std::exception& e) {
                // std::cerr << e.what() << '\n';
                continue;
            }

        } while (std::getline(file, line));

        file.close();
        
        return traj; // NRVO
    }

    const std::string SHRP2Reader::get_uid() const {
        return uid_;
    }

    SHRP2Writer::SHRP2Writer(const std::string& outdir, const Config::DIConfig& conf) 
        : output_{ outdir }
        , conf_{ conf }
        {}

    void SHRP2Writer::write_trajectory(const trajectory::Trajectory& traj, const std::string& uid, bool strip_cr) const {
        std::string output_file_path;

        if (output_.empty()) {
            output_file_path = uid + ".csv";
        } else {
            output_file_path = output_ + "/" + uid + ".csv";
        }

        std::ofstream os(output_file_path, std::ofstream::trunc);

        if (os.fail()) {
            throw std::invalid_argument("Could not open trajectory output file: " + output_file_path);
        }

        if ( !conf_.outfile_header.empty() ) {
            os << conf_.outfile_header << std::endl;
        }

        for (auto& tp : traj) {
            std::string data = tp->get_data();

            if (strip_cr && !data.empty() && data[data.size() - 1] == '\r') {
                data.erase(data.size() - 1);
            }
            os << *tp << std::endl;
            //os << data << std::endl;
        }
        os.close();
    }
}
