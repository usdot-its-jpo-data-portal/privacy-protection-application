#include "di_multi.hpp"
#include "config.hpp"

#include <iomanip>
#include <chrono>
#include <ctime>

namespace DIMulti {
    // FileInfo
    SingleFileInfo::SingleFileInfo(const std::string& file_path, uint64_t size) :
        file_path_(file_path),
        size_(size)
        {}    

    SingleFileInfo::SingleFileInfo(const std::string& file_path, const std::string& aux_data, uint64_t size) :
        file_path_(file_path),
        aux_data_(aux_data),
        size_(size)
        {}

    uint64_t SingleFileInfo::GetSize() const {
        return size_;
    }

    const std::string SingleFileInfo::GetAuxData() const {
        return aux_data_;
    } 

    const std::string SingleFileInfo::GetFilePath() const {
        return file_path_;
    }

    // BatchCSV

    BatchCSV::BatchCSV(const std::string& file_path) :
        MultiThread::Parallel<FileInfo>(),
        file_path_(file_path)
    {
        std::ifstream file(file_path_, std::ios::binary | std::ios::ate);
        
        if (file.fail()) {
            throw std::invalid_argument("Could not open file: " + file_path_);
        }

        uint64_t size = file.tellg();
        file.close();
        file_ptr_ = std::make_shared<std::ifstream>(file_path_);
    }
    
    void BatchCSV::Init(unsigned n_used_threads) {
        // Do nothing.
        return;
    }

    IFSPtr BatchCSV::GetFilePtr() const {
        return file_ptr_;
    }

    uint64_t BatchCSV::ItemSize(FileInfo& trip_file) {
        return trip_file.GetSize();
    }

    void BatchCSV::Close() {
        file_ptr_->close();
    }

    // SingleBatchCSV
      
    SingleBatchCSV::SingleBatchCSV(const std::string& file_path) :
        BatchCSV(file_path)
        {}

    FileInfo::Ptr SingleBatchCSV::NextItem() {
        std::string line;

        while (std::getline(*(GetFilePtr()), line)) {
            line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
            StrVector items = string_utilities::split(line, ':');

            if (items.size() < 1) {
                continue;
            }

            std::string file_path = items[0];
            std::ifstream file(file_path, std::ios::binary | std::ios::ate);

            if (file.fail()) {  
                std::cerr << "Could not open file: " << file_path << std::endl; 

                continue;
            }
        
            uint64_t size = file.tellg();
        
            file.close();

            StrVector path_items = string_utilities::split(file_path, '/');

            if (path_items.size() < 1) {
                continue;
            }

            if (items.size() > 1) {
                return std::make_shared<SingleFileInfo>(file_path, items[1], size);
            } else {
                return std::make_shared<SingleFileInfo>(file_path, size);
            }
    
        }

        return nullptr;
    }

    DICSV::DICSV(const std::string& file_path, const std::string& quad_file_path, const std::string& out_dir_path, const std::string& config_file_path, const std::string& kml_dir_path, bool count_points) :
        SingleBatchCSV(file_path),
        out_dir_path_(out_dir_path),
        kml_dir_path_(kml_dir_path), 
        count_points_(count_points)
        {
            if (!config_file_path.empty()) {
                config_ptr_ = Config::DIConfig::ConfigFromFile(config_file_path);
            } else {
                config_ptr_ = std::make_shared<Config::DIConfig>();
            }

            config_ptr_->PrintConfig(std::cerr);
            geo::Point sw{ config_ptr_->GetQuadSWLat(), config_ptr_->GetQuadSWLng() };
            geo::Point ne{ config_ptr_->GetQuadNELat(), config_ptr_->GetQuadNELng() };

            quad_ptr_ = std::make_shared<Quad>(sw, ne);
            shapes::CSVInputFactory shape_factory(quad_file_path);
            shape_factory.make_shapes();

            for (auto& edge_ptr : shape_factory.get_edges()) {
                Quad::insert(quad_ptr_, std::dynamic_pointer_cast<const geo::Entity>(edge_ptr)); 
            }
        }
    
    void DICSV::Init(unsigned n_used_threads) {
        SingleBatchCSV::Init(n_used_threads);

        if (!count_points_) {
            return;
        }

        for (unsigned i = 0; i < n_used_threads; ++i) {
            counters_.push_back(std::make_shared<instrument::PointCounter>());
        }
    }

