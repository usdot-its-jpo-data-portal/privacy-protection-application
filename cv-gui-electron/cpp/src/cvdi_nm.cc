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
#include <nan.h>

#include <sstream>
#include <string>
#include <time.h>
#include <vector>
#include <uv.h>

#include "cvdi.hpp"
#include "multi_thread.hpp"

#ifndef _WIN32
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif

using std::string;

using Nan::AsyncWorker;  
using Nan::Callback;  
using Nan::GetFunction;  
using Nan::HandleScope;  
using Nan::New;  
using Nan::Null;  
using Nan::Set;

using v8::Function;  
using v8::FunctionTemplate;  
using v8::Local;  
using v8::Number;  
using v8::String;  
using v8::Value;
using v8::Object;

CSVFactory::CSVFactory( std::istream& stream, const std::string& header, const std::string& lat_field, const std::string& lon_field, const std::string& heading_field, const std::string& speed_field, const std::string& gentime_field) :
        RX_DEVICE{ 0 },
        FILE_ID{ 1 },
        TX_DEV{ 2 },
        GENTIME{ 3 },
        TX_RANDOM{ 4 },
        MSG_COUNT{ 5 },
        DSECONDS{ 6 },
        LAT{ 7 },
        LON{ 8 },
        ELEVATION{ 9 },
        SPEED{10},
        HEADING{11},
        AX{ 12 },
        AY{ 13 },
        AZ{ 14 },
        YAW_RATE{ 15 },
        PATH_COUNT{ 16 },
        RADIUS_OF_CURVE{ 18 },
        CONFIDENCE{ 19 },
        stream_{ stream },
        header{ header },
        lat_field{ lat_field },
        lon_field{ lon_field },
        heading_field{ heading_field },
        speed_field{ speed_field },
        gentime_field{ gentime_field },
        index{ 0 }
    {
        map_index_fields();
    }

/**
 *
 * \throws out_of_range and invalid_argument when the fileline cannot be parsed.
 */
trajectory::Point::Ptr CSVFactory::make_point( const std::string& fileline )
{
    StrVector parts = string_utilities::split( fileline, ',' );

    double lat = std::stod( parts[LAT] );

    if (lat > 80.0 || lat < -84.0) {
        throw std::out_of_range{ "bad latitude: " + std::to_string(lat) };
    }

    double lon = std::stod( parts[LON] );

    if (lon >= 180.0 || lon <= -180.0) {
        throw std::out_of_range{"bad longitude: " + std::to_string(lon) };
    }

    if (lat == 0.0 && lon == 0.0) 
    {
        throw std::out_of_range{"equator: " + std::to_string(lat) + "  " + std::to_string(lon) };
    }

    double heading = std::stod( parts[HEADING] );

    if (heading > 360.0 || heading < 0.0) {
        throw std::out_of_range{"bad heading: " + std::to_string(heading)};
    }

    double speed = std::stod( parts[SPEED] );
    uint64_t gentime = std::stoull( parts[GENTIME] );

    return std::make_shared<trajectory::Point>( fileline, gentime, lat, lon, heading, speed, index++ );
}

void CSVFactory::make_trajectory(trajectory::Trajectory& traj) 
{
    std::string line;

    while ( std::getline( stream_, line ) ) {
        try {
            traj.push_back( make_point( line ) );
        } catch ( std::exception& ) {
            // nothing.
        }
    }
}

void CSVFactory::make_trajectory( trajectory::Trajectory& traj, uint64_t start, uint64_t end )
{
    stream_.seekg( start ); 
    std::string line;

    while ( static_cast<uint64_t>(stream_.tellg()) < end && std::getline( stream_, line ) ) {
        try {
            traj.push_back( make_point( line ) );
        } catch ( std::exception& ) {
            // nothing.
        }
    }
}

void CSVFactory::write_trajectory( std::ostream& stream, const std::string& header, const trajectory::Trajectory& traj ) 
{
    stream << header << std::endl;

    for(auto& tp : traj)
    {
        stream << tp->get_data() << std::endl;    
    } 
}

const std::string CSVFactory::GetHeader() const {
    return header;
}

void CSVFactory::map_index_fields()
{
    StrVector parts = string_utilities::split( header, ',' );
    bool lat_found = false;
    bool lon_found = false;
    bool heading_found = false;
    bool speed_found = false;
    bool gentime_found = false;

    int i = 0;

    for (auto& part : parts) 
    {
        if (part == lat_field) 
        {
            lat_found = true;
            LAT = i;
        } 
        else if (part == lon_field) 
        {
            lon_found = true;
            LON = i;
        }
        else if (part == heading_field) 
        {
            heading_found = true;
            HEADING = i;
        }
        else if (part == speed_field) 
        {
            speed_found = true;
            SPEED = i;
        }
        else if (part == gentime_field) 
        {
            gentime_found = true;
            GENTIME = i;
        }

        i++;
    }

    if (!lat_found) 
    {
        throw std::invalid_argument{ "latitude string: " + lat_field + " not found!"};
    }
    if (!lon_found) 
    {
        throw std::invalid_argument{ "longitude string: " + lon_field + " not found!"};
    }
    if (!heading_found) 
    {
        throw std::invalid_argument{ "heading string: " + heading_field + " not found!"};
    }
    if (!speed_found) 
    {
        throw std::invalid_argument{ "speed string: " + speed_field + " not found!"};
    }
    if (!gentime_found) 
    {
        throw std::invalid_argument{ "gentime string: " + gentime_field + " not found!"};
    }
}

CSVSplitter::CSVSplitter( std::istream& stream, const std::vector<int> uid_indices, uint64_t size, bool has_header, char delimiter ) :
    stream_{ stream },
    uid_indices{ uid_indices },
    size{ size },
    delimiter{ delimiter },
    start{ 0 },
    end{ 0 }
{
    if (has_header)
    {
        std::getline( stream_, header );
    }

    start = stream_.tellg();
    std::getline( stream_, curr_line );
    uid = get_curr_uid(); 
}
    
