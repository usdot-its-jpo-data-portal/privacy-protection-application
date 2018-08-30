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
#ifndef CVDI_MULTI_HPP
#define CVDI_MULTI_HPP

#include "config.hpp"
#include "cvdi.hpp"
#include "hmm_mm.hpp"
#include "multi_thread.hpp"

#include "spdlog/spdlog.h"

/** \brief Multi-threaded connected vehicle command line tool. */
namespace cvdi_multi {

using IFSPtr = std::shared_ptr<std::ifstream>;

/**
 * \brief An abstract base class containing information about a file containing one or more trips.
 */
class FileInfo {
    public:
        using Ptr = std::shared_ptr<FileInfo>;
        
        virtual const std::string GetFilePath(void) const = 0;
};

/**
 * \brief A class representing a file containing a single trip. Each instance can be annotated with auxiliary
 * information. Size is used to distribute work across threads.
 */
class SingleFileInfo : public FileInfo {
    public:
        using Ptr = std::shared_ptr<SingleFileInfo>;

        SingleFileInfo(const std::string& file_path);
        SingleFileInfo(const std::string& file_path, const std::string& aux_data);

        const std::string GetFilePath(void) const;
        const std::string GetAuxData(void) const;
    private:
        std::string file_path_;
        std::string aux_data_;
};

/**
 * \brief A class that processes files containing trips in parallel.
 */
class BatchParallel : public MultiThread::Parallel<FileInfo>
{
    public:
        /**
         * \brief Construct a batch processor that uses the provided file to find the trip files that needed to be
         * processed.
         *
         * \param file_path a file having a trip file on each line.
         */
        BatchParallel(const std::string& file_path);

        /**
         * \brief The main work thread.
         *
         * \param thread_num a easy to read unique identifier for this thread.
         * \param the work queue containing the trip files that this thread should process.
         */
        virtual void Thread(unsigned thread_num, MultiThread::SharedQueue<FileInfo::Ptr>* q) = 0;

        /**
         * \brief an initialization method; override to perform a concrete initialization action.
         *
         * \param n_used_threads the number of threads that will be managed by this instance.
         */
        void Init(unsigned n_used_threads);

        /**
         * \brief Close the batch file.
         */
        void Close(void);
        IFSPtr GetFilePtr(void) const;
        virtual FileInfo::Ptr NextItem(void) = 0;
    private:
        std::string file_path_;
        std::string header_;
        IFSPtr file_ptr_;
        std::string multi_file_path_;
};

/**
 * \brief Single file batch processing. 
 */
class SingleBatchParallel : public BatchParallel {
    public:
        SingleBatchParallel(const std::string& file_path);
        virtual void Thread(unsigned thread_num, MultiThread::SharedQueue<FileInfo::Ptr>* q) = 0;
        FileInfo::Ptr NextItem(void);
};

/**
 * \brief Single file CVDI batch processing application.
 */
class CVDIParallel : public SingleBatchParallel {
    public:
        CVDIParallel(const std::string file_path, const std::string osm_file, const std::string out_dir, const std::string config_file);
        void Init(unsigned n_used_threads);
        void Close(void);
        void Thread(unsigned thread_num, MultiThread::SharedQueue<FileInfo::Ptr>* q);
    private:
        std::string out_dir_;
        bool save_mm_;
        bool plot_kml_;
        bool count_pts_;

        hmm_mm::RoadMap::Ptr road_map_ptr_;
        std::vector<cvdi::PointCounter> counters_;
        config::Config::Ptr cvdi_config_;
        std::shared_ptr<spdlog::logger> ilogger_;
        std::shared_ptr<spdlog::logger> elogger_;
};
}

#endif