    trajectory::Trajectory DICSV::DeIdentify(trajectory::Trajectory& traj, const std::string& uid) const {
        bool plot_kml = config_ptr_->IsPlotKML();
        std::string shape_in_file_path, shape_out_file_path;
    
        ErrorCorrector ec(50);
        ec.correct_error(traj, uid);

        MapFitter mf{quad_ptr_, config_ptr_->GetMapFitScale(), config_ptr_->GetFitExt()};
        mf.fit(traj);

        ImplicitMapFitter imf{config_ptr_->GetHeadingGroups(), config_ptr_->GetMinEdgeTripPoints()};
        imf.fit(traj);

        IntersectionCounter ic{};
        ic.count_intersections(traj);

        Detector::TurnAround tad{config_ptr_->GetTAMaxQSize(), config_ptr_->GetTAAreaWidth(), config_ptr_->GetTAMaxSpeed(), config_ptr_->GetTAHeadingDelta()};
        trajectory::Interval::PtrList ta_critical_intervals = tad.find_turn_arounds(traj);

        Detector::Stop stop_detector{config_ptr_->GetStopMaxTime(), config_ptr_->GetStopMinDistance(), config_ptr_->GetStopMaxSpeed()};
        trajectory::Interval::PtrList stop_critical_intervals = stop_detector.find_stops( traj );

        StartEndIntervals sei;

        IntervalMarker im( { ta_critical_intervals, stop_critical_intervals, sei.get_start_end_intervals( traj ) } );
        im.mark_trajectory( traj );

        trajectory::Interval::PtrList priv_intervals;

        PrivacyIntervalFinder pif(config_ptr_->GetMinDirectDistance(), 
                                  config_ptr_->GetMinManhattanDistance(), 
                                  config_ptr_->GetMinOutDegree(), 
                                  config_ptr_->GetMaxDirectDistance(), 
                                  config_ptr_->GetMaxManhattanDistance(), 
                                  config_ptr_->GetMaxOutDegree(), 
                                  config_ptr_->GetRandDirectDistance(), 
                                  config_ptr_->GetRandManhattanDistance(), 
                                  config_ptr_->GetRandOutDegree());
        priv_intervals = pif.find_intervals( traj );

        PrivacyIntervalMarker pim({ priv_intervals });
        pim.mark_trajectory(traj);

        if (plot_kml) {
            std::string kml_path;
    
            if (kml_dir_path_.empty()) {
                kml_path = uid + ".di.kml";
            } else {
                kml_path = kml_dir_path_ + "/" + uid + ".di.kml";
            }
    
            std::ofstream out_file(kml_path, std::ofstream::trunc);
    
            if (out_file.fail()) {
                throw std::invalid_argument("Could not open kml output file: " + kml_path);
            }
    
            KML::File kml_file(out_file, uid);
    
            kml_file.write_poly_style( "explicit_boxes", 0xff990000, 1 );
            kml_file.write_poly_style( "implicit_boxes", 0xff0033ff, 1 );
            kml_file.write_line_style( "ci_intervals", 0xffff00ff, 7 );
            kml_file.write_line_style( "priv_intervals", 0xffffff00, 5 );
            kml_file.write_trajectory( traj, true );
            kml_file.write_areas(mf.area_set, "explicit_boxes");
            kml_file.write_areas(imf.area_set, "implicit_boxes");
            kml_file.write_intervals( stop_critical_intervals, traj, "ci_intervals", "stop_marker_style" );
            kml_file.write_intervals( ta_critical_intervals, traj, "ci_intervals", "turnaround_marker_style" );
            kml_file.write_intervals( priv_intervals, traj, "priv_intervals" );
            kml_file.finish();
    
            out_file.close();
        }

        DeIdentifier di;

        return di.de_identify(traj);
    }

