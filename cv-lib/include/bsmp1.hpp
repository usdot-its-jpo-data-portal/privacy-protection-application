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
#ifndef CTES_BSMP1_HPP
#define CTES_BSMP1_HPP

#include "instrument.hpp"
#include "trajectory.hpp"

namespace BSMP1 {

    const std::string kCSVHeader = "RxDevice,FileId,TxDevice,Gentime,TxRandom,MsgCount,DSecond,Latitude,Longitude,Elevation,Speed,Heading,Ax,Ay,Az,Yawrate,PathCount,RadiusOfCurve,Confidence";
    const uint32_t kNFields = 19;
    
    /**
     * \brief Instances of this class build trajectories from the BSMP1 dataset.
     */
    class BSMP1CSVTrajectoryFactory : public trajectory::TrajectoryFactory {
        public:

            /**
             * \brief Default constructor.
             */
            BSMP1CSVTrajectoryFactory(void);

            /**
             * \brief Build a Trajectory instance from an input file.
             *
             * \param input the name of the file containing the trajectory data.
             * \return a Trajectory instance (a vector of pointers to Point instances).
             * \throws invalid argument if the file cannot be opened or it doesn't have a header.
             */
            const trajectory::Trajectory make_trajectory(const std::string& input);

            /**
             * \brief Build a Trajectory instance from an input file and count the number of points in the trajectory.
             *
             * \param input the name of the file containing the trajectory data.
             * \param point_counter a PointCounter instance that keeps track of various statistics about a trajectory.
             * \return a Trajectory instance (a vector of pointers to Point instances).
             * \throws invalid argument if the file cannot be opened or it doesn't have a header.
             */
            const trajectory::Trajectory make_trajectory(const std::string& input, instrument::PointCounter& point_counter);

            /**
             * \brief Return the current trajectory unique identifier.
             *
             * \return the UID as a string.
             */
            const std::string get_uid(void) const;

            /**
             * \brief Build and return a BSMP1 UID
             *
             * The UID is constructed from the first two fields in each record.
             *
             * \param line the trip point record from the file that contains the UID information.
             * \throws out_of_range if the number of fields in the record exceeds what is expected for BSMP1.
             */
            static const std::string make_uid(const std::string& line);

        private:
            uint64_t index_;
            uint64_t line_number_;
            std::string uid_;


            /**
             * \brief Using the provided point record from an input file, make and return a shared pointer to the Point instance.
             *
             * \param line a line from a trajectory file that represents data for a single point.
             * \throws out_of_range if the number of fields in the record exceeds expectations, the geolocation latitude
             * and longitude is out of range, and heading is outside of the interval: [0,360].
             */
            trajectory::Point::Ptr make_point(const std::string& line);

            /** \brief Using the provided point record from an input file, make and return a shared pointer to the Point
             * instance and update the provided PointCounter.
             *
             * \param line a line from a trajectory file that represents data for a single point.  
             * \param point_counter a PointCounter instance to update based on the exception checks
             *
             * \throws out_of_range if the number of fields in the record exceeds expectations, the geolocation latitude
             * and longitude is out of range, and heading is outside of the interval: [0,360].
             */
            trajectory::Point::Ptr make_point(const std::string& line, instrument::PointCounter& point_counter); };

    /**
     * \brief Instances of this class write trajectories in the BSMP1 form.
     */
    class BSMP1CSVTrajectoryWriter : public trajectory::TrajectoryWriter {
        public:

            /**
             * \brief Constructor
             *
             * \param output directory in which to store the file containing the trajectory data; file names are based
             * on the unique id of the trajectory.
             */
            BSMP1CSVTrajectoryWriter(const std::string& output);

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
            std::string output_;            ///> The output directory.
    };
}

#endif