CSVSplitter::CSVSplitter( std::istream& stream, uint64_t size, const std::string& uid_fields, bool has_header, char delimiter) :
    stream_{ stream },
    size{ size },
    uid_fields{ uid_fields },
    delimiter{ delimiter },
    start{ 0 },
    end{ 0 }
{
    if (std::getline( stream_, header ))
    {
        map_index_fields();   
    } 

    start = stream_.tellg();
    std::getline( stream_, curr_line );
    uid = get_curr_uid(); 
}

CSVSplitter::CSVSplitter( std::istream& stream, uint64_t size, char delimiter) :
    stream_{ stream },
    size{ size },
    uid_fields{ "RxDevice,FileId" },
    delimiter{ delimiter },
    start{ 0 },
    end{ 0 }
{
    if (std::getline( stream_, header ))
    {
        map_index_fields();   
    } 

    start = stream_.tellg();
    std::getline( stream_, curr_line );
    uid = get_curr_uid(); 
}

CSVSplitter::CSVSplitter( std::istream& stream, uint64_t size, const std::string& header, const std::string& uid_fields, char delimiter) :
    stream_{ stream },
    size{ size },
    header{ header },
    uid_fields{ uid_fields },
    delimiter{ delimiter },
    start{ 0 },
    end{ 0 }
{
    map_index_fields();   
    std::getline( stream_, curr_line );
    uid = get_curr_uid(); 
}

CSVSplitter::CSVSplitter( std::istream& stream, uint64_t size, const std::string& header, char delimiter ) :
    stream_{ stream },
    size{ size },
    header{ header },
    uid_fields{ "RxDevice,FileId" },
    delimiter{ delimiter },
    start{ 0 },
    end{ 0 }
{
    map_index_fields();   
    std::getline( stream_, curr_line );
    uid = get_curr_uid(); 
}

const CSVSplitter::TripLocationPtr CSVSplitter::next_trajectory( )
{
    if (end >= size)
    {
        return nullptr;
    }

    // Find the index within the stream of the next trip.
    find_next_trip();

    TripLocationPtr ret = std::make_shared<std::tuple<std::string, uint64_t, uint64_t>>(uid, start, end);
    start = end;
    uid = curr_uid;

    return ret;
}

const std::string& CSVSplitter::get_header() const
{
    return header;
}

const std::string CSVSplitter::get_uid(const std::string& header, const std::string& line, const std::string& uid_fields, char delimiter) {
    std::vector<int> uid_indices;
    StrVector uid_parts = string_utilities::split( uid_fields, delimiter );
    StrVector parts = string_utilities::split( header, delimiter );
    bool found = false;
    int part_index = 0;

    for (auto& field : uid_parts) 
    {
        found = false;

        // Search the remaining parts.
        for (; part_index < static_cast<int>(parts.size()); part_index++) 
        {
            if (parts[part_index] == field)
            {
                uid_indices.push_back(part_index);
                found = true;
                break;
            }
        }

        if (!found) 
        {
            throw std::invalid_argument{ "Could not find header field: " + field };
        }
    }

    parts = string_utilities::split( line, delimiter );

    std::string ret = "";
    bool first_part = true;
    
    for (auto i : uid_indices)
    {
        if (!first_part)
        {
            ret += "_";
        }

        ret += parts[i];
        first_part = false;
    }    

    return ret;
}

const std::string CSVSplitter::get_curr_uid() const
{
    StrVector parts = string_utilities::split( curr_line, delimiter );

    std::string ret = "";
    bool first_part = true;
    
    for (auto i : uid_indices)
    {
        if (!first_part)
        {
            ret += "_";
        }

        ret += parts[i];
        first_part = false;
    }    

    return ret;
}

void CSVSplitter::find_next_trip()
{
    uint64_t next_end;

    // Find the next trip by doing a linear line-by-line search on the file.
    while ( std::getline( stream_, curr_line) )
    {
        curr_uid = get_curr_uid();
        next_end = stream_.tellg();

        if (curr_uid != uid)
        {
            return;
        }

        end = next_end;
    }

    end = size;
}

void CSVSplitter::map_index_fields()
{
    uid_indices.clear();
    StrVector uid_parts = string_utilities::split( uid_fields, delimiter );
    StrVector parts = string_utilities::split( header, delimiter );
    bool found = false;
    int part_index = 0;

    for (auto& field : uid_parts) 
    {
        found = false;

        // Search remaining parts.
        for (; part_index < static_cast<int>(parts.size()); part_index++) 
        {
            if (parts[part_index] == field)
            {
                uid_indices.push_back(part_index);
                found = true;

                break;
            }
        }

        if (!found) 
        {
            throw std::invalid_argument{ "Could not find header field: " + field };
        }
    }
}

// DIConfig
DIConfig::DIConfig() {}

void DIConfig::SetLatField(const std::string& lat_field) {
    lat_field_ = lat_field;
}

void DIConfig::SetLonField(const std::string& lon_field) {
   lon_field_ = lon_field;
}

void DIConfig::SetHeadingField(const std::string& heading_field) {
    heading_field_ = heading_field;
}

void DIConfig::SetSpeedField(const std::string& speed_field) {
    speed_field_ = speed_field;
}

void DIConfig::SetGentimeField(const std::string& gentime_field) {
    gentime_field_ = gentime_field;
}

void DIConfig::SetUIDFields(const std::string& uid_fields) {
    uid_fields_ = uid_fields;
}

void DIConfig::SetFitExt(double fit_ext) {
    fit_ext_ = fit_ext;
}

void DIConfig::ToggleScaleMapFit(bool scale_map_fit) {
    scale_map_fit_ = scale_map_fit;
}