    trajectory::Trajectory DICSV::DeIdentify(trajectory::Trajectory& traj, const std::string& uid, instrument::PointCounter& point_counter) const {
        bool plot_kml = config_ptr_->IsPlotKML();
        std::string shape_in_file_path, shape_out_file_path;
    
        ErrorCorrector ec(50);
        ec.correct_error(traj, uid, point_counter);

        MapFitter mf{quad_ptr_, config_ptr_->GetMapFitScale(), config_ptr_->GetFitExt()};
        mf.fit(traj);

        ImplicitMapFitter imf{config_ptr_->GetHeadingGroups(), config_ptr_->GetMinEdgeTripPoints()};
        imf.fit(traj);

        IntersectionCounter ic{};
        ic.count_intersections(traj);

        Detector::TurnAround tad{config_ptr_->GetTAMaxQSize(), config_ptr_->GetTAAreaWidth(), config_ptr_->GetTAMaxSpeed(), config_ptr_->GetTAHeadingDelta()};
        trajectory::Interval::PtrList ta_critical_intervals = tad.find_turn_arounds(traj);

        Detector::Stop stop_detector{config_ptr_->GetStopMaxTime(), config_ptr_->GetStopMinDistance(), config_ptr_->GetStopMaxSpeed()};
        trajectory::Interval::PtrList stop_critical_intervals = stop_detector.find_stops( traj );

        StartEndIntervals sei;

        IntervalMarker im( { ta_critical_intervals, stop_critical_intervals, sei.get_start_end_intervals( traj ) } );
        im.mark_trajectory( traj );

        trajectory::Interval::PtrList priv_intervals;

        PrivacyIntervalFinder pif(config_ptr_->GetMinDirectDistance(), 
                                  config_ptr_->GetMinManhattanDistance(), 
                                  config_ptr_->GetMinOutDegree(), 
                                  config_ptr_->GetMaxDirectDistance(), 
                                  config_ptr_->GetMaxManhattanDistance(), 
                                  config_ptr_->GetMaxOutDegree(), 
                                  config_ptr_->GetRandDirectDistance(), 
                                  config_ptr_->GetRandManhattanDistance(), 
                                  config_ptr_->GetRandOutDegree());
        priv_intervals = pif.find_intervals( traj );

        PrivacyIntervalMarker pim({ priv_intervals });
        pim.mark_trajectory(traj);

        if (plot_kml) {
            std::string kml_path;
    
            if (kml_dir_path_.empty()) {
                kml_path = uid + ".di.kml";
            } else {
                kml_path = kml_dir_path_ + "/" + uid + ".di.kml";
            }
    
            std::ofstream out_file(kml_path, std::ofstream::trunc);
    
            if (out_file.fail()) {
                throw std::invalid_argument("Could not open kml output file: " + kml_path);
            }
    
            KML::File kml_file(out_file, uid);
    
            kml_file.write_poly_style( "explicit_boxes", 0xff990000, 1 );
            kml_file.write_poly_style( "implicit_boxes", 0xff0033ff, 1 );
            kml_file.write_line_style( "ci_intervals", 0xffff00ff, 7 );
            kml_file.write_line_style( "priv_intervals", 0xffffff00, 5 );
            kml_file.write_trajectory( traj, true );
            kml_file.write_areas(mf.area_set, "explicit_boxes");
            kml_file.write_areas(imf.area_set, "implicit_boxes");
            kml_file.write_intervals( stop_critical_intervals, traj, "ci_intervals", "stop_marker_style" );
            kml_file.write_intervals( ta_critical_intervals, traj, "ci_intervals", "turnaround_marker_style" );
            kml_file.write_intervals( priv_intervals, traj, "priv_intervals" );
            kml_file.finish();
    
            out_file.close();
        }

        DeIdentifier di;

        return di.de_identify(traj, point_counter);
    }

    void DICSV::Thread(unsigned thread_num, MultiThread::SharedQueue<FileInfo::Ptr>* q) {
        SingleFileInfo::Ptr trip_file_ptr;
        trajectory::Trajectory traj;
        BSMP1::BSMP1CSVTrajectoryWriter traj_writer(out_dir_path_);

        while ((trip_file_ptr = std::dynamic_pointer_cast<SingleFileInfo>(q->pop())) != nullptr) {
            if (count_points_) {
                try {
                    BSMP1::BSMP1CSVTrajectoryFactory factory;
                    std::shared_ptr<instrument::PointCounter> point_counter_ptr = counters_[thread_num];
                    traj = factory.make_trajectory(trip_file_ptr->GetFilePath(), *point_counter_ptr);
                    traj_writer.write_trajectory(DeIdentify(traj, factory.get_uid(), *point_counter_ptr), factory.get_uid(), true);
                } catch (std::exception& e) {
                    std::cerr << "DeIdentification error: " << e.what() << std::endl;
    
                    continue;
                }
            } else {
                try {
                    BSMP1::BSMP1CSVTrajectoryFactory factory;
                    traj = factory.make_trajectory(trip_file_ptr->GetFilePath());
                    traj_writer.write_trajectory(DeIdentify(traj, factory.get_uid()), factory.get_uid(), true);
                } catch (std::exception& e) {
                    std::cerr << "DeIdentification error: " << e.what() << std::endl;
    
                    continue;
                }
            }
        }
    }

    void DICSV::Close(void) {
        SingleBatchCSV::Close();

        if (!count_points_) {
            return;
        }
        
        instrument::PointCounter summary;

        for (auto& point_counter_ptr : counters_) {
            summary = summary + *point_counter_ptr; 
        }
    
        std::cerr << "********************************** Point Summary ****************************************" << std::endl;
        std::cerr << "total,invalid_fields,invalid_GPS,invalid_heading,error,critical_interval,privacy_interval" << std::endl;
        std::cerr << summary << std::endl;
        std::cerr << "*****************************************************************************************" << std::endl;
    }
}
