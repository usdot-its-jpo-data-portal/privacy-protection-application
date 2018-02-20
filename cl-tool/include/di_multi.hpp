#ifndef DI_MULTI_HPP
#define DI_MULTI_HPP

#include "config.hpp"
#include "cvlib.hpp"
#include "multi_thread.hpp"
#include "shrp2type.hpp"
#include "tracktypes.hpp"

namespace DIMulti {
    using IFSTPtr = std::shared_ptr<std::ifstream>;

    /**
     * \brief An abstract base class containing information about a file containing one or more trips.
     */
    class FileInfo {
        public:
            using Ptr = std::shared_ptr<FileInfo>;
            
            virtual const std::string GetFilePath(void) const = 0;
            virtual uint64_t GetSize(void) const = 0;
    };

    /**
     * \brief A class representing a file containing a single trip. Each instance can be annotated with auxiliary
     * information. Size is used to distribute work across threads.
     */
    class SingleFileInfo : public FileInfo {
        public:
            using Ptr = std::shared_ptr<SingleFileInfo>;

            SingleFileInfo(const std::string& file_path, uint64_t size);
            SingleFileInfo(const std::string& file_path, const std::string& aux_data, uint64_t size);

            const std::string GetFilePath(void) const;
            uint64_t GetSize(void) const;
            const std::string GetAuxData(void) const;
        private:
            std::string file_path_;
            std::string aux_data_;
            uint64_t size_;
    };

    /**
     * \brief A class that processes files containing trips in parallel.
     */
    class BatchCSV : public MultiThread::Parallel<FileInfo>
    {
        public:
            /**
             * \brief Construct a batch processor that uses the provided file to find the trip files that needed to be
             * processed.
             *
             * \param file_path a file having a trip file on each line.
             */
            BatchCSV(const std::string& file_path);

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
            uint64_t ItemSize(FileInfo& trip_file);
            virtual FileInfo::Ptr NextItem(void) = 0;
        private:
            std::string file_path_;
            std::string header_;
            IFSPtr file_ptr_;
            std::string multi_file_path_;
    };

    class SingleBatchCSV : public BatchCSV {
        public:
            SingleBatchCSV(const std::string& file_path);
            virtual void Thread(unsigned thread_num, MultiThread::SharedQueue<FileInfo::Ptr>* q) = 0;
            FileInfo::Ptr NextItem(void);
    };

    class DICSV : public SingleBatchCSV
    {
        public:
            DICSV(const std::string& file_path, const std::string& quad_file_path, const std::string& out_dir_path, const std::string& config_file_path, const std::string& kml_dir_path, bool count_points=false);
            void Init(unsigned n_used_threads);
            void Close(void);
            void Thread(unsigned thread_num, MultiThread::SharedQueue<FileInfo::Ptr>* q);
        private:
            Config::DIConfig::Ptr config_ptr_;
            std::string out_dir_path_;
            std::string kml_dir_path_;
            bool count_points_;
            Quad::Ptr quad_ptr_;
            std::vector<std::shared_ptr<instrument::PointCounter>> counters_;

            trajectory::Trajectory DeIdentify(trajectory::Trajectory& traj, const std::string& uid, std::shared_ptr<instrument::PointCounter> pc ) const;
    };
}

#endif
