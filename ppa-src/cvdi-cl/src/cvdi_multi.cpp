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
#include "cvdi_multi.hpp"

#include <iomanip>
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>

#include "tool.hpp"
#include "geo.hpp"
#include "geo_data.hpp"
#include "util.hpp"

namespace cvdi_multi {

// FileInfo
SingleFileInfo::SingleFileInfo(const std::string& file_path) :
    file_path_(file_path)
    {}    

SingleFileInfo::SingleFileInfo(const std::string& file_path, const std::string& aux_data) :
    file_path_(file_path),
    aux_data_(aux_data)
    {}

const std::string SingleFileInfo::GetAuxData() const {
    return aux_data_;
} 

const std::string SingleFileInfo::GetFilePath() const {
    return file_path_;
}

// BatchParallel

BatchParallel::BatchParallel(const std::string& file_path) :
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

void BatchParallel::Init(unsigned n_used_threads) {
    // Do nothing.
    return;
}

IFSPtr BatchParallel::GetFilePtr() const {
    return file_ptr_;
}

void BatchParallel::Close() {
    file_ptr_->close();
}

// SingleBatchParallel
  
SingleBatchParallel::SingleBatchParallel(const std::string& file_path) :
    BatchParallel(file_path)
    {}

FileInfo::Ptr SingleBatchParallel::NextItem() {
    std::string line;

    while (std::getline(*(GetFilePtr()), line)) {
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
        util::StrVector items = util::split_string(line, ':');

        if (items.size() < 1) {
            continue;
        }

        std::string file_path = items[0];
        util::StrVector path_items = util::split_string(file_path, '/');

        if (path_items.size() < 1) {
            continue;
        }

        if (items.size() > 1) {
            return std::make_shared<SingleFileInfo>(file_path, items[1]);
        } else {
            return std::make_shared<SingleFileInfo>(file_path);
        }

    }

    return nullptr;
}

CVDIParallel::CVDIParallel(const std::string file_path, const std::string osm_file, const std::string out_dir, const std::string config_file) :
    SingleBatchParallel(file_path), 
    out_dir_(out_dir)
{
    // loggers
    std::string ilog_path;
    std::string elog_path;

    if (out_dir.empty()) {
        ilog_path = "cvdi_info.log";
        elog_path = "cvdi_error.log";
    } else {
        ilog_path = out_dir + "/cvdi_info.log";
        elog_path = out_dir + "/cvdi_error.log";
    }  

    ilogger_ = spdlog::rotating_logger_mt("ilog", ilog_path, 1048576 * 5, 5);
    ilogger_->set_pattern("[%C%m%d %H:%M:%S.%f] [%l] %v");
    ilogger_->set_level( spdlog::level::trace );

    // setup error logger.
    elogger_ = spdlog::rotating_logger_mt("elog", elog_path, 1048576 * 2, 2);
    elogger_->set_pattern("[%C%m%d %H:%M:%S.%f] [%l] %v");
    elogger_->set_level( spdlog::level::err );

    // config
    cvdi_config_ = config::Config::ConfigFromFile(config_file);

    save_mm_ = cvdi_config_->IsSaveMM();
    plot_kml_ = cvdi_config_->IsPlotKML();
    count_pts_ = cvdi_config_->IsCountPoints();

    cvdi_config_->PrintConfig(std::cerr);

    try {
        geo_data::CSVRoadReader road_reader(osm_file);
        road_map_ptr_ = std::make_shared<hmm_mm::RoadMap>();
        road_map_ptr_->construct(road_reader);
    } catch (std::invalid_argument& e) {
        std::stringstream ss;
        ss << "Road map contruction error: " << e.what() << std::endl;
        elogger_->error(ss.str());
    }
}

void CVDIParallel::Init(unsigned n_used_threads) {
    SingleBatchParallel::Init(n_used_threads);
    ilogger_->info("Starting Connected Vehicle DeIdentification with " + std::to_string(n_used_threads) + " threads.");

    if (count_pts_) {
        for (unsigned i = 0; i < n_used_threads; ++i) {
            counters_.emplace_back();
        }
    }
}

void CVDIParallel::Thread(unsigned thread_num, MultiThread::SharedQueue<FileInfo::Ptr>* q) {
    SingleFileInfo::Ptr trip_file_ptr;
    std::string file_path;
    std::ofstream out_file;
    std::string uid;
    geo::Edge::Ptr edge;
    cvdi::AreaFitter::AreaSet explicit_area_set;
    cvdi::AreaFitter::AreaSet implicit_area_set;
    geo_data::Interval::PtrList ta_critical_intervals;
    geo_data::Interval::PtrList stop_critical_intervals;
    geo_data::Interval::PtrList se_intervals;
    geo_data::Interval::PtrList priv_intervals;
    geo_data::Sample::Trace raw_trace;
    geo_data::Sample::Trace trace;
    bool scale_map_fit = cvdi_config_->IsScaleMapFit();
    double map_fit_scale = scale_map_fit ? cvdi_config_->GetMapFitScale() : 1.0;
    double map_fit_extension = cvdi_config_->GetFitExt();
    uint32_t n_heading_groups = cvdi_config_->GetHeadingGroups();
    uint32_t min_edge_trip_points = cvdi_config_->GetMinEdgeTripPoints();
    uint32_t ta_max_q_size = cvdi_config_->GetTAMaxQSize(); 
    double ta_area_wdith = cvdi_config_->GetTAAreaWidth();
    double ta_max_speed = cvdi_config_->GetTAMaxSpeed();
    double ta_heading_delta = cvdi_config_->GetTAHeadingDelta();
    uint64_t stop_max_time = cvdi_config_->GetStopMaxTime();
    double stop_min_distance = cvdi_config_->GetStopMinDistance();
    double stop_max_speed = cvdi_config_->GetStopMaxSpeed();
    double pi_min_dd = cvdi_config_->GetMinDirectDistance();
    double pi_min_md = cvdi_config_->GetMinManhattanDistance();
    uint32_t pi_min_od = cvdi_config_->GetMinOutDegree();
    double pi_max_dd = cvdi_config_->GetMaxDirectDistance();
    double pi_max_md = cvdi_config_->GetMaxManhattanDistance();
    uint32_t pi_max_od = cvdi_config_->GetMaxOutDegree();
    double pi_rand_dd = cvdi_config_->GetRandDirectDistance();
    double pi_rand_md = cvdi_config_->GetRandManhattanDistance();
    double pi_rand_od = cvdi_config_->GetRandOutDegree();
    uint32_t kml_stride = cvdi_config_->GetKMLStride();
    bool kml_suppress_di = cvdi_config_->IsKMLSuppressDI();

    while ((trip_file_ptr = std::dynamic_pointer_cast<SingleFileInfo>(q->pop())) != nullptr) {
        file_path = trip_file_ptr->GetFilePath();

        try {
            raw_trace = geo_data::make_trace(file_path);

            if (raw_trace.empty()) {
                ilogger_->warn("Not de-identifying empty trace: " + file_path);

                continue;
            }

            trace.clear();
            
            // remove error points
            geo_data::remove_trace_errors(raw_trace, trace);

            if (trace.empty()) {
                ilogger_->warn("Not de-identifying empty trace: " + file_path);

                continue;
            }

            uid = trace[0]->id();

            // map matching
            std::chrono::steady_clock::time_point mm_start = std::chrono::steady_clock::now();
            hmm_mm::Matcher matcher(0.0, 0);
            matcher.map_match(*road_map_ptr_, trace);
            std::chrono::steady_clock::time_point mm_end = std::chrono::steady_clock::now();

            std::stringstream ss;
            ss << "Map matching for " << file_path << " took "; 
            ss << std::chrono::duration_cast<std::chrono::seconds>(mm_end - mm_start).count() << " seconds.";
            ilogger_->info(ss.str());

            // area fitting
            cvdi::AreaFitter area_fitter(map_fit_scale, map_fit_extension, n_heading_groups, min_edge_trip_points);
            area_fitter.fit(trace);

            ilogger_->info("Finished area fit for: " + file_path);

            // intersection counting
            cvdi::IntersectionCounter ic;
            ic.count_intersections(trace);

            ilogger_->info("Finished intersection count for: " + file_path);

            // critical intervals
            cvdi::TurnAround tad(ta_max_q_size, ta_area_wdith, ta_max_speed, ta_heading_delta);
            ta_critical_intervals = tad.find_turn_arounds(trace);
            cvdi::Stop stop_detector(stop_max_time, stop_min_distance, stop_max_speed);
            stop_critical_intervals = stop_detector.find_stops(trace);
            cvdi::StartEndIntervals sei;
            se_intervals = sei.get_start_end_intervals(trace);

            ilogger_->info("Finished critical intervals for: " + file_path);

            // merge critical intervals and mark the trace
            geo_data::IntervalMarker ci_marker({ ta_critical_intervals, stop_critical_intervals, se_intervals }, cvdi::kCriticalIntervalType);
            ci_marker.mark_trace(trace);

            ilogger_->info("Finished critical interval marking for: " + file_path);

            // privacy intervals
            cvdi::PrivacyIntervalFinder pif(pi_min_dd,
                                            pi_min_md,
                                            pi_min_od,
                                            pi_max_dd,
                                            pi_max_md,
                                            pi_max_od,
                                            pi_rand_dd,
                                            pi_rand_md,
                                            pi_rand_od);
            priv_intervals = pif.find_intervals( trace );

            ilogger_->info("Finished privacy intervals done: " + file_path);
   
            // merge priv_intervals and mark the trace 
            geo_data::IntervalMarker pi_marker({ priv_intervals }, cvdi::kPrivacyIntervalType);
            pi_marker.mark_trace(trace);

            ilogger_->info("Finished privacy interval marking done: " + file_path);

            // trace is marked with all intervals

            if (out_dir_.empty()) {
                out_file = std::ofstream(uid + ".di.csv");
            } else {
                out_file = std::ofstream(out_dir_ + "/" + uid + ".di.csv");
            }

            out_file << geo_data::kTraceCSVHeader << std::endl;

            for (auto& sample : trace) {
                if (sample->interval() != nullptr && sample->interval()->contains(sample->index())) {
                    continue;
                }
                    
                out_file << sample->record() << std::endl;
            }

            out_file.close();

            ilogger_->info("Finished writing de-identified trace: " + file_path);

            if (count_pts_) {
                cvdi::count_points(raw_trace, counters_[thread_num]);
            }

            if (save_mm_) {
                if (out_dir_.empty()) {
                    out_file = std::ofstream(uid + ".mm");
                } else {
                    out_file = std::ofstream(out_dir_ + "/" + uid + ".mm");
                }

                out_file << "index,osm_way_id,explicit,out_degree" << std::endl;

                for (auto& sample : raw_trace) {
                    out_file << sample->raw_index() << ",";

                    if (sample->matched_edge() != nullptr) {
                        out_file << sample->matched_edge()->road()->osm_id();
                    } 
 
                    out_file << "," << sample->is_explicit_fit() << "," << sample->out_degree() << std::endl;
                }

                out_file.close();
            }

            if (!plot_kml_) {
                continue;
            }

            if (out_dir_.empty()) {
                out_file = std::ofstream(uid + ".kml");
            } else {
                out_file = std::ofstream(out_dir_ + "/" + uid + ".kml");
            }

            explicit_area_set = area_fitter.explicit_area_set;
            implicit_area_set = area_fitter.implicit_area_set;

            geo_data::KML kml(out_file, uid);
            kml.write_poly_style( "explicit_boxes", 0xff990000, 1 );
            kml.write_poly_style( "implicit_boxes", 0xff0033ff, 1 );
            kml.write_line_style( "ci_intervals", 0xffff00ff, 7 );
            kml.write_line_style( "priv_intervals", 0xffffff00, 5 );

            kml.write_trace(trace, kml_suppress_di, kml_stride);

            if (!kml_suppress_di) {
                kml.write_areas(explicit_area_set, "explicit_boxes");
                kml.write_areas(implicit_area_set, "implicit_boxes");

                kml.write_intervals( stop_critical_intervals, trace, "ci_intervals", "stop_marker_style" );
                kml.write_intervals( ta_critical_intervals, trace, "ci_intervals", "turnaround_marker_style" );
                kml.write_intervals( priv_intervals, trace, "priv_intervals" );
            }

            kml.finish();

            out_file.close();

            ilogger_->info("Finished de-identification KML for: " + file_path);
        } catch (std::exception& e) {
            std::stringstream ss;
            ss << "DeIdentification error: " << e.what() << std::endl;
            elogger_->error(ss.str());

            continue;
        }
    }
}

void CVDIParallel::Close(void) {
    SingleBatchParallel::Close();
    ilogger_->info("Finished Connected Vehicle DeIdentification.");

    if (count_pts_) {
        cvdi::PointCounter point_counter;

        for (auto& pc : counters_) {
            point_counter = point_counter + pc;
        }

        std::cerr << cvdi::kPointCountHeader << std::endl;
        std::cerr << point_counter << std::endl;
    }
}
}