void DIConfig::SetMapFitScale(double map_fit_scale) {
    map_fit_scale_ = map_fit_scale;
}

void DIConfig::SetHeadingGroups(uint32_t n_heading_groups) {
    n_heading_groups_ = n_heading_groups;
}

void DIConfig::SetMinEdgeTripPoints(uint32_t min_edge_trip_points) {
    min_edge_trip_points_ = min_edge_trip_points;
}

void DIConfig::SetTAMaxQSize(uint32_t ta_max_q_size) {
    ta_max_q_size_ = ta_max_q_size;
}

void DIConfig::SetTAAreaWidth(double ta_area_width) {
    ta_area_width_ = ta_area_width;
}

void DIConfig::SetTAMaxSpeed(double ta_max_speed) {
    ta_max_speed_ = ta_max_speed;
}

void DIConfig::SetTAHeadingDelta(double ta_heading_delta) {
    ta_heading_delta_ = ta_heading_delta;
}

void DIConfig::SetStopMaxTime(double stop_max_time) {
    stop_max_time_ = stop_max_time;
}

void DIConfig::SetStopMinDistance(double stop_min_distance) {
    stop_min_distance_ = stop_min_distance;
}

void DIConfig::SetStopMaxSpeed(double stop_max_speed) {
    stop_max_speed_ = stop_max_speed;
}

void DIConfig::SetMinDirectDistance(double min_direct_distance) {
    min_direct_distance_ = min_direct_distance;
}

void DIConfig::SetMinManhattanDistance(double min_manhattan_distance) {
    min_manhattan_distance_ = min_manhattan_distance;
}

void DIConfig::SetMinOutDegree(uint32_t min_out_degree) {
    min_out_degree_ = min_out_degree;
}

void DIConfig::SetMaxDirectDistance(double max_direct_distance) {
    max_direct_distance_ = max_direct_distance;
}

void DIConfig::SetMaxManhattanDistance(double max_manhattan_distance) {
    max_manhattan_distance_ = max_manhattan_distance;
}

void DIConfig::SetMaxOutDegree(uint32_t max_out_degree) {
    max_out_degree_ = max_out_degree;
}

void DIConfig::SetRandDirectDistance(double rand_direct_distance) {
    rand_direct_distance_ = rand_direct_distance;
}

void DIConfig::SetRandManhattanDistance(double rand_manhattan_distance) {
    rand_manhattan_distance_ = rand_manhattan_distance;
}

void DIConfig::SetRandOutDegree(double rand_out_degree) {
    rand_out_degree_ = rand_out_degree;
}

void DIConfig::SetQuadSWLat(double quad_sw_lat) {
    quad_sw_lat_ = quad_sw_lat;
}

void DIConfig::SetQuadSWLon(double quad_sw_lon) {
    quad_sw_lon_ = quad_sw_lon;
}

void DIConfig::SetQuadNELat(double quad_ne_lat) {
    quad_ne_lat_ = quad_ne_lat;
}

void DIConfig::SetQuadNELon(double quad_ne_lon) {
    quad_ne_lon_ = quad_ne_lon;
}

void DIConfig::TogglePlotKML(bool plot_kml) {
    plot_kml_ = plot_kml;
}

bool DIConfig::IsPlotKML(void) const {
    return plot_kml_;
}

const std::string& DIConfig::GetLatField(void) const {
    return lat_field_;
}

const std::string& DIConfig::GetLonField(void) const {
    return lon_field_;
}

const std::string& DIConfig::GetSpeedField(void) const {
    return speed_field_;
}

const std::string& DIConfig::GetHeadingField(void) const {
    return heading_field_;
}

const std::string& DIConfig::GetGentimeField(void) const {
    return gentime_field_;
}

const std::string& DIConfig::GetUIDFields(void) const {
    return uid_fields_;
}

double DIConfig::GetFitExt(void) const {
    return fit_ext_;
}

bool DIConfig::IsScaleMapFit(void) const {
    return scale_map_fit_;
}

double DIConfig::GetMapFitScale(void) const {
    return map_fit_scale_;
}

uint32_t DIConfig::GetHeadingGroups(void) const {
    return n_heading_groups_;
}

uint32_t DIConfig::GetMinEdgeTripPoints(void) const {
    return min_edge_trip_points_;
}

uint32_t DIConfig::GetTAMaxQSize(void) const {
    return ta_max_q_size_;
}

double DIConfig::GetTAAreaWidth(void) const {
    return ta_area_width_;
}

double DIConfig::GetTAMaxSpeed(void) const {
    return ta_max_speed_;
}

double DIConfig::GetTAHeadingDelta(void) const {
    return ta_heading_delta_;
}

double DIConfig::GetStopMaxTime(void) const {
    return stop_max_time_;
}

double DIConfig::GetStopMinDistance(void) const {
    return stop_min_distance_;
}

double DIConfig::GetStopMaxSpeed(void) const {
    return stop_max_speed_;
}

double DIConfig::GetMinDirectDistance(void) const {
    return min_direct_distance_;
}

double DIConfig::GetMinManhattanDistance(void) const {
    return min_manhattan_distance_;
}

uint32_t DIConfig::GetMinOutDegree(void) const {
    return min_out_degree_;
}

double DIConfig::GetMaxDirectDistance(void) const {
    return max_direct_distance_;
}

double DIConfig::GetMaxManhattanDistance(void) const {
    return max_manhattan_distance_;
}

uint32_t DIConfig::GetMaxOutDegree(void) const {
    return max_out_degree_;
}

double DIConfig::GetRandDirectDistance(void) const {
    return rand_direct_distance_;
}

double DIConfig::GetRandManhattanDistance(void) const {
    return rand_manhattan_distance_;
}

double DIConfig::GetRandOutDegree(void) const {
    return rand_out_degree_;
}

