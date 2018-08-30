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
#include "geo_data.hpp"

#include "util.hpp"

#include <iomanip>

#include <rapidjson/istreamwrapper.h>

namespace geo_data {

OSMConfigMap osm_config_map(const std::string& config_file_path) {
    OSMConfigMap ret;

    std::ifstream in_file(config_file_path);

    if (in_file.fail()) {
        throw std::invalid_argument("Could not open input config file: " + config_file_path);
    }

    rapidjson::IStreamWrapper isw(in_file);   
    rapidjson::Document json_doc;
    
    if (json_doc.ParseStream(isw).HasParseError() || !json_doc.IsObject()) {
        throw std::invalid_argument("Could not parse input config file: " + config_file_path);
    }

    in_file.close();

    if (!json_doc.HasMember("tags")) {
        throw std::invalid_argument("Input config file missing \"tags\".");
    } 

    rapidjson::Value& tags = json_doc["tags"];

    if (!tags.IsArray()) {
        throw std::invalid_argument("Input config \"tags\" is not an array.");
    } 
        
    for (rapidjson::SizeType i = 0; i < tags.Size(); i++) {
        if (!tags[i].HasMember("values")) {
            throw std::invalid_argument("Input config \"tags\" has no value \"values\" member.");
        }

        rapidjson::Value& values = tags[i]["values"];

        if (!values.IsArray()) {
            throw std::invalid_argument("Input config \"values\" is not an array.");
        }

        for (rapidjson::SizeType j = 0; j < values.Size(); j++) {
            if (!values[j].HasMember("id") || !values[j]["id"].IsNumber()) {
                throw std::invalid_argument("Input config \"value\" missing \"id\" number.");
            } 

            if (!values[j].HasMember("priority") || !values[j]["id"].IsNumber()) {
                throw std::invalid_argument("Input config \"value\" missing \"priority\" number.");
            }

            if (!values[j].HasMember("maxspeed") || !values[j]["maxspeed"].IsNumber()) {
                throw std::invalid_argument("Input config \"value\" missing \"maxspeed\" number.");
            }

            if (!values[j].HasMember("width") || !values[j]["width"].IsNumber()) {
                throw std::invalid_argument("Input config \"value\" missing \"width\" number.");
            }

            if (!values[j].HasMember("exclude") || !values[j]["exclude"].IsBool()) {
                throw std::invalid_argument("Input config \"value\" missing \"exclude\" bool.");
            }
        
            ret[values[j]["id"].GetInt()] = std::make_tuple<float, int, double, bool>(values[j]["priority"].GetFloat(), values[j]["maxspeed"].GetInt(), values[j]["width"].GetDouble(), values[j]["exclude"].GetBool());
        }
    } 

    return ret; 
}

PostGISRoadReader::PostGISRoadReader(const std::string& host, uint32_t port, const std::string& database, const std::string& user, const std::string& password, const OSMConfigMap& osm_config_map) :
    result_index_(0),
    result_set_(nullptr),
    osm_config_map_(osm_config_map)
{
    std::stringstream ss;
    ss << "dbname=" << database << " user=" << user << " password=" << password << " hostaddr=" << host << " port=" << port;    

    try {
        conn_ptr_ = std::make_shared<pqxx::connection>(ss.str());
    } catch (const pqxx::broken_connection& e) {
        throw std::invalid_argument("Could not connect to the database: " + std::string(e.what()));
    }
    
    result_set_ = execute(kWayQueryPrefix);
}

PostGISRoadReader::~PostGISRoadReader() {
    try {
        conn_ptr_->disconnect();
    } catch (pqxx::pqxx_exception&) {
        // do nothing
    }
}

geo::Road::Ptr PostGISRoadReader::next_road(void) {
    int maxspeed_forward = 0;
    int maxspeed_backward = 0;
    float priority = 0.0;
    long gid = -1;
    long osm_id = -1;
    long class_id = -1;
    long source = -1;
    long target = -1;
    double reverse = -1.0;
    double width = -1.0;
    bool is_excluded = false;
    int maxspeed;
    std::string geom;
    std::string field;

    if (!result_set_ || result_index_ >= result_set_->size()) {
        return nullptr;
    }

    pqxx::result::tuple row = result_set_->at(result_index_);
    result_index_++;

    try {
        gid = std::stol(row[0].as(std::string()));
        osm_id = std::stol(row[1].as(std::string()));
        class_id = std::stol(row[2].as(std::string()));
        source = std::stol(row[3].as(std::string()));
        target = std::stol(row[4].as(std::string()));
        std::string geom = row[10].as(std::string());

        OSMConfigMap::const_iterator it = osm_config_map_.find(class_id);
        
        if (it == osm_config_map_.end()) {
            return std::make_shared<geo::Road>(gid, osm_id, source, target, reverse, class_id, priority, maxspeed_forward, maxspeed_backward, width, is_excluded, geom, false, "Road type " + std::to_string(class_id) + " is  not in supported.");
        }

        priority = std::get<0>(it->second);
        maxspeed = std::get<1>(it->second);
        width = std::get<2>(it->second);
        is_excluded = std::get<3>(it->second);

        reverse = std::stod(row[6].as(std::string())) * 1000.0;
        field = row[7].as(std::string());
        maxspeed_forward = !field.empty() ? std::stoi(field) : maxspeed;
        field = row[8].as(std::string()); 
        maxspeed_backward = !field.empty() ? std::stoi(field) : maxspeed;
    
        return std::make_shared<geo::Road>(gid, osm_id, source, target, reverse, class_id, priority, maxspeed_forward, maxspeed_backward, width, is_excluded, geom);
    } catch (std::invalid_argument& e) {
        return std::make_shared<geo::Road>(gid, osm_id, source, target, reverse, class_id, priority, maxspeed_forward, maxspeed_backward, width, is_excluded, geom, false, std::string(e.what()));
    }
}

PostGISRoadReader::ResultPtr PostGISRoadReader::execute(const std::string& query_string) {
    ResultPtr ret = nullptr;

    pqxx::nontransaction non_trans(*conn_ptr_);

    try {
         ret = std::make_shared<pqxx::result>(non_trans.exec(query_string));
    } catch (const pqxx::pqxx_exception& e) {
        throw std::invalid_argument("Could not execute query due to pqxx error " + std::string(e.base().what()));
    } catch (const std::exception& e) {
        throw std::invalid_argument("Could not execute query: " + std::string(e.what()));
    } 

    return ret;
}

CSVRoadWriter::CSVRoadWriter(const std::string& file_path) {
    out_file_ = std::ofstream(file_path, std::ofstream::trunc);

    if (out_file_.fail()) {
        error_ = true;

        throw std::invalid_argument("Could not open output road file: " + file_path);
    }

    out_file_ << kRoadCSVHeader << std::endl;
}

CSVRoadWriter::~CSVRoadWriter() {
    out_file_.close();
}

void CSVRoadWriter::write_road(geo::Road& road) {
    if (error_) {
        return;
    }

    out_file_ << std::setprecision(16) << road << std::endl;
}

CSVRoadReader::CSVRoadReader(const std::string& file_path) {
    std::string line;
    in_file_ = std::ifstream(file_path);

    if (in_file_.fail()) {
        error_ = true;

        throw std::invalid_argument("Could not open input road file: " + file_path);
    }

    if (!std::getline(in_file_, line)) {
        error_ = true;

        throw std::invalid_argument("Input road file missing header!");
    }
}

CSVRoadReader::~CSVRoadReader() {
    in_file_.close();
}

geo::Road::Ptr CSVRoadReader::next_road(void) {
    int maxspeed_forward = 0;
    int maxspeed_backward = 0;
    float priority = 0.0;
    long gid = -1;
    long osm_id = -1;
    long class_id = -1;
    long source = -1;
    long target = -1;
    double reverse = -1.0;
    double width = -1.0;
    bool is_valid = false;
    bool is_excluded = false;
    std::string geom;
    std::string error;
    std::string line;

    if (error_) {
        return nullptr;
    }

    if (std::getline(in_file_, line)) {
        util::StrVector parts = util::split_string(line); 

        if (parts.size() < kRoadCSVNumFields) {
            return std::make_shared<geo::Road>(gid, osm_id, source, target, reverse, class_id, priority, maxspeed_forward, maxspeed_backward, width, is_excluded, geom, false, "Road file line missing fields.");
        }

        try {
            gid = std::stol(parts[0]);
            source = std::stol(parts[1]);
            target = std::stol(parts[2]);
            osm_id = std::stol(parts[3]);
            reverse = std::stod(parts[4]);
            class_id = std::stol(parts[5]);
            priority = std::stof(parts[6]);
            maxspeed_forward = std::stoi(parts[7]);
            maxspeed_backward = std::stoi(parts[8]);
            width = std::stod(parts[9]);
            is_excluded = !!std::stoi(parts[10]);
            geom = parts[11];
            is_valid = !!std::stoi(parts[12]);
            error = parts[13];

            return std::make_shared<geo::Road>(gid, osm_id, source, target, reverse, class_id, priority, maxspeed_forward, maxspeed_backward, width, is_excluded, geom, is_valid, error);
        } catch (std::invalid_argument& e) {
            return std::make_shared<geo::Road>(gid, osm_id, source, target, reverse, class_id, priority, maxspeed_forward, maxspeed_backward, width, is_excluded, geom, false, std::string(e.what()));
        }
    } else {
        return nullptr;
    }
}

Interval::Interval( Index left, Index right, std::string aux, uint32_t type ) :
    left_{left},
    right_{right},
    aux_set_{ { aux } },
    type_{ type }
{}

Interval::Interval( Index left, Index right, Interval::AuxSetPtr aux_set, uint32_t type ) :
    left_{left},
    right_{right},
    aux_set_{ *aux_set },
    type_{ type }
{}

uint32_t Interval::type() const
{
    return type_;
}

Index Interval::left() const
{
    return left_;
}

Index Interval::right() const
{
    return right_;
}

Interval::AuxSetPtr Interval::aux_set() const
{
    return std::make_shared<AuxSet>(aux_set_);
}

const std::string Interval::aux_str() const {
    std::string aux_types = "";
    bool first = true;

    for (auto& aux : aux_set_) {
        if (first) {
            first = false;    
        } else {
            aux_types += ";";
        }

        aux_types += aux;
    }
    
    return aux_types;
}

bool Interval::contains( Index value ) const 
{
    return value >= left_ && value < right_;
}

bool Interval::is_before( Index value ) const
{
    return right_ <= value;
}

std::ostream& operator<< ( std::ostream& os, const Interval& interval )
{
    std::string aux_types = "";
    bool first = true;

    for (auto& aux : interval.aux_set_) 
    {
        if (first) 
        {
            first = false;    
        } else
        {
            aux_types += ", ";
        }

        aux_types += aux;
    }

    return os << "type = " << interval.type_ << " [" << interval.left_ << ", " << interval.right_ << " ) types: { " << aux_types << " }";
}

Sample::Sample(const std::string& id, std::size_t index, long timestamp, double lat, double lon, double azimuth, double speed, const std::string& record, bool is_valid, SampleError error_type, const std::string& error_msg) :
    id_(id),
    index_(index),
    raw_index_(index),
    timestamp_(timestamp),
    azimuth_(azimuth),
    speed_(speed),
    record_(record),
    is_valid_(is_valid),
    error_type_(error_type),
    error_msg_(error_msg),
    matched_edge_(nullptr),
    fit_edge_(nullptr),
    is_explicit_fit_(true),
    interval_(nullptr),
    out_degree_(0)
{
    point.setX(lon);
    point.setY(lat);
}

std::string Sample::id() const {
    return id_;
}

std::size_t Sample::index() const {
    return index_;
}

std::size_t Sample::raw_index() const {
    return raw_index_;
}

long Sample::timestamp() const {
    return timestamp_;
}

double Sample::lat() const {
    return point.getY();
}

double Sample::lon() const {
    return point.getX();
}

double Sample::azimuth() const {
    return azimuth_;
}

double Sample::speed() const {
    return speed_;
}

std::string Sample::record() const {
    return record_;
}

bool Sample::is_valid() const {
    return is_valid_;
}

std::string Sample::error_msg() const {
    return error_msg_;
}

geo::Edge::Ptr Sample::matched_edge() const {
    return matched_edge_;
}

geo::Edge::Ptr Sample::fit_edge() const {
    return fit_edge_;
}

void Sample::matched_edge(geo::Edge::Ptr edge) {
    matched_edge_ = edge;
}

void Sample::fit_edge(geo::Edge::Ptr edge) {
    fit_edge_ = edge;
}

bool Sample::is_explicit_fit() const {
    return is_explicit_fit_;
}

void Sample::is_explicit_fit(bool is_explicit_fit) {
    is_explicit_fit_ = is_explicit_fit;
}

void Sample::interval(Interval::Ptr interval) {
    interval_ = interval;
}

Interval::Ptr Sample::interval() const {
    return interval_;
}

void Sample::out_degree(uint32_t out_degree) {
    out_degree_ = out_degree;
}

uint32_t Sample::out_degree() const {
    return out_degree_;
}

void Sample::index(std::size_t index) {
    index_ = index;
}

SampleError Sample::error_type() const {
    return error_type_;
}

Sample::Trace make_trace(const std::string& input) {
    double lat = 90.0;
    double lon = 180.0;
    // TODO read in azimuth from csv
    double heading = geo::kNAN;
    double speed = geo::kNAN;
    long timestamp = -1;
    double gentime;
    bool is_valid = true;
    std::size_t index = 0;
    SampleError error_type;
    Sample::Trace trace;
    std::string id;
    std::string record;
    std::string line;
    std::ifstream in_file = std::ifstream(input);

    if (in_file.fail()) {
        throw std::invalid_argument("Could not open BSMP1 CSV trace file: " + input);
    }

    if (!std::getline(in_file, line)) {
        throw std::invalid_argument("BSMP1 CSV trace file missing header!");
    }

    while (std::getline(in_file, line)) {
        util::StrVector parts = util::split_string(line); 
        error_type = SampleError::NONE;

        try {
            if (parts.size() != kTraceCSVNumFields) {
                trace.push_back(std::make_shared<Sample>(id, index, timestamp, lat, lon, heading, speed, line, false, SampleError::FIELD, "BSMP1 CSV: invalid number of fields"));
                index++;
                
                continue;
            }

            id = parts[0] + "_" + parts[1];

            lat = std::stod(parts[7]);

            if (lat > 80.0 || lat < -84.0) {
                trace.push_back(std::make_shared<Sample>(id, index, timestamp, lat, lon, heading, speed, line, false, SampleError::GEO, "BSMP1 CSV: bad latitude: " + parts[7]));
                index++;
                
                continue;
            }

            lon = std::stod(parts[8]);

            if (lon >= 180.0 || lon <= -180.0) {
                trace.push_back(std::make_shared<Sample>(id, index, timestamp, lat, lon, heading, speed, line, false, SampleError::GEO, "BSMP1 CSV: bad longitude: " + parts[8]));
                index++;
                
                continue;
            }

            if (lat == 0.0 && lon == 0.0) {
                trace.push_back(std::make_shared<Sample>(id, index, timestamp, lat, lon, heading, speed, line, false, SampleError::GEO, "BSMP1 CSV: equator point"));
                index++;
                
                continue;
            }

            heading = std::stod(parts[11]);

            if (heading > 360.0 || heading < 0.0) {
                trace.push_back(std::make_shared<Sample>(id, index, timestamp, lat, lon, heading, speed, line, false, SampleError::HEADING, "BSMP1 CSV: bad heading: " + parts[11]));
                index++;
                
                continue;
            }
        
            gentime = std::stod(parts[3]);
            timestamp = static_cast<long>(1000 * ((gentime / 1000000 - 35) + 1072933200)); 
            speed = std::stod(parts[10]);
        } catch (std::invalid_argument& e) {
            trace.push_back(std::make_shared<Sample>(id, index, timestamp, lat, lon, heading, speed, line, false, SampleError::FIELD, std::string(e.what())));
            index++;
            
            continue;
        }
            
        trace.push_back(std::make_shared<Sample>(id, index, timestamp, lat, lon, heading, speed, line));
        index++;
    }

    in_file.close();

    return trace;
}

void remove_trace_errors(const Sample::Trace& trace, Sample::Trace& out) {
    std::size_t index = 0;    

    for (auto& sample : trace) {
        if (!sample->is_valid()) {
            continue;
        }

        sample->index(index);
        index++;
        out.push_back(sample);
    }
}

IntervalMarker::IntervalMarker( const std::initializer_list<Interval::PtrList> list, uint32_t interval_type ) :
    interval_type_{ interval_type },
    critical_interval{ 0 },
    iptr{ nullptr }
{
    merge_intervals( list );
    set_next_interval();
}

bool IntervalMarker::compare( Interval::Ptr a, Interval::Ptr b )
{
    Index a_left = a->left();
    Index b_left = b->left();

    if (a_left < b_left) 
    {
        return true;
    } 
    else if (a_left == b_left) 
    {
        return a->right() < b->right();
    }

    return false;
}

void IntervalMarker::merge_intervals( const std::initializer_list<Interval::PtrList> list )  
{
    Interval::PtrList sorted_intervals;

    // Combine all the interval lists into one list.
    for (auto& interval_list : list)
    {
        sorted_intervals.insert( sorted_intervals.end(), interval_list.begin(), interval_list.end() );
    }

    // Check there are 1 or less intervals.
    if (sorted_intervals.size() <= 0) {
        // There are no intervals.
        // No need to sort or merge.
        return;
    }

    if (sorted_intervals.size() == 1) {
        // There is one interval.
        // No need to sort or merge.
        intervals.push_back(sorted_intervals[0]);         

        return;
    }

    // There are two or more intervals.
    // Sort the intervals.
    std::sort(sorted_intervals.begin(), sorted_intervals.end(), compare);

    // Try to merge the intervals.

    // Save the first internal state.
    auto first = sorted_intervals.begin();
    Index start = (*first)->left();
    Index end = (*first)->right();
    Interval::AuxSetPtr aux_set_ptr = (*first)->aux_set();
        
    // Go through the rest of the intervals.
    for (auto it = std::next(first, 1); it != sorted_intervals.end(); ++it) 
    {
        Index next_start = (*it)->left();
        Index next_end = (*it)->right();
        Interval::AuxSetPtr next_aux_set_ptr = (*it)->aux_set();

        if (next_start <= end) 
        {
            // This interval starts within the saved interval.
            // Update the aux set with this interval's aux set.
            aux_set_ptr->insert( next_aux_set_ptr->begin(), next_aux_set_ptr->end() ); 

            if (next_end > end)
            {
                // This interval extends past the saved interval.
                // Set the saved end to this interval end.
                end = next_end;
            }
        } 
        else 
        {
            // This interval starts after the saved interval.
            // Add the saved interval to the list and update the 
            // saved interval state.
            intervals.push_back( std::make_shared<Interval>( start, end, aux_set_ptr, interval_type_ ) );

            start = next_start;
            end = next_end;
            aux_set_ptr = next_aux_set_ptr;
        }
    }
    
    // The intevals were exhausted.
    // Add the removing saved interval to the list.
    intervals.push_back( std::make_shared<Interval>( start, end, aux_set_ptr, interval_type_ ) );
}

void IntervalMarker::set_next_interval()
{
    if (critical_interval >= intervals.size()) 
    {   
        iptr = nullptr;

        return;
    }
    
    iptr = intervals[critical_interval++];
}

void IntervalMarker::mark_trace( geo_data::Sample::Trace& trace ) 
{
    for (auto& sample : trace) {
        mark_trip_point( sample );
    }
}

void IntervalMarker::mark_trip_point( geo_data::Sample::Ptr sample ) 
{
    if (!iptr) 
    {
        // There are no more intervals.
        // Nothing can be done for this trip point.
        return;
    }
    
    while (iptr->is_before( sample->index() ))
    {
        // The trip point is after the end of the interval.
        // Find the next interval that contains the trip point.
        set_next_interval();

        if (!iptr)
        {
            // There are no more intervals.
            // Nothing can be done for this trip point.
            return;
        }
    }
        
    // The trip point is before or within the interval.
    // Check if it is within.
    if (iptr->contains( sample->index() )) 
    {
        // The trip point is within the interval.
        // Set critical interval for the trip point.
        sample->interval( iptr );

        return;
    }

    // The trip point is before the interval.
    // Check the next trip point.
}

// meters per second - 36 m/s ~= 80 MPH.
const double KML::MAX_SPEED = 36.0;

KML::KML(std::ostream& stream, const std::string& doc_name, bool visibility ) :
    stream_{ stream }
{
    unsigned int color;

    for (int a = 0; a <= 255; a+=16) {
        color = 0xff0000ff | ( a << 8 );
        colors.push_back(color);
    }

    for (int a = 255; a >= 0; a-=16) {
        color = 0xff00ff00 | a;
        colors.push_back(color);
    }

    stream_ << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n<Document>\n"; 
    stream_ << "<name>" << doc_name << "</name>\n";
    stream_ << "<open>" << visibility << "</open>\n";
    stream_ << std::setprecision(10);

    write_icon_style( "start_style", "http://maps.google.ca/mapfiles/kml/pal4/icon54.png" );
    write_icon_style( "end_style", "http://maps.google.ca/mapfiles/kml/pal4/icon7.png" );
    write_icon_style( "stop_marker_style", "http://maps.google.com/mapfiles/kml/paddle/S.png", 2.0 );
    write_icon_style( "turnaround_marker_style", "http://maps.google.com/mapfiles/kml/paddle/T.png", 2.0 );

    for ( auto& color : colors ) {
        std::string style_name{ "lcolor_" + std::to_string(color) };
        write_line_style( style_name, color, 2 );
    }
}

void KML::finish()
{
    stream_ <<  "</Document>" << std::endl << "</kml>"; 
}

/**
 * Convert a speed into a KML based color that ranges from red (stopped) to
 * green (80 mph or greater). The returned value is in the form: aabbggrr
 * NOTE: The input speed for this function is m/s
 *
 * speed ~ 0 = RED
 * speed ~ 80 = GREEN
 */
unsigned int KML::get_speed_color( double speed )
{
    int n_colors = static_cast<int>(colors.size());
    int i = static_cast<int>(speed / MAX_SPEED * static_cast<double>(n_colors));
    if (i >= n_colors) i = n_colors-1;
    return colors[i];
}

void KML::start_folder( const std::string& name, const std::string& description, const std::string& id, const bool open )
{
    stream_ << "<Folder";
    if ( id.length() > 0 ) {
        stream_ << " id=\"" << id << "\"";
    } 
    stream_ << ">\n";
    stream_ << "<name>" << name << "</name>\n";
    stream_ << "<description>" << description << "</description>\n";
    stream_ << "<visibility>" << std::noboolalpha << open << "</visibility>\n";
    stream_ << "<open>0</open>\n";

}

void KML::stop_folder( )
{
    stream_ << "</Folder>\n";
}

void KML::write_point( const OGRPoint& point, const std::string& style_name )
{
    stream_ << "<Placemark>\n";
    stream_ << "<styleUrl>#" << style_name << "</styleUrl>\n";
    stream_ << "<description>" << style_name << "</description>\n";
    stream_ << "<Point>\n";
    stream_ << "<gx:altitudeMode>clampToGround</gx:altitudeMode>\n";
    stream_ << "<coordinates>" << point.getX() << "," << point.getY() << ",0</coordinates>\n";
    stream_ << "</Point>\n";
    stream_ << "</Placemark>\n";
}

void KML::write_icon_style( const std::string& name, const std::string& href, float scale )
{
    stream_ << "<Style id=\"" << name << "\">\n";
    stream_ << "<IconStyle>\n";
    stream_ << "<Icon>\n";
    stream_ << "<href>" << href << "</href>\n";
    stream_ << "</Icon>\n";
    stream_ << "<hotSpot x=\"0.5\" xunits=\"fraction\" y=\"0.5\" yunits=\"fraction\"/>";
    stream_ << "<scale>" << scale << "</scale>\n";
    stream_ << "</IconStyle>\n";
    stream_ << "</Style>\n";
}

void KML::write_line_style( const std::string& name, unsigned int color_value, int width )
{
    stream_ << "<Style id=\"" << name << "\">\n";
    stream_ << "<LineStyle>\n";
    stream_ << "<color>" << std::hex << color_value << std::dec << "</color>\n";
    stream_ << "<width>" << width << "</width>\n";
    stream_ << "<gx:labelVisibility>" << true << "</gx:labelVisibility>\n";
    stream_ << "</LineStyle>\n";
    stream_ << "</Style>\n";
}

void KML::write_poly_style( const std::string& name, unsigned int color_value, int width, bool fill, bool outline )
{
    stream_ << "<Style id=\"" << name << "\">\n";

    stream_ << "<LineStyle>\n";
    stream_ << "<color>" << std::hex << color_value << std::dec << "</color>\n";
    stream_ << "<width>" << width << "</width>\n";
    stream_ << "</LineStyle>\n";

    stream_ << "<PolyStyle>\n";
    // keep the previous line color and make it transparent.
    color_value = (color_value & 0x00ffffff) | 0x55000000;
    stream_ << "<color>" << std::hex << color_value << std::dec << "</color>\n";
    stream_ << "<colorMode>normal</colorMode>\n";
    stream_ << "<fill>" << fill << "</fill>\n";
    stream_ << "<outline>" << outline << "</outline>\n";
    stream_ << "</PolyStyle>\n";

    stream_ << "</Style>\n";
}

void KML::write_trace( const Sample::Trace& trace, bool de_identify, int stride )
{
    int next, c;
    double speed;
    unsigned int color, previous_color;
    bool is_private;

    int n_trippoints = static_cast<int>(trace.size());

    if (trace.size() == 0) {
        return;
    }

    start_folder( "trajectory-full", "trip point list", "TRAJ", false );
    write_point( trace.front()->point, "start_style" );

    next = 0;

    while (next < n_trippoints) {
        // initial speed color.
        speed = trace[next]->speed();
        previous_color = get_speed_color(speed);

        // Determine is we are removing private trip points.
        is_private = de_identify ? trace[next]->interval() != nullptr : false;

        if (is_private) {
            // This trip point is private.
            // Don't attempt to start a segment.
            next++;
            continue;
        }

        stream_ << "<Placemark>\n";
        // convert to MPH for the label.
        speed *= 2.23694;
        stream_ << "<name>MPH = " << speed << "</name>\n";
        stream_ << "<styleUrl>#lcolor_" << previous_color << "</styleUrl>\n";
        stream_ << "<LineString>\n";
        stream_ << "<coordinates>\n";

        c = 0;
        do {
            is_private = de_identify ? trace[next]->interval() != nullptr : false;
            speed = trace[next]->speed();
            color = get_speed_color(speed);
            stream_ << trace[next]->point.getX() << "," << trace[next]->point.getY() << ",0 ";
            next += stride;
            ++c;
        } while (next < n_trippoints && (c < 2 || color == previous_color));
        
        if (next >= n_trippoints) {
            // write the segment that extends to the last point.
            stream_ << trace.back()->point.getX() << "," << trace.back()->point.getY() << ",0";
        } else {
            // so the next segment starts correctly.
            next -= stride;
        }

        stream_ << "\n</coordinates>\n";
        stream_ << "</LineString>\n";
        stream_ << "</Placemark>\n";

        previous_color = color;
    }

    write_point( trace.back()->point, "end_style" );
    stop_folder( );
}

void KML::write_intervals( const Interval::PtrList& intervals, const Sample::Trace& trace, const std::string& stylename, const std::string& marker_style, int stride )
{
    // for scoping reasons.
    Index i, last;

    start_folder( marker_style, marker_style, "intervals", false );
    // makes the assumption that intervals are not out of trajectory bounds.
    for ( auto& intptr : intervals ) {

        write_point( trace[intptr->left()]->point, marker_style );

        stream_ << "<Placemark>\n";
        stream_ << "<name>" << stylename << "</name>\n";
        stream_ << "<styleUrl>#" << stylename << "</styleUrl>\n";
        stream_ << "<LineString>\n";
        stream_ << "<coordinates>\n";

        last = intptr->right();
        for ( i = intptr->left(); i < last; i+=stride ) {
            stream_ << trace[i]->point.getX() << "," << trace[i]->point.getY() << ",0 ";
        }

        // open on the right.
        --last;

        if (i >= last) {
            // skipped the last point of the interval (one before the spec); write it.
            stream_ << trace[last]->point.getX() << "," << trace[last]->point.getY() << ",0 ";
        } 

        stream_ << "\n</coordinates>\n";
        stream_ << "</LineString>\n";
        stream_ << "</Placemark>\n";
    }

    stop_folder();
}

void KML::write_intervals( const Interval::PtrList& intervals, const Sample::Trace& trace, const std::string& stylename, int stride )
{
    // for scoping reasons.
    Index i, last;

    start_folder( stylename, stylename, "intervals", false );
    // makes the assumption that intervals are not out of trajectory bounds.
    for ( auto& intptr : intervals ) {

        stream_ << "<Placemark>\n";
        stream_ << "<name>" << stylename << "</name>\n";
        stream_ << "<styleUrl>#" << stylename << "</styleUrl>\n";
        stream_ << "<LineString>\n";
        stream_ << "<coordinates>\n";

        last = intptr->right();
        for ( i = intptr->left(); i < last; i+=stride ) {
            stream_ << trace[i]->point.getX() << "," << trace[i]->point.getY() << ",0 ";
        }

        // open on the right.
        --last;

        if (i >= last) {
            // skipped the last point of the interval (one before the spec); write it.
            stream_ << trace[last]->point.getX() << "," << trace[last]->point.getY() << ",0 ";
        } 

        stream_ << "\n</coordinates>\n";
        stream_ << "</LineString>\n";
        stream_ << "</Placemark>\n";
    }

    stop_folder();
}

void KML::write_areas( const std::unordered_set<geo::Area::Ptr>& aptrset, const std::string& stylename ) {
    start_folder( stylename, stylename, "areas", false );

    for (auto& aptr : aptrset) {
        for (auto ring : aptr->rings) {
            stream_ << "<Placemark>\n";

            if (!stylename.empty()) {
                stream_ << "<styleUrl>#" << stylename << "</styleUrl>\n";
            }

            stream_ << "<Polygon>\n";
            stream_ << "<extrude>0</extrude>\n";
            stream_ << "<altitudeMode>clampToGround</altitudeMode>\n";
            stream_ << "<outerBoundaryIs>\n";
            stream_ << "<LinearRing>\n";

            stream_ << "<coordinates>\n";
            
            for (int i = 0; i < ring.getNumPoints(); ++i) {
                stream_ << std::setprecision(16) << ring.getX(i) << "," << ring.getY(i) << ",0" << std::endl;
            }

            stream_ << "</coordinates>\n";

            stream_ << "</LinearRing>\n";
            stream_ << "</outerBoundaryIs>\n";
            stream_ << "</Polygon>\n";

            stream_ << "</Placemark>\n";
        }
    }

    stop_folder();
}

void KML::write_areas( const std::vector<geo::Area::Ptr>& areas, const std::string& stylename ) {
    start_folder( stylename, stylename, "areas", false );

    for (auto& aptr : areas) {
        for (auto ring : aptr->rings) {
            stream_ << "<Placemark>\n";

            if (!stylename.empty()) {
                stream_ << "<styleUrl>#" << stylename << "</styleUrl>\n";
            }

            stream_ << "<Polygon>\n";
            stream_ << "<extrude>0</extrude>\n";
            stream_ << "<altitudeMode>clampToGround</altitudeMode>\n";
            stream_ << "<outerBoundaryIs>\n";
            stream_ << "<LinearRing>\n";

            stream_ << "<coordinates>\n";

            for (int i = 0; i < ring.getNumPoints(); ++i) {
                stream_ << std::setprecision(16) << ring.getX(i) << "," << ring.getY(i) << ",0" << std::endl;
            }
                
            stream_ << "</coordinates>\n";

            stream_ << "</LinearRing>\n";
            stream_ << "</outerBoundaryIs>\n";
            stream_ << "</Polygon>\n";

            stream_ << "</Placemark>\n";
        }
    }

    stop_folder();
}

}