int main( int argc, char **argv ) {
    // Set up the tool.
    tool::Tool tool("cvdi_multi", "Batch Connected Vehicle DeIdentification.");
    tool.AddOption(tool::Option('h', "help", "Print this message."));
    tool.AddOption(tool::Option('t', "thread", "The number of threads to use (default: 1 thread).", "1"));
    tool.AddOption(tool::Option('e', "osm_file", "The OSM edge network file (default: osm_network.csv).", "osm_network.csv"));
    tool.AddOption(tool::Option('o', "out_dir", "The output directory (default: working directory).", ""));
    tool.AddOption(tool::Option('c', "config", "The CVDI configuration file. (default: cvdi_config.ini)", "cvdi_config.ini"));

    if (!tool.ParseArgs(std::vector<std::string>{argv + 1, argv + argc})) {
        exit(1);
    }

    unsigned n_threads = 0;
    unsigned n_supported_threads = std::thread::hardware_concurrency();

    try {
        n_threads = static_cast<unsigned>(tool.GetIntVal("thread"));    
    } catch (std::out_of_range&) {
        std::cerr << "Invalid value for \"thread\"!" << std::endl;
        exit(1);
    }

    if (n_threads < 1) {
        std::cerr << "Number of threads must be greater than 1." << std::endl;
        exit(1);
    }
                
    if (n_threads > n_supported_threads + (n_supported_threads / 2)) {
        n_threads = n_supported_threads + (n_supported_threads / 2);    
        std::cerr << "Warning thread number is too high. Using  "  << n_threads << " threads." <<std::endl;
    }
    
    try {
        cvdi_multi::CVDIParallel cvdi_parallel(tool.GetSource(), tool.GetStringVal("osm_file"), tool.GetStringVal("out_dir"), tool.GetStringVal("config"));
        cvdi_parallel.Start(n_threads);
    } catch (std::invalid_argument& e) {    
        std::cerr << e.what() << std::endl; 

        return 1;
    }

    return 0;
}