double DIConfig::GetQuadSWLat(void) const {
    return quad_sw_lat_;
}

double DIConfig::GetQuadSWLon(void) const {
    return quad_sw_lon_;
}

double DIConfig::GetQuadNELat(void) const {
    return quad_ne_lat_;
}

double DIConfig::GetQuadNELon(void) const {
    return quad_ne_lon_;
}

void DIConfig::PrintConfig(std::ostream& stream) const {
    stream << "Plot KML: " << plot_kml_ << std::endl; 
    stream << "Fit extension: " << fit_ext_  << std::endl;
    stream << "Scale map fit: " << scale_map_fit_  << std::endl;
    stream << "N Heading groups: " << n_heading_groups_ << std::endl;
    stream << "Min edge trip points: " << min_edge_trip_points_ << std::endl;
    stream << "TA max queue size: " << ta_max_q_size_ << std::endl;
    stream << "TA area width: " << ta_area_width_ << std::endl;
    stream << "TA heading delta: " << ta_heading_delta_ << std::endl;
    stream << "Stop max time: " << stop_max_speed_ << std::endl;
    stream << "Stop min distance: " << stop_min_distance_ << std::endl;
    stream << "Stop max speed: " << stop_max_speed_ << std::endl;
    stream << "Min direct distance: " << min_direct_distance_ << std::endl;
    stream << "Min manhattan distance: " << min_manhattan_distance_ << std::endl;
    stream << "Min out degree: " << min_out_degree_ << std::endl;
    stream << "Max direct distance: " << max_direct_distance_ << std::endl;
    stream << "Max manhattan distance: " << max_manhattan_distance_ << std::endl;
    stream << "Max out degree: " << max_out_degree_ << std::endl;
    stream << "Rand direct distance: " << rand_direct_distance_ << std::endl; 
    stream << "Rand manhattan distance: " << rand_manhattan_distance_ << std::endl; 
    stream << "Rand out degree: " << rand_out_degree_ << std::endl; 
    stream << "Quad SW Lat: " << quad_sw_lat_ << std::endl; 
    stream << "Quad SW Lon: " << quad_sw_lon_ << std::endl; 
    stream << "Quad NE Lat: " << quad_ne_lat_ << std::endl; 
    stream << "Quad NE Lon: " << quad_ne_lon_ << std::endl; 
    stream << "Latitude field: " << lat_field_ << std::endl;
    stream << "Longitude field: " << lon_field_ << std::endl;
    stream << "Heading field: " << heading_field_ << std::endl;
    stream << "Speed field: " << speed_field_ << std::endl;
    stream << "Time field: " << gentime_field_ << std::endl;
    stream << "UID fields: " << uid_fields_ << std::endl;
}

FileInfo::FileInfo(const std::string& path, bool is_multi) :
    path_(path),
    is_multi_(is_multi),
    size_(0),
    is_error_(false)
{
    std::ifstream bin_file(path_, std::ios::binary | std::ios::ate);

    if (bin_file.fail()) {
        is_error_ = true;
        return;
    }

    size_ = bin_file.tellg();
    bin_file.close();
}

const std::string& FileInfo::GetPath(void) const {
    return path_;
}

bool FileInfo::IsMulti(void) const {
    return is_multi_;
}

uint64_t FileInfo::GetSize(void) const {
    return size_;
}

bool FileInfo::IsError(void) const {
    return is_error_;
}

static std::string GetStringVal(v8::Isolate* isolate, v8::Handle<v8::Object> object, const char* field) {
    v8::Handle<v8::Value> local = object->Get(v8::String::NewFromUtf8(isolate, field, v8::String::NewStringType::kNormalString));
    v8::String *string = *(local->ToString());
    char *buff = new char[string->Length() + 1];
    string->WriteUtf8(buff, string->Length());
    std::string ret = std::string(buff, string->Length());

    delete buff;
    return ret;
}

static std::string ToString(v8::Handle<v8::Value> handle) {
    v8::String *string = *(handle->ToString());
    char *buff = new char[string->Length() + 1];
    string->WriteUtf8(buff, string->Length());
    std::string ret = std::string(buff, string->Length());

    delete buff;
    return ret;
}

double GetDoubleVal(v8::Isolate* isolate, v8::Handle<v8::Object> object, const char* field) {
    v8::Handle<v8::Value> local = object->Get(v8::String::NewFromUtf8(isolate, field, v8::String::NewStringType::kNormalString));
    return local->NumberValue();
}

uint32_t GetUInt32Val(v8::Isolate* isolate, v8::Handle<v8::Object> object, const char* field) {
    v8::Handle<v8::Value> local = object->Get(v8::String::NewFromUtf8(isolate, field, v8::String::NewStringType::kNormalString));
    return local->Uint32Value();
}

bool GetBoolVal(v8::Isolate* isolate, v8::Handle<v8::Object> object, const char* field) {
    v8::Handle<v8::Value> local = object->Get(v8::String::NewFromUtf8(isolate, field, v8::String::NewStringType::kNormalString));
    return local->BooleanValue();
}

