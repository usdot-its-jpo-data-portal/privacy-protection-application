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
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <memory>
#include <sstream>

#include "cvdi.hpp"
#include "geo.hpp"
#include "geo_data.hpp"
#include "hmm_mm.hpp"
#include "util.hpp"
#include "tool.hpp"

TEST_CASE( "tool", "command line" ) {
    SECTION("tool") {
        std::stringstream ss;

        tool::Tool tool("test", "test tool.", ss);
        tool.AddOption(tool::Option('h', "help", "Print this message."));
        tool.AddOption(tool::Option('b', "bool", "Boolean option."));
        tool.AddOption(tool::Option('i', "int", "Integer option.", "-1"));
        tool.AddOption(tool::Option('u', "uint", "Long long option.", "100"));
        tool.AddOption(tool::Option('d', "double", "Double option.", "3.145"));

        std::vector<std::string> args1({"test", "-h"});

        CHECK_FALSE(tool.ParseArgs(args1));

        std::vector<std::string> args2({"foo"});
        CHECK(tool.ParseArgs(args2));
        CHECK(tool.GetName() == "test");
        CHECK(tool.GetDesciption() == "test tool.");
        CHECK(tool.GetSource() == "foo");
        CHECK(tool.GetStringVal("int") == "-1");
        CHECK(tool.GetIntVal("int") == -1);
        CHECK(tool.GetUint64Val("uint") == 100);
        CHECK(tool.GetDoubleVal("double") ==  Approx(3.145));
        CHECK_FALSE(tool.GetBoolVal("bool"));

        std::vector<std::string> args3({"-b", "-i", "x", "--double", "y", "--uint", "z", "foo"});
        CHECK(tool.ParseArgs(args3));
        CHECK_THROWS_AS(tool.GetIntVal("bar"), std::out_of_range);
        CHECK_THROWS_AS(tool.GetIntVal("int"), std::invalid_argument);
        CHECK_THROWS_AS(tool.GetUint64Val("uint"), std::invalid_argument);
        CHECK_THROWS_AS(tool.GetDoubleVal("double"), std::invalid_argument);
        CHECK(tool.GetBoolVal("bool"));

        std::vector<std::string> args4({"-bh", "foo"});

        std::vector<std::string> args5({"-q", "foo"});

        std::vector<std::string> args6({"--bar", "foo"});

        std::vector<std::string> args7;

        CHECK_FALSE(tool.ParseArgs(args4));
        CHECK_FALSE(tool.ParseArgs(args5));
        CHECK_FALSE(tool.ParseArgs(args6));
        CHECK_FALSE(tool.ParseArgs(args7));
    }
}

