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
#include "cvdi_driver.hpp"
#include "cvlib.hpp"
    
CSVFactory::CSVFactory( std::istream& stream, int lat_index, int lon_index, int heading_index, int speed_index, int gentime_index, bool has_header, char delimiter) :
    RX_DEVICE{ 0 },
    FILE_ID{ 1 },
    TX_DEV{ 2 },
    GENTIME{ gentime_index },
    TX_RANDOM{ 4 },
    MSG_COUNT{ 5 },
    DSECONDS{ 6 },
    LAT{ lat_index },
    LON{ lon_index },
    ELEVATION{ 9 },
    SPEED{ speed_index },
    HEADING{ heading_index },
    AX{ 12 },
    AY{ 13 },
    AZ{ 14 },
    YAW_RATE{ 15 },
    PATH_COUNT{ 16 },
    RADIUS_OF_CURVE{ 18 },
    CONFIDENCE{ 19 },
    stream_{ stream },
    has_header{ has_header },
    delimiter{ delimiter },
    index{ 0 }
    {
        if (has_header)
        {
           std::getline( stream_, header );
        }
    }

CSVFactory::CSVFactory( std::istream& stream, const std::string& lat_field, const std::string& lon_field, const std::string& heading_field, const std::string& speed_field, const std::string& gentime_field, char delimiter) :
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
    lat_field{ lat_field },
    lon_field{ lon_field },
    heading_field{ heading_field },
    speed_field{ speed_field },
    gentime_field{ gentime_field },
    has_header{ true },
    delimiter{ delimiter },
    index{ 0 }
    {
        if (std::getline( stream_, header ))
        {
            map_index_fields();
        }
    }

CSVFactory::CSVFactory( std::istream& stream, char delimiter) :
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
    lat_field{ "Latitude" },
    lon_field{ "Longitude" },
    heading_field{ "Heading" },
    speed_field{ "Speed" },
    gentime_field{ "Gentime" },
    has_header{ true },
    delimiter{ delimiter },
    index{ 0 }
    {
        if (std::getline( stream_, header ))
        {
            map_index_fields();
        }
    }

CSVFactory::CSVFactory( std::istream& stream, const std::string& header, const std::string& lat_field, const std::string& lon_field, const std::string& heading_field, const std::string& speed_field, const std::string& gentime_field, char delimiter) :
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
        has_header{ true },
        delimiter{ delimiter },
        index{ 0 }
    {
        map_index_fields();
    }

CSVFactory::CSVFactory( std::ifstream& stream, const std::string& header, char delimiter ) :
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
    lat_field{ "Latitude" },
    lon_field{ "Longitude" },
    heading_field{ "Heading" },
    speed_field{ "Speed" },
    gentime_field{ "Gentime" },
    has_header{ true },
    delimiter{ delimiter },
    index{ 0 }
    {
        map_index_fields();
    }

    /**
     *
     * \throws out_of_range and invalid_argument when the fileline cannot be parsed.
     */
Trajectory::Point::Ptr CSVFactory::make_point( const std::string& fileline )
{
    StrVector parts = util::split( fileline, delimiter );

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

    return std::make_shared<Trajectory::Point>( fileline, gentime, lat, lon, heading, speed, index++ );
}

void CSVFactory::make_trajectory(Trajectory::Trajectory& traj) 
{
    std::string line;

    while ( std::getline( stream_, line ) ) {
        try {
            traj.push_back( make_point( line ) );
        } catch ( std::exception& e ) {
            // Failed to make point.
        }
    }
}

void CSVFactory::make_trajectory( Trajectory::Trajectory& traj, uint64_t start, uint64_t end )
{
    stream_.seekg( start ); 

    std::string line;

    while ( static_cast<uint64_t>(stream_.tellg()) < end && std::getline( stream_, line ) ) {
        try {
            traj.push_back( make_point( line ) );
        } catch ( std::exception& e ) {
            // Failed to make point.
        }
    }
}

void CSVFactory::write_trajectory( std::ostream& stream, const std::string& header, const Trajectory::Trajectory& traj ) 
{
    stream << header << std::endl;

    for(auto& tp : traj)
    {
        stream << tp->get_data() << std::endl;    
    } 
}