void GetConfiguration(v8::Isolate* isolate, v8::Handle<v8::Object> object, DIConfig::Ptr config) {
    v8::Handle<v8::Object> config_object = v8::Handle<v8::Object>::Cast(object->Get(v8::String::NewFromUtf8(isolate, "config", v8::String::NewStringType::kNormalString)));

    config->TogglePlotKML(GetBoolVal(isolate, config_object, "kml-output")); 
    config->SetLatField(GetStringVal(isolate, config_object, "lat-field")); 
    config->SetLonField(GetStringVal(isolate, config_object, "lng-field")); 
    config->SetHeadingField(GetStringVal(isolate, config_object, "heading-field")); 
    config->SetSpeedField(GetStringVal(isolate, config_object, "speed-field")); 
    config->SetGentimeField(GetStringVal(isolate, config_object, "time-field")); 
    config->SetUIDFields(GetStringVal(isolate, config_object, "uid-fields")); 
    config->SetFitExt(GetDoubleVal(isolate, config_object, "fit-ext")); 
    config->ToggleScaleMapFit(GetBoolVal(isolate, config_object, "mapfit-scale-toggle"));
    config->SetMapFitScale(GetDoubleVal(isolate, config_object, "mapfit-scale")); 
    config->SetHeadingGroups(GetUInt32Val(isolate, config_object, "heading-groups")); 
    config->SetMinEdgeTripPoints(GetUInt32Val(isolate, config_object, "min-edge-trippoints")); 
    config->SetTAMaxQSize(GetUInt32Val(isolate, config_object, "ta-max-q")); 
    config->SetTAAreaWidth(GetDoubleVal(isolate, config_object, "ta-area-width")); 
    config->SetTAMaxSpeed(GetDoubleVal(isolate, config_object, "ta-max-speed")); 
    config->SetTAHeadingDelta(GetDoubleVal(isolate, config_object, "ta-heading-delta"));
    config->SetStopMaxTime(GetDoubleVal(isolate, config_object, "stop-max-time"));
    config->SetStopMinDistance(GetDoubleVal(isolate, config_object, "stop-min-dist"));
    config->SetStopMaxSpeed(GetDoubleVal(isolate, config_object, "stop-max-speed"));
    config->SetMinDirectDistance(GetDoubleVal(isolate, config_object, "min-direct-distance"));
    config->SetMinManhattanDistance(GetDoubleVal(isolate, config_object, "min-manhattan-distance"));
    config->SetMinOutDegree(GetUInt32Val(isolate, config_object, "min-out-degree")); 
    config->SetMaxDirectDistance(GetDoubleVal(isolate, config_object, "max-direct-distance"));
    config->SetMaxManhattanDistance(GetDoubleVal(isolate, config_object, "max-manhattan-distance"));
    config->SetMaxOutDegree(GetUInt32Val(isolate, config_object, "max-out-degree")); 
    config->SetRandDirectDistance(GetDoubleVal(isolate, config_object, "rand-direct-distance"));
    config->SetRandManhattanDistance(GetDoubleVal(isolate, config_object, "rand-manhattan-distance"));
    config->SetRandOutDegree(GetDoubleVal(isolate, config_object, "rand-out-degree"));
    config->SetQuadSWLat(GetDoubleVal(isolate, config_object, "sw-lat"));
    config->SetQuadSWLon(GetDoubleVal(isolate, config_object, "sw-lng"));
    config->SetQuadNELat(GetDoubleVal(isolate, config_object, "ne-lat"));
    config->SetQuadNELon(GetDoubleVal(isolate, config_object, "ne-lng"));
}

/**
 * Return the total size in bytes of all files that need to be processed.
 */
uint64_t GetFiles(v8::Isolate* isolate, v8::Handle<v8::Object> object, std::vector<FileInfo::Ptr>& files) {
    uint64_t total_size = 0;
    v8::Handle<v8::Array> file_array = v8::Handle<v8::Array>::Cast(object->Get(v8::String::NewFromUtf8(isolate, "files", v8::String::NewStringType::kNormalString)));

    if (!file_array->IsArray()) {
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Module error: files field must be an array.")));
    }

    uint32_t length = GetUInt32Val(isolate, file_array, "length");

    for(uint32_t i = 0; i < length; i++) {
        std::string path = ToString(file_array->Get(i));
        
        FileInfo::Ptr file_info_ptr = std::make_shared<FileInfo>(path, true);
        total_size += file_info_ptr->GetSize();
        files.push_back(file_info_ptr); 
    }

    return total_size;
}

struct data_t {
    // 0 = progress; 1 = message; 2 = thread message?
    unsigned type;
    double progress;
    unsigned thread_index;
};

struct thread_message {
    unsigned type; // 0 log, 1 warn, 2 error, 3 debug
    std::string message;
};

class TrajectoryFactory {
    public:
        using Ptr = std::shared_ptr<TrajectoryFactory>;

        TrajectoryFactory(const std::string& file_path, const std::string& header, const std::string& uid, uint64_t start, uint64_t end) :
            file_path_(file_path),
            header_(header),
            uid_(uid),
            start_(start),
            end_(end),
            size_(end - start)
        {}

        void GetTrajectory(DIConfig::Ptr config, trajectory::Trajectory& traj) const {
            std::ifstream file(file_path_);

            if (file.fail()) {  
                throw  std::invalid_argument("Could not open file: " + file_path_); 
            }

            CSVFactory factory(file, header_, config->GetLatField(), config->GetLonField(), config->GetHeadingField(), config->GetSpeedField(), config->GetGentimeField());
            // fille traj with records from the file.
            factory.make_trajectory(traj,  start_, end_ );
            file.close();
        }

        uint64_t GetSize() const {
            return size_;
        }

        const std::string GetFilePath(void) const {
            return file_path_;
        }

        const std::string GetUID(void) const {
            return uid_;
        }

        const std::string GetHeader(void) const {
            return header_;
        }

    private:
        std::string file_path_;
        std::string header_;
        std::string uid_;
        uint64_t start_;
        uint64_t end_;
        uint64_t size_;
};

Quad::Ptr qptr_ = nullptr;                          // Global quad ptr persists through the life of the module.

/**
 * AsyncProgressWorkerBase provides progress reporting callback support for asynchronous communication with the GUI. An
 * instance of this AsyncWorker must be provided to AsyncQueueWorker to function.
 *
 * Parallelizes building and processing trajectories.
 *
 * Below this template is expanded with T = data_t
 */
class CVDI : public Nan::AsyncProgressWorkerBase<data_t>, public MultiThread::Parallel<TrajectoryFactory>  {
    public:

        CVDI(Callback *callback, Callback *progress, DIConfig::Ptr config_ptr, const std::string& output_dir_path, const std::string& quad_path, bool build_quad, const std::string& log_path, const std::vector<FileInfo::Ptr>& files, uint64_t total_size, unsigned n_threads): 
            Nan::AsyncProgressWorkerBase<data_t>(callback),
            progress(progress),
            config_ptr_(config_ptr),
            output_dir_path_(output_dir_path),
            quad_path_(quad_path),
            build_quad_(build_quad),
            log_path_(log_path),
            files_(files),
            total_size_(total_size),
            n_threads_(n_threads),
            curr_index_(0),
            splitter_ptr_(nullptr),
            multi_file_ptr_(nullptr),
            log_file_ptr_(nullptr),
            curr_file_(nullptr)
        {}

        ~CVDI() {}

        /**
         * Deidentifies a single trajectory
         */
        void DeIdentify(TrajectoryFactory::Ptr traj_factory_ptr) {
            std::string uid = traj_factory_ptr->GetUID();
            std::string header = traj_factory_ptr->GetHeader();
            trajectory::Trajectory traj;
            
            // Opens trajectory CSV file uses configuration to determine fields.
            traj_factory_ptr->GetTrajectory(config_ptr_, traj);

            ErrorCorrector ec(50);
            ec.correct_error(traj, uid);
        
            MapFitter mf{qptr_, config_ptr_->GetMapFitScale(), config_ptr_->GetFitExt()};
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

            PrivacyIntervalFinder pif(config_ptr_->GetMinDirectDistance(), 
                                      config_ptr_->GetMinManhattanDistance(), 
                                      config_ptr_->GetMinOutDegree(), 
                                      config_ptr_->GetMaxDirectDistance(), 
                                      config_ptr_->GetMaxManhattanDistance(), 
                                      config_ptr_->GetMaxOutDegree(), 
                                      config_ptr_->GetRandDirectDistance(), 
                                      config_ptr_->GetRandManhattanDistance(), 
                                      config_ptr_->GetRandOutDegree());
            trajectory::Interval::PtrList priv_intervals =  pif.find_intervals( traj );

            PrivacyIntervalMarker pim({ priv_intervals });
            pim.mark_trajectory(traj);

            DeIdentifier di;

            trajectory::Trajectory di_traj = di.de_identify(traj);

            // Write de-identified trajectory to file.
            std::string out_file_path = output_dir_path_ + "/di_out/" + uid + ".di.csv";
            std::ofstream os(out_file_path, std::ofstream::trunc);

            if (os.fail()) {
                throw std::invalid_argument("Could not open output file: " + out_file_path);
            }

            CSVFactory::write_trajectory(os, header, di_traj);
            os.close();
            
            if (!config_ptr_->IsPlotKML()) {
                return;
            }

            // Create annotated KML version of trip.
            std::string kml_file_path = output_dir_path_ + "/kml_out/" + uid + ".di.kml";
            std::ofstream kml_os(kml_file_path, std::ofstream::trunc);

            if (os.fail()) {
                throw std::invalid_argument("Could not open KML file: " + kml_file_path);
            }

            KML::File kml_file(kml_os, uid);
            kml_file.write_poly_style( "explicit_boxes", 0xff990000, 1 );
            kml_file.write_poly_style( "implicit_boxes", 0xff0033ff, 1 );
            kml_file.write_line_style( "ci_intervals", 0xffff00ff, 7 );
            kml_file.write_line_style( "priv_intervals", 0xff00ffff, 5 );
            kml_file.write_trajectory( traj, true );
            kml_file.write_areas(mf.area_set, "explicit_boxes");
            kml_file.write_areas(imf.area_set, "implicit_boxes");
            kml_file.write_intervals( stop_critical_intervals, traj, "ci_intervals", "stop_marker_style" );
            kml_file.write_intervals( ta_critical_intervals, traj, "ci_intervals", "turnaround_marker_style" );
            kml_file.write_intervals( priv_intervals, traj, "priv_intervals" );
            kml_file.finish();
            kml_os.close();
        }

        void SendProgress(int thread_num) {
            thread_data_[thread_num]->type = 0; // progress 
            thread_data_[thread_num]->progress = GetProgress();
            progress_->Send(thread_data_[thread_num], sizeof((*thread_data_[thread_num])));
            Sleep(5);
        }

        void ProgressDone() {
            message_data_.type = 0;
            message_data_.progress = 1.0;
            progress_->Send(&message_data_, sizeof(message_data_));
            Sleep(5);
        }

        void SendMessage(int severity, const std::string& message) {
            message_data_.type = 1;
            message_.message = message;
            message_.type = severity;
            progress_->Send(&message_data_, sizeof(message_data_));
            Sleep(5);
        }

        void SendThreadMessage(int thread_num, int severity, const std::string& message) {
            thread_data_[thread_num]->type = 02; // thread message
            thread_data_[thread_num]->thread_index = thread_num;
            messages_[thread_num]->type = severity;
            messages_[thread_num]->message = message;
            progress_->Send(thread_data_[thread_num], sizeof((*thread_data_[thread_num])));
            Sleep(5);
        }

        void ReportLog(const std::string& message) {
            SendMessage(0, "[DI Log] " + message); 
        }

        void ReportWarning(const std::string& message) {
            SendMessage(1, "[DI Warning] " + message); 
        }

        void ReportError(const std::string& message) {
            SendMessage(2, "[DI Error] " + message); 
        }

        void ReportDebug(const std::string& message) {
            SendMessage(3, "[DI Debug] " + message); 
        }

        void ReportThreadLog(int thread_num, const std::string& message) {
            SendThreadMessage(thread_num, 0, "[DI] " + message); 
        }