TEST_CASE("DI Algorithm", "[map match][intersection count][critical interval][privacy interval][de-identification]") {
    // map matching, intersection counting, critical intervals
    SECTION("Feature Detection") {
        hmm_mm::RoadMap road_map;
        geo_data::Sample::Trace raw_trace;
        geo_data::Sample::Trace trace;
        geo_data::CSVRoadReader road_reader("/build/test/data/utk.edges");
        road_map.construct(road_reader);
        raw_trace = geo_data::make_trace("/build/test/data/utk_test.csv");
        geo_data::remove_trace_errors(raw_trace, trace);
        hmm_mm::Matcher matcher(0.0, 0);
        hmm_mm::MatchedResults mm_results;
        matcher.map_match(road_map, trace);

        cvdi::AreaFitter area_fitter(1.0, .5, 36, 10);
        area_fitter.fit(trace);

        for (uint64_t i = 0; i < trace.size(); ++i) {
            if (i < 35 || (i > 57 && i < 98) || (i > 143 && i < 277)) {
                CHECK(trace[i]->is_explicit_fit());
            } else {
                CHECK_FALSE(trace[i]->is_explicit_fit());
            }
        }

        cvdi::IntersectionCounter ic;
        ic.count_intersections(trace);

        cvdi::StartEndIntervals sei;
        geo_data::Interval::PtrList se_intervals = sei.get_start_end_intervals(trace);

        CHECK(se_intervals.size() == 2);
        CHECK(se_intervals[0]->left() == 0);
        CHECK(se_intervals[0]->right() == 1);
        CHECK(se_intervals[0]->aux_str() == "start_pt");
        CHECK(se_intervals[1]->left() == trace.size() - 1);
        CHECK(se_intervals[1]->right() == trace.size());
        CHECK(se_intervals[1]->aux_str() == "end_pt");

        // Set the speed becuase of how the data was generated.
        cvdi::TurnAround tad{20, 30.0, 100.0, 90.0};
        geo_data::Interval::PtrList ta_critical_intervals = tad.find_turn_arounds(trace);

        CHECK(ta_critical_intervals.size() == 2);
        CHECK(ta_critical_intervals[0]->left() == 34);
        CHECK(ta_critical_intervals[0]->right() == 58);
        CHECK(ta_critical_intervals[0]->aux_str() == "ta_fit");
        CHECK(ta_critical_intervals[1]->left() == 109);
        CHECK(ta_critical_intervals[1]->right() == 129);
        CHECK(ta_critical_intervals[1]->aux_str() == "ta");

        // Make the time smaller to handle the test data.
        cvdi::Stop stop_detector{1, 50.0, 2.5};
        geo_data::Interval::PtrList stop_critical_intervals = stop_detector.find_stops(trace);

        CHECK(stop_critical_intervals.size() == 2);
        CHECK(stop_critical_intervals[0]->left() == 0);
        CHECK(stop_critical_intervals[0]->right() == 10);
        CHECK(stop_critical_intervals[0]->aux_str() == "stop");
        CHECK(stop_critical_intervals[1]->left() == 177);
        CHECK(stop_critical_intervals[1]->right() == 187);
        CHECK(stop_critical_intervals[1]->aux_str() == "stop");

        geo_data::IntervalMarker im( { ta_critical_intervals, stop_critical_intervals, se_intervals }, cvdi::kCriticalIntervalType );
        im.mark_trace( trace );

        for (auto& sample : trace) {
            std::size_t i = sample->index();

            if (sample->interval() != nullptr) {
                CHECK((i < 10 || (i > 32 && i < 58) || (i > 106 && i < 130) || ( i > 176 && i < 187) || i == 280));
            }
        }
    }

    SECTION("Out Degree Max") {
        hmm_mm::RoadMap road_map;
        geo_data::Sample::Trace raw_trace;
        geo_data::Sample::Trace trace;
        geo_data::CSVRoadReader road_reader("/build/test/data/utk.edges");
        road_map.construct(road_reader);
        raw_trace = geo_data::make_trace("/build/test/data/utk_test.csv");
        geo_data::remove_trace_errors(raw_trace, trace);
        hmm_mm::Matcher matcher(0.0, 0);
        hmm_mm::MatchedResults mm_results;
        matcher.map_match(road_map, trace);

        cvdi::AreaFitter area_fitter(1.0, .5, 36, 10);
        area_fitter.fit(trace);

        cvdi::IntersectionCounter ic;
        ic.count_intersections(trace);

        cvdi::StartEndIntervals sei;
        geo_data::Interval::PtrList se_intervals = sei.get_start_end_intervals(trace);

        // Set the speed becuase of how the data was generated.
        cvdi::TurnAround tad{20, 30.0, 100.0, 90.0};
        geo_data::Interval::PtrList ta_critical_intervals = tad.find_turn_arounds(trace);

        geo_data::IntervalMarker im( { ta_critical_intervals, se_intervals }, cvdi::kCriticalIntervalType );
        im.mark_trace( trace );
        
        // First check max out-degree coverage for the two intersections.
        cvdi::PrivacyIntervalFinder pif(10000.0, 
                                        10000.0, 
                                        1, 
                                        11000.0, 
                                        11000.0, 
                                        2, 
                                        0.0,
                                        0.0, 
                                        0.0);

        geo_data::Interval::PtrList priv_intervals = pif.find_intervals(trace);

        CHECK(priv_intervals[0]->left() == 1);
        CHECK(priv_intervals[0]->right() == 34);
        CHECK(priv_intervals[0]->aux_str() == "forward:ci");
        CHECK(priv_intervals[1]->left() == 58);
        CHECK(priv_intervals[1]->right() == 109);
        CHECK(priv_intervals[1]->aux_str() == "forward:ci");
        CHECK(priv_intervals[2]->left() == 129);
        CHECK(priv_intervals[2]->right() == 168);
        CHECK(priv_intervals[2]->aux_str() == "forward:max_out_degree::122.083::217.72::2");
        CHECK(priv_intervals[3]->left() == 168);
        CHECK(priv_intervals[3]->right() == 280);
        CHECK(priv_intervals[3]->aux_str() == "backward:pi");
    }

    SECTION("Direct Distance Max") {
        hmm_mm::RoadMap road_map;
        geo_data::Sample::Trace raw_trace;
        geo_data::Sample::Trace trace;
        geo_data::CSVRoadReader road_reader("/build/test/data/utk.edges");
        road_map.construct(road_reader);
        raw_trace = geo_data::make_trace("/build/test/data/utk_test.csv");
        geo_data::remove_trace_errors(raw_trace, trace);
        hmm_mm::Matcher matcher(0.0, 0);
        hmm_mm::MatchedResults mm_results;
        matcher.map_match(road_map, trace);

        cvdi::AreaFitter area_fitter(1.0, .5, 36, 10);
        area_fitter.fit(trace);

        cvdi::IntersectionCounter ic;
        ic.count_intersections(trace);

        cvdi::StartEndIntervals sei;
        geo_data::Interval::PtrList se_intervals = sei.get_start_end_intervals(trace);

        // Set the speed becuase of how the data was generated.
        cvdi::TurnAround tad{20, 30.0, 100.0, 90.0};
        geo_data::Interval::PtrList ta_critical_intervals = tad.find_turn_arounds(trace);

        geo_data::IntervalMarker im( { ta_critical_intervals, se_intervals }, cvdi::kCriticalIntervalType );
        im.mark_trace( trace );
    
        // Check max direct distance.
        cvdi::PrivacyIntervalFinder pif(50.0, 
                                        10000.0, 
                                        10, 
                                        100.0, 
                                        11000.0, 
                                        11, 
                                        0.0,
                                        0.0, 
                                        0.0);

        geo_data::Interval::PtrList priv_intervals = pif.find_intervals(trace);


        CHECK(priv_intervals[0]->left() == 1);
        CHECK(priv_intervals[0]->right() == 34);
        CHECK(priv_intervals[0]->aux_str() == "forward:ci");
        CHECK(priv_intervals[1]->left() == 58);
        CHECK(priv_intervals[1]->right() == 95);
        CHECK(priv_intervals[1]->aux_str() == "forward:max_dist::108.397::263.964::0");
        CHECK(priv_intervals[2]->left() == 95);
        CHECK(priv_intervals[2]->right() == 109);
        CHECK(priv_intervals[2]->aux_str() == "backward:pi");
        CHECK(priv_intervals[3]->left() == 129);
        CHECK(priv_intervals[3]->right() == 162);
        CHECK(priv_intervals[3]->aux_str() == "forward:max_dist::122.083::217.72::2");
        CHECK(priv_intervals[4]->left() == 249);
        CHECK(priv_intervals[4]->right() == 280);
        CHECK(priv_intervals[4]->aux_str() == "backward:max_dist::315.337::9.86011::2");
    }

    SECTION("Manhattan Distance Max") {
        hmm_mm::RoadMap road_map;
        geo_data::Sample::Trace raw_trace;
        geo_data::Sample::Trace trace;
        geo_data::CSVRoadReader road_reader("/build/test/data/utk.edges");
        road_map.construct(road_reader);
        raw_trace = geo_data::make_trace("/build/test/data/utk_test.csv");
        geo_data::remove_trace_errors(raw_trace, trace);
        hmm_mm::Matcher matcher(0.0, 0);
        hmm_mm::MatchedResults mm_results;
        matcher.map_match(road_map, trace);

        cvdi::AreaFitter area_fitter(1.0, .5, 36, 10);
        area_fitter.fit(trace);

        cvdi::IntersectionCounter ic;
        ic.count_intersections(trace);

        cvdi::StartEndIntervals sei;
        geo_data::Interval::PtrList se_intervals = sei.get_start_end_intervals(trace);

        // Set the speed becuase of how the data was generated.
        cvdi::TurnAround tad{20, 30.0, 100.0, 90.0};
        geo_data::Interval::PtrList ta_critical_intervals = tad.find_turn_arounds(trace);

        geo_data::IntervalMarker im( { ta_critical_intervals, se_intervals }, cvdi::kCriticalIntervalType );
        im.mark_trace( trace );
    
        // First check max Manhattan distance coverage.
        cvdi::PrivacyIntervalFinder pif(10000.0, 
                                        100.0, 
                                        10, 
                                        11000.0, 
                                        150.0, 
                                        11, 
                                        0.0,
                                        0.0, 
                                        0.0);

        geo_data::Interval::PtrList priv_intervals = pif.find_intervals(trace);

        CHECK(priv_intervals[0]->left() == 1);
        CHECK(priv_intervals[0]->right() == 34);
        CHECK(priv_intervals[0]->aux_str() == "forward:ci");
        CHECK(priv_intervals[1]->left() == 58);
        CHECK(priv_intervals[1]->right() == 89);
        CHECK(priv_intervals[1]->aux_str() == "forward:max_dist::92.4295::235.383::0");
        CHECK(priv_intervals[2]->left() == 89);
        CHECK(priv_intervals[2]->right() == 109);
        CHECK(priv_intervals[2]->aux_str() == "backward:pi");
        CHECK(priv_intervals[3]->left() == 129);
        CHECK(priv_intervals[3]->right() == 168);
        CHECK(priv_intervals[3]->aux_str() == "forward:max_dist::122.083::217.72::2");
        CHECK(priv_intervals[4]->left() == 168);
        CHECK(priv_intervals[4]->right() == 280);
        CHECK(priv_intervals[4]->aux_str() == "backward:pi");
    }

    SECTION("Min Metrics") {
        hmm_mm::RoadMap road_map;
        geo_data::Sample::Trace raw_trace;
        geo_data::Sample::Trace trace;
        geo_data::CSVRoadReader road_reader("/build/test/data/utk.edges");
        road_map.construct(road_reader);
        raw_trace = geo_data::make_trace("/build/test/data/utk_test.csv");
        geo_data::remove_trace_errors(raw_trace, trace);
        hmm_mm::Matcher matcher(0.0, 0);
        hmm_mm::MatchedResults mm_results;
        matcher.map_match(road_map, trace);

        cvdi::AreaFitter area_fitter(1.0, .5, 36, 10);
        area_fitter.fit(trace);

        cvdi::IntersectionCounter ic;
        ic.count_intersections(trace);

        cvdi::StartEndIntervals sei;
        geo_data::Interval::PtrList se_intervals = sei.get_start_end_intervals(trace);

        // Set the speed becuase of how the data was generated.
        cvdi::TurnAround tad{20, 30.0, 100.0, 90.0};
        geo_data::Interval::PtrList ta_critical_intervals = tad.find_turn_arounds(trace);

        geo_data::IntervalMarker im( { ta_critical_intervals, se_intervals }, cvdi::kCriticalIntervalType );
        im.mark_trace( trace );
        
        cvdi::PrivacyIntervalFinder pif(100.0, 
                                        200.0, 
                                        2, 
                                        11000.0, 
                                        11000.0, 
                                        10, 
                                        0.0,
                                        0.0, 
                                        0.0);

        geo_data::Interval::PtrList priv_intervals = pif.find_intervals(trace);

        CHECK(priv_intervals[0]->left() == 1);
        CHECK(priv_intervals[0]->right() == 34);
        CHECK(priv_intervals[0]->aux_str() == "forward:ci");
        CHECK(priv_intervals[1]->left() == 58);
        CHECK(priv_intervals[1]->right() == 109);
        CHECK(priv_intervals[1]->aux_str() == "forward:ci");
        CHECK(priv_intervals[2]->left() == 129);
        CHECK(priv_intervals[2]->right() == 168);
        CHECK(priv_intervals[2]->aux_str() == "forward:min::122.083::217.72::2");
        CHECK(priv_intervals[3]->left() == 168);
        CHECK(priv_intervals[3]->right() == 280);
        CHECK(priv_intervals[3]->aux_str() == "backward:pi");
    }

    SECTION("DI") {
        hmm_mm::RoadMap road_map;
        geo_data::Sample::Trace raw_trace;
        geo_data::Sample::Trace trace;
        geo_data::CSVRoadReader road_reader("/build/test/data/utk.edges");
        road_map.construct(road_reader);
        raw_trace = geo_data::make_trace("/build/test/data/utk_test.csv");
        geo_data::remove_trace_errors(raw_trace, trace);
        hmm_mm::Matcher matcher(0.0, 0);
        hmm_mm::MatchedResults mm_results;
        matcher.map_match(road_map, trace);

        cvdi::AreaFitter area_fitter(1.0, .5, 36, 10);
        area_fitter.fit(trace);

        cvdi::IntersectionCounter ic;
        ic.count_intersections(trace);

        cvdi::StartEndIntervals sei;
        geo_data::Interval::PtrList se_intervals = sei.get_start_end_intervals(trace);

        geo_data::IntervalMarker im( { se_intervals }, cvdi::kCriticalIntervalType );
        im.mark_trace( trace );
        
        cvdi::PrivacyIntervalFinder pif(10.0, 
                                        10.0, 
                                        0, 
                                        11000.0, 
                                        11000.0, 
                                        10, 
                                        0.0,
                                        0.0, 
                                        0.0);

        geo_data::Interval::PtrList priv_intervals = pif.find_intervals(trace);
        geo_data::IntervalMarker pi_marker({ priv_intervals }, cvdi::kPrivacyIntervalType);
        pi_marker.mark_trace(trace);
                
        uint64_t n_kept_samples = 0;

        for (auto& sample : trace) {
            if (sample->interval() != nullptr && sample->interval()->contains(sample->index())) {
                std::size_t index = sample->index();
                CHECK((index < 35 || index > 142));
            } else {
                n_kept_samples++;
            }
        }

        CHECK(n_kept_samples == 108);
    }

    SECTION("Point Count") {
        hmm_mm::RoadMap road_map;
        geo_data::Sample::Trace raw_trace;
        geo_data::Sample::Trace trace;
        geo_data::CSVRoadReader road_reader("/build/test/data/utk.edges");
        road_map.construct(road_reader);

        raw_trace = geo_data::make_trace("/build/test/data/utk_test.csv");
        geo_data::remove_trace_errors(raw_trace, trace);
        hmm_mm::Matcher matcher_1(0.0, 0);
        matcher_1.map_match(road_map, trace);

        cvdi::AreaFitter area_fitter_1(1.0, .5, 36, 10);
        area_fitter_1.fit(trace);

        cvdi::IntersectionCounter ic_1;
        ic_1.count_intersections(trace);

        cvdi::StartEndIntervals sei_1;
        geo_data::Interval::PtrList se_intervals = sei_1.get_start_end_intervals(trace);

        cvdi::Stop stop_detector{1, 50.0, 2.5};
        geo_data::Interval::PtrList stop_critical_intervals = stop_detector.find_stops(trace);

        geo_data::IntervalMarker im_1( { se_intervals, stop_critical_intervals }, cvdi::kCriticalIntervalType );
        im_1.mark_trace( trace );
        
        cvdi::PrivacyIntervalFinder pif_1(10.0, 
                                          10.0, 
                                          0, 
                                          11000.0, 
                                          11000.0, 
                                          10, 
                                          0.0,
                                          0.0, 
                                          0.0);

        geo_data::Interval::PtrList priv_intervals = pif_1.find_intervals(trace);
        geo_data::IntervalMarker pi_marker_1({ priv_intervals }, cvdi::kPrivacyIntervalType);
        pi_marker_1.mark_trace(trace);

        cvdi::PointCounter pc_1;
        cvdi::count_points(raw_trace, pc_1);
        CHECK(pc_1.n_points == 281);
        CHECK(pc_1.n_invalid_field_points == 0);
        CHECK(pc_1.n_invalid_geo_points == 0);
        CHECK(pc_1.n_invalid_heading_points == 0);
        CHECK(pc_1.n_ci_points == 21);
        CHECK(pc_1.n_pi_points == 128);

        raw_trace.clear();
        trace.clear();
        raw_trace = geo_data::make_trace("/build/test/data/utk_err_test.csv");
        geo_data::remove_trace_errors(raw_trace, trace);
        hmm_mm::Matcher matcher_2(0.0, 0);
        matcher_1.map_match(road_map, trace);

        cvdi::AreaFitter area_fitter_2(1.0, .5, 36, 10);
        area_fitter_2.fit(trace);

        cvdi::IntersectionCounter ic_2;
        ic_2.count_intersections(trace);

        cvdi::StartEndIntervals sei_2;
        se_intervals.clear();
        se_intervals = sei_2.get_start_end_intervals(trace);

        geo_data::IntervalMarker im_2( { se_intervals }, cvdi::kCriticalIntervalType );
        im_2.mark_trace( trace );
        
        cvdi::PrivacyIntervalFinder pif_2(10.0, 
                                          10.0, 
                                          0, 
                                          11000.0, 
                                          11000.0, 
                                          10, 
                                          0.0,
                                          0.0, 
                                          0.0);

        priv_intervals.clear();
        priv_intervals = pif_2.find_intervals(trace);
        geo_data::IntervalMarker pi_marker_2({ priv_intervals }, cvdi::kPrivacyIntervalType);
        pi_marker_2.mark_trace(trace);

        cvdi::PointCounter pc_2;
        cvdi::count_points(raw_trace, pc_2);
        CHECK(pc_2.n_points == 140);
        CHECK(pc_2.n_invalid_field_points == 2);
        CHECK(pc_2.n_invalid_geo_points == 3);
        CHECK(pc_2.n_invalid_heading_points == 1);
        CHECK(pc_2.n_ci_points == 2);
        CHECK(pc_2.n_pi_points == 131);

        cvdi::PointCounter pc_3 = pc_1 + pc_2;
        CHECK(pc_3.n_points == 421);
        CHECK(pc_3.n_invalid_field_points == 2);
        CHECK(pc_3.n_invalid_geo_points == 3);
        CHECK(pc_3.n_invalid_heading_points == 1);
        CHECK(pc_3.n_ci_points == 23);
        CHECK(pc_3.n_pi_points == 259);
    }
}
