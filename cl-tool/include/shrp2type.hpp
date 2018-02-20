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
#ifndef CTES_SHRP2_HPP
#define CTES_SHRP2_HPP

#include "instrument.hpp"
#include "trajectory.hpp"
#include "config.hpp"

namespace track_types {

    /**
     * \brief Instances of this class build trajectories from the BSMP1 dataset.
     */
    class SHRP2Reader : public trajectory::TrajectoryFactory {
        public:

            /**
             * \brief Build and return a BSMP1 UID
             *
             * The UID is constructed from the first two fields in each record.
             *
             * \param line the trip point record from the file that contains the UID information.
             * \throws out_of_range if the number of fields in the record exceeds what is expected for BSMP1.
             */
            static const std::string make_uid(const std::string& infilename);


            /**
             * \brief Default SHRP2 Trajectory Reader
             */
            SHRP2Reader( const Config::DIConfig& conf, std::shared_ptr<instrument::PointCounter> counter = nullptr );

            /**
             * \brief Build a Trajectory instance from an input file.
             *
             * \param input the name of the file containing the trajectory data.
             * \return a Trajectory instance (a vector of pointers to Point instances).
             * \throws invalid argument if the file cannot be opened or it doesn't have a header.
             */
            const trajectory::Trajectory make_trajectory(const std::string& input);

            /**
             * \brief Return the current trajectory unique identifier.
             *
             * \return the UID as a string.
             */
            const std::string get_uid(void) const;

        private:
            uint64_t index_;                ///< the current index into the trip.
            std::string uid_;               ///< unique identifier for this trip.
            const Config::DIConfig& conf_;  ///< The configuration for processing.
            std::shared_ptr<instrument::PointCounter> counter_; ///< pointer to the counter.

            /**
             * \brief Using the provided point record from an input file, make and return a shared pointer to the Point instance.
             *
             * \param line a line from a trajectory file that represents data for a single point.
             * \throws out_of_range if the number of fields in the record exceeds expectations, the geolocation latitude
             * and longitude is out of range, and heading is outside of the interval: [0,360].
             */
            trajectory::Point::Ptr make_point(const std::string& line);
    };


    /**
     * \brief Instances of this class write trajectories in the SHRP2 form.
     */
    class SHRP2Writer : public trajectory::TrajectoryWriter {
        public:

            /**
             * \brief Constructor
             *
             * \param output directory in which to store the file containing the trajectory data; file names are based
             * on the unique id of the trajectory.
             */
            SHRP2Writer(const std::string& output, const Config::DIConfig& conf);

            /**
             * \brief Write a trajectory to a file named based on the trajectories unique id (uid).
             *
             * \param trajectory the trajectory to write.
             * \param uid the trajectories UID -- this will be used to name the output file.
             * \param strip_cr flag to signal carriage returns should be removed.
             *
             * \throws invalid_argument when the output stream cannot be opened.
             */
            void write_trajectory(const trajectory::Trajectory& traj, const std::string& uid, bool strip_cr) const;

        private:
            std::string output_;            ///< The output directory.
            const Config::DIConfig& conf_;  ///< The configuration for processing.
    };
}

#endif