        void ReportThreadWarning(int thread_num, const std::string& message) {
            SendThreadMessage(thread_num, 1, "[DI Warning] " + message); 
        }

        void ReportThreadError(int thread_num, const std::string& message) {
            SendThreadMessage(thread_num, 2, "[DI Error] " + message); 
        }

        void ReportThreadDebug(int thread_num, const std::string& message) {
            SendThreadMessage(thread_num, 3, "[DI Debug] " + message); 
        }

        void Thread(int thread_num, MultiThread::SharedQueue<TrajectoryFactory::Ptr>* q) 
        {
            TrajectoryFactory::Ptr traj_factory_ptr;

            while ((traj_factory_ptr = q->pop()) != nullptr) {
                try {
                    // DeIdentify
                    DeIdentify(traj_factory_ptr);
    
                    // Update the progress for this thread.
                    // unique to this thread.
                    work_[thread_num] += traj_factory_ptr->GetSize();

                    SendProgress(thread_num);

                    ReportThreadLog(thread_num, "De-identified trip: " + traj_factory_ptr->GetUID() + " from file: " + traj_factory_ptr->GetFilePath());

                } catch (std::exception& e) {
                    std::string err = "De-identification error: ";
                    err += e.what();
                    ReportThreadError(thread_num, err);

                    continue;
                }
            }
        }

        bool Init(void) {
            log_file_ptr_ = std::make_shared<std::ofstream>(log_path_, std::ofstream::trunc);

            if (log_file_ptr_->fail()) {
                ReportError("Could not open the log file: " + log_path_);
                return false;
            }


            if (build_quad_ || qptr_ == nullptr) {
                ReportLog("Building quad from file: " + quad_path_);
            
                geo::Point sw(config_ptr_->GetQuadSWLat(), config_ptr_->GetQuadSWLon());
                geo::Point ne(config_ptr_->GetQuadNELat(), config_ptr_->GetQuadNELon());

                qptr_ = std::make_shared<Quad>(sw, ne);
                shapes::CSVInputFactory shape_factory(quad_path_);

                try {
                    shape_factory.make_shapes();
                } catch (std::exception& e) {
                    std::string err = "Error processing quad file: " + quad_path_ + " :" ;
                    err += e.what();
                    ReportError(err);

                    return false;
                }

                for (auto& edge_ptr : shape_factory.get_edges()) {
                    Quad::insert(qptr_, std::dynamic_pointer_cast<const geo::Entity>(edge_ptr)); 
                }

            } else {
                ReportLog("Reusing quad from file: " + quad_path_);
            }

            return true;
        }

        void Close(void) {
            if (log_file_ptr_) {
                log_file_ptr_->close();
            }
        }
                
        uint64_t ItemSize(TrajectoryFactory& traj_factory) {
            return traj_factory.GetSize();
        }

        TrajectoryFactory::Ptr NextMultiItem(void) {
            CSVSplitter::TripLocationPtr locptr = splitter_ptr_->next_trajectory();
            
            if (locptr) {
                std::shared_ptr<TrajectoryFactory> ret = std::make_shared<TrajectoryFactory>(curr_file_->GetPath(), splitter_ptr_->get_header(), std::get<0>(*locptr), std::get<1>(*locptr), std::get<2>(*locptr));

                return std::static_pointer_cast<TrajectoryFactory>(ret);
            } else {
                return nullptr;
            }
        }

        TrajectoryFactory::Ptr NextItem(void) {
            while (true) {
                if (splitter_ptr_) {
                    TrajectoryFactory::Ptr ret = NextMultiItem();

                    if (ret) {
                        return ret;
                    } else {
                        splitter_ptr_ = nullptr;
                        multi_file_ptr_->close();
                    }
                }
                
                if (curr_index_ >= files_.size()) {
                    return nullptr;
                }

                curr_file_ = files_[curr_index_];
                curr_index_++;
                
                std::ifstream file(curr_file_->GetPath(), std::ios::binary | std::ios::ate);
                
                if (file.fail()) {  
                    ReportError("Could not open file: " + curr_file_->GetPath()); 
                
                    continue;
                }

                uint64_t size = file.tellg();
                file.close();

                multi_file_ptr_ = std::make_shared<std::ifstream>(curr_file_->GetPath());
                
                if (multi_file_ptr_->fail()) {  
                    ReportError("Could not open file: " + curr_file_->GetPath()); 

                    continue;
                }

                splitter_ptr_ = std::make_shared<CSVSplitter>(*multi_file_ptr_, size, config_ptr_->GetUIDFields(), true);

                TrajectoryFactory::Ptr ret = NextMultiItem();

                if (ret) {
                    return ret;
                } else {
                    splitter_ptr_ = nullptr;
                    multi_file_ptr_->close();
                }
            }
                
            return nullptr;
        }

        /**
         * Work is based on total number of bytes processed. The update blocks are in completed file sizes.
         */
        double GetProgress() {
            uint64_t total_work = 0;

            for (unsigned i = 0; i < n_threads_; ++i) {
                total_work += work_[i];
            }

            return static_cast<double>(total_work) / static_cast<double>(total_size_);
        }

        /**
         * Implementation of virtual base method in AsyncWorker
         */
        void Execute(const typename Nan::AsyncProgressWorkerBase<data_t>::ExecutionProgress& progress) {

            // Set up a vector of output queues to store the messages from each thread.
            for (unsigned i = 0; i < n_threads_; ++i) {
                work_.push_back(0);
                messages_.push_back(new thread_message);
                thread_data_.push_back(new data_t);
            }

            progress_ = &progress;

            // Start all the de-identification threads; this blocks until complete.
            Start(n_threads_);

            // Make the progress bar full.
            ProgressDone();

            // All threads will be finished at this point.
            for (unsigned i = 0; i < n_threads_; ++i) {
                delete messages_[i];
                delete thread_data_[i];
            }
        }