bool CSVFactory::HasHeader() const {
    return has_header;
}

const std::string CSVFactory::GetHeader() const {
    return header;
}

void CSVFactory::map_index_fields()
{
    StrVector parts = util::split( header, delimiter );
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

    // Get the next line and find the UID.
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

    // Get the next line and find the UID.
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

    // Get the next line and find the UID.
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

    // Get the next line and find the UID.
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

    // Get the next line and find the UID.
    std::getline( stream_, curr_line );
    uid = get_curr_uid(); 
}

const CSVSplitter::TripLocationPtr CSVSplitter::next_trajectory( )
{
    if (end >= size)
    {
        return nullptr;
    }

    // Find the indices within the stream of the next trip.
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
    StrVector uid_parts = util::split( uid_fields, delimiter );
    StrVector parts = util::split( header, delimiter );
    bool found = false;
    int part_index = 0;

    for (auto& field : uid_parts) 
    {
        found = false;

        // Only search the parts we havn't already searched.
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

    parts = util::split( line, delimiter );

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
    StrVector parts = util::split( curr_line, delimiter );

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

    // Find the next trip by doing a linear search on the file.
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
    StrVector uid_parts = util::split( uid_fields, delimiter );
    StrVector parts = util::split( header, delimiter );
    bool found = false;
    int part_index = 0;

    for (auto& field : uid_parts) 
    {
        found = false;

        // Only search the parts we havn't already searched.
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

SingleTrajectoryFactory::SingleTrajectoryFactory(const std::string& file_path, const std::string& uid_fields) :
    file_path_(file_path)
{
    std::ifstream bin_file(file_path, std::ios::binary | std::ios::ate);

    if (bin_file.fail()) {  
        throw std::invalid_argument("Could not open file: " + file_path); 
    }

    size_ = bin_file.tellg();
    bin_file.close();

    std::ifstream file(file_path);

    if (file.fail()) {  
        throw std::invalid_argument("Could not open file: " + file_path); 
    }

    if (!std::getline(file, header_)) {
        throw std::invalid_argument("File missing header: " + file_path); 
    }

    std::string line;

    if (!std::getline(file, line)) {
        throw std::invalid_argument("File is empty: " + file_path); 
    }

    uid_ = CSVSplitter::get_uid(header_, line, uid_fields);
    file.close();
}

void SingleTrajectoryFactory::GetTrajectory(DIConfig::Ptr config, Trajectory::Trajectory& traj) const {
    std::ifstream file(file_path_);

    if (file.fail()) {  
        throw  std::invalid_argument("Could not open file: " + file_path_); 
    }

    CSVFactory factory(file, config->GetLatField(), config->GetLonField(), config->GetHeadingField(), config->GetSpeedField(), config->GetGentimeField());
    
    factory.make_trajectory(traj);
    file.close();
}

uint64_t SingleTrajectoryFactory::GetSize() const {
    return size_;
}

const std::string SingleTrajectoryFactory::GetUID(void) const {
    return uid_;
}

const std::string SingleTrajectoryFactory::GetHeader(void) const {
    return header_;
}

// MultiPartTrajectoryFactory

MultiPartTrajectoryFactory::MultiPartTrajectoryFactory(const std::string& file_path, const std::string& header, const std::string& uid, uint64_t start, uint64_t end) :
    file_path_(file_path),
    header_(header),
    uid_(uid),
    start_(start),
    end_(end),
    size_(end - start)
{}

void MultiPartTrajectoryFactory::GetTrajectory(DIConfig::Ptr config, Trajectory::Trajectory& traj) const {
    std::ifstream file(file_path_);

    if (file.fail()) {  
        throw  std::invalid_argument("Could not open file: " + file_path_); 
    }

    CSVFactory factory(file, header_, config->GetLatField(), config->GetLonField(), config->GetHeadingField(), config->GetSpeedField(), config->GetGentimeField());

    factory.make_trajectory(traj,  start_, end_ );

    file.close();
}

uint64_t MultiPartTrajectoryFactory::GetSize() const {
    return size_;
}

const std::string MultiPartTrajectoryFactory::GetUID(void) const {
    return uid_;
}

const std::string MultiPartTrajectoryFactory::GetHeader(void) const {
    return header_;
}