        /**
         * Part of the AsyncQueueWorker Interface. Implementation of virtual function.
         */
        void HandleProgressCallback(const data_t *data, size_t size) {
            HandleScope scope;
            v8::Local<v8::Object> obj = Nan::New<v8::Object>();
            unsigned thread_num, type;

            switch (data->type) {           
                case 0:         // progress
                    Nan::Set(obj, Nan::New("type").ToLocalChecked(), New<v8::String>("progress").ToLocalChecked());
                    Nan::Set(obj, Nan::New("progress").ToLocalChecked(), New<v8::Number>(data->progress));

                    break;

                case 1:         // message or thread message
                    type = message_.type;

                    if (type == 0) {
                        // Log only goes to file.
                        *log_file_ptr_ << message_.message << std::endl;
                        return;

                    } else if (type == 1) {
                        Nan::Set(obj, Nan::New("type").ToLocalChecked(), New<v8::String>("warning").ToLocalChecked());
                        *log_file_ptr_ << message_.message << std::endl;

                    } else if (type == 2) {
                        Nan::Set(obj, Nan::New("type").ToLocalChecked(), New<v8::String>("error").ToLocalChecked());
                        *log_file_ptr_ << message_.message << std::endl;

                    } else if (type == 3) {
                        Nan::Set(obj, Nan::New("type").ToLocalChecked(), New<v8::String>("debug").ToLocalChecked());
                        *log_file_ptr_ << message_.message << std::endl;

                    }
                   
                    Nan::Set(obj, Nan::New("message").ToLocalChecked(), New<v8::String>(message_.message).ToLocalChecked());
                    break;

                case 2:         // thread message.
                    thread_num = data->thread_index;
                    type = messages_[thread_num]->type;

                    if (type == 0) {
                        // Log only goes to file.
                        *log_file_ptr_ << messages_[thread_num]->message << std::endl;
                        return;

                    } else if (type == 1) {
                        Nan::Set(obj, Nan::New("type").ToLocalChecked(), New<v8::String>("warning").ToLocalChecked());
                        *log_file_ptr_ << messages_[thread_num]->message << std::endl;

                    } else if (type == 2) {
                        Nan::Set(obj, Nan::New("type").ToLocalChecked(), New<v8::String>("error").ToLocalChecked());
                        *log_file_ptr_ << messages_[thread_num]->message << std::endl;

                    } else if (type == 3) {
                        Nan::Set(obj, Nan::New("type").ToLocalChecked(), New<v8::String>("debug").ToLocalChecked());
                        *log_file_ptr_ << messages_[thread_num]->message << std::endl;
                    }
                   
                    Nan::Set(obj, Nan::New("message").ToLocalChecked(), New<v8::String>(messages_[thread_num]->message).ToLocalChecked());
                    break;

                default:
                    break;
            }

            v8::Local<v8::Value> argv[] = { obj };
            progress->Call(1, argv);
        }
    private:
        Callback *progress;
        std::string config_str_;
        std::string debug_str_;
        DIConfig::Ptr config_ptr_;
        std::string output_dir_path_;
        std::string quad_path_;
        bool build_quad_;
        std::string log_path_;
        std::vector<FileInfo::Ptr> files_;
        uint64_t total_size_;
        unsigned n_threads_;
        uint32_t curr_index_;
        CSVSplitter::Ptr splitter_ptr_;
        std::shared_ptr<std::ifstream> multi_file_ptr_;
        std::shared_ptr<std::ofstream> log_file_ptr_;
        FileInfo::Ptr curr_file_;
        std::vector<uint64_t> work_; 

        std::vector<thread_message*> messages_;
        std::vector<data_t*> thread_data_;
        thread_message message_;
        data_t message_data_;

        const typename Nan::AsyncProgressWorkerBase<data_t>::ExecutionProgress *progress_;
};

/**
 * This helper macro expands to: void DeIdentify(const Nan::FunctionCallbackInfo<v8::Value>& info)
 * The info reference provides a set of methods to retrieve information about a callback.
 */
NAN_METHOD(DeIdentify) {
    // Config and files for DI.
    DIConfig::Ptr config = std::make_shared<DIConfig>();
    std::vector<FileInfo::Ptr> files;
    uint64_t total_size = 0;

    v8::Isolate* isolate = info.GetIsolate();

    // Get the first argument from: cvdiModule.deIdentify(diObject, diCallback, function () {});
    v8::Handle<Object> di_object = v8::Handle<Object>::Cast(info[0]);

    GetConfiguration(isolate, di_object, config);
    std::string quad_path = GetStringVal(isolate, di_object, "quadFile");
    std::string log_path = GetStringVal(isolate, di_object, "logFile");
    std::string output_dir_path = GetStringVal(isolate, di_object, "outputDir");
    bool build_quad = GetBoolVal(isolate, di_object, "buildQuad");
    total_size = GetFiles(isolate, di_object, files);

    // Get the second argument from: cvdiModule.deIdentify(diObject, diCallback, function () {});
    // Based on the parameter sent to diCallback GUI components will be updated.
    Callback *progress = new Callback(info[1].As<v8::Function>());
    
    // Get the third argument from: cvdiModule.deIdentify(diObject, diCallback, function () {});
    Callback *callback = new Callback(info[2].As<v8::Function>());

    // Create a worker with the JS callback.
    // Run the execute routine async.
    AsyncQueueWorker(new CVDI(callback, progress, config, output_dir_path, quad_path, build_quad, log_path, files, total_size, std::thread::hardware_concurrency()));
}

// Defines the entry point function to a Node add-on.
NAN_MODULE_INIT(Init) {  
  Set(
    target,
    New("deIdentify").ToLocalChecked(),
    GetFunction(New<FunctionTemplate>(DeIdentify)).ToLocalChecked()
  );
}

NODE_MODULE(cvdi, Init)
