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
#include "geo.hpp"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <queue>
#include <utility>

#include <rapidjson/istreamwrapper.h>
#include <gdal/cpl_conv.h>
#include <GeographicLib/GeodesicLine.hpp>

namespace geo {

unsigned char* geom_bytes_from_hex_string(const std::string& geom_string) {
    unsigned char* geom_bytes = new unsigned char[geom_string.length() - 2]();
    int i, j;

    for (i = 2, j = 0; i < geom_string.length() - 1; i += 2, j++) {
        geom_bytes[j] = kHexMap.at(geom_string[i]) << 4 | kHexMap.at(geom_string[i + 1]); 
    }   

    return geom_bytes;
}


void copy_line_string(const OGRLineString& line_string, bool invert, OGRLineString& out) {
    int n_points = line_string.getNumPoints();
    
    out.setNumPoints(n_points);
    
    if (invert) {
        for (int i = n_points - 1, j = 0; i >= 0; --i) {
            out.setPoint(j++, line_string.getX(i), line_string.getY(i));
        }
    } else {
        for (int i = 0; i < n_points; ++i) {
            out.setPoint(i, line_string.getX(i), line_string.getY(i));
        }
    }
}

Vector::Vector(double x, double y, double z) :
    x(x),
    y(y),
    z(z)
{}

void Vector::add(const Vector& in, Vector& out) const {
    out.x = x + in.x;
    out.y = y + in.y;
    out.z = z + in.z;
}

void Vector::multiply(double scalar, Vector& out) const {
    out.x = scalar * x;
    out.y = scalar * y;
    out.z = scalar * z;
}

void Vector::cross(const Vector& in, Vector& out) const {
   out.x = (in.z * y) - (z * in.y); 
   out.y = (in.x * z) - (x * in.z); 
   out.z = (in.y * x) - (y * in.x); 
}

double Vector::dot(const Vector& in) const {
    return x * in.x + y * in.y + z * in.z;
}
        
void Spatial::intercept(const OGRPoint& point_a, const OGRPoint& point_b, const OGRPoint& point_c, OGRPoint& out) const {
    double s12;
    double azi1;
    double azi2;
    double lat_b2;
    double lat_b2_;
    double lon_b2;
    double lon_b2_; 
    double xb1;
    double yb1;
    Vector va1;
    Vector va2;
    Vector la; 
    Vector lb;
    Vector p0;
    Vector tmp;

    if (doubles_are_equal(point_a.getX(), point_b.getX(), kGPSEpsilon) && doubles_are_equal(point_a.getY(), point_b.getY(), kGPSEpsilon)) {
        out.setX(point_a.getX());
        out.setY(point_a.getY());

        return;
    }

    geod_.Inverse(point_a.getY(), point_a.getX(), point_b.getY(), point_b.getX(), s12, azi1, azi2); 
    GeographicLib::GeodesicLine line(geod_, point_a.getY(), point_a.getX(), azi1);
    line.Position(s12 * 0.5, lat_b2, lon_b2);

    for (int i = 0; i < 10; ++i) {
        gnom_.Forward(lat_b2, lon_b2, point_a.getY(), point_a.getX(), va1.x, va1.y);
        gnom_.Forward(lat_b2, lon_b2, point_b.getY(), point_b.getX(), va2.x, va2.y);
        gnom_.Forward(lat_b2, lon_b2, point_c.getY(), point_c.getX(), xb1, yb1);

        va1.cross(va2, la);
        lb.x = la.y;
        lb.y = -(la.x);
        lb.z = la.x * yb1 - la.y * xb1;
        la.cross(lb, p0);
        p0.multiply(1.0 / p0.z, tmp);
        p0.x = tmp.x;
        p0.y = tmp.y;
        p0.z = tmp.z;

        lat_b2_ = lat_b2;
        lon_b2_ = lon_b2;
        
        gnom_.Reverse(lat_b2_, lon_b2_, p0.x, p0.y, lat_b2, lon_b2);

        if (doubles_are_equal(lon_b2_, lon_b2, kInterceptEpsilon) && doubles_are_equal(lat_b2_, lat_b2, kInterceptEpsilon)) {
            break;
        }
    }

    out.setX(lon_b2);
    out.setY(lat_b2);
}

double Spatial::intercept(const OGRPoint& point_a, const OGRPoint& point_b, const OGRPoint& point_c) const {
    double azi1_i;
    double azi2_i;
    double azi1_b;
    double azi2_b;
    double s12_i;
    double s12_b;
    OGRPoint out;
    
    intercept(point_a, point_b, point_c, out);

    geod_.Inverse(point_a.getY(), point_a.getX(), out.getY(), out.getX(), s12_i, azi1_i, azi2_i);
    geod_.Inverse(point_a.getY(), point_a.getX(), point_b.getY(), point_b.getX(), s12_b, azi1_b, azi2_b);

    return (std::fabs(azi1_i - azi1_b) < 1.0) ? s12_i / s12_b : (-1.0) * s12_i / s12_b;
}

double Spatial::intercept(const OGRLineString& line_string, const OGRPoint& point) const {
    double d = std::numeric_limits<double>::max();
    double s = 0.0;
    double sf = 0.0;
    double ds = 0.0;
    double b_x;
    double b_y;
    double f_;
    double d_;
    OGRPoint point_a;
    OGRPoint point_b;
    OGRPoint interp;

    line_string.getPoint(0, &point_a);

    for (int i = 1; i < line_string.getNumPoints(); ++i) {
        line_string.getPoint(i, &point_b); 
        ds = distance(point_a, point_b);
        f_ = intercept(point_a, point_b, point);
        f_ = (f_ > 1.0) ? 1.0 : (f_ < 0.0) ? 0.0 : f_;
        interpolate(point_a, point_b, f_, interp);
        d_ = distance(point, interp); 
    
        if (d_ < d) {
            sf = (f_ * ds) + s;
            d = d_;
        }

        s = s + ds;
        point_a.setX(point_b.getX());
        point_a.setY(point_b.getY());
    }

    return s == 0.0 ? 0.0 : sf / s;
}

void Spatial::interpolate(const OGRPoint& point_a, const OGRPoint& point_b, double fraction, OGRPoint& out) const {
    double azi1;
    double azi2;
    double s12;
    double lat;
    double lon;

    geod_.Inverse(point_a.getY(), point_a.getX(), point_b.getY(), point_b.getX(), s12, azi1, azi2);
    GeographicLib::GeodesicLine line(geod_, point_a.getY(), point_a.getX(), azi1);
    line.Position(s12 * fraction, lat, lon);

    out.setY(lat);
    out.setX(lon); 
}

bool Spatial::interpolate(const OGRLineString& line_string, double length, double fraction, OGRPoint& out) const {
    double d = length * fraction;
    double s = 0.0;
    double ds = 0.0;
    OGRPoint point_a; 
    OGRPoint point_b; 
    
    if (fraction < 0 + 1E-10) {
        line_string.getPoint(0, &out);

        return true;
    }

    if (fraction > 1 - 1E-10) {
        line_string.getPoint(line_string.getNumPoints() - 1, &out);

        return true;
    }

    line_string.getPoint(0, &point_a);

    for (int i = 1; i < line_string.getNumPoints(); ++i) {
        line_string.getPoint(i, &point_b); 
        ds = distance(point_a, point_b);

        if ((s + ds) >= d) {
            interpolate(point_a, point_b, (d - s) / ds, out);
        
            return true;
        }

        s = s + ds;
        point_a.setX(point_b.getX());
        point_a.setY(point_b.getY());
    }

    return false;
}

bool Spatial::interpolate(const OGRLineString& line_string, double fraction, OGRPoint& out) const {
    return interpolate(line_string, length(line_string), fraction, out);
}

double Spatial::distance(const OGRPoint& a, const OGRPoint& b) const {
    double d = -1.0;

    geod_.Inverse(a.getY(), a.getX(), b.getY(), b.getX(), d); 

    return d;
}

double Spatial::length(const OGRLineString& line_string) const {
    double dist = 0.0;
    int nPoints = line_string.getNumPoints();
    OGRPoint point_a;
    OGRPoint point_b;

    if (nPoints == 0 || nPoints == 1) {
        return 0.0;
    }

    for (int i = 1; i < nPoints; ++i) {
        line_string.getPoint(i, &point_b); 
        line_string.getPoint(i - 1, &point_a); 
        dist += distance(point_a, point_b); 
    }

    return dist;
}

double Spatial::azimuth(const OGRPoint& point_a, const OGRPoint& point_b, double fraction) const {
    double s12;
    double azi1;
    double azi2;
    double azi = 0.0;
    OGRPoint tmp;

    if (fraction < 0 + 1E-10) {
        geod_.Inverse(point_a.getY(), point_a.getX(), point_b.getY(), point_b.getX(), s12, azi1, azi2);
        azi = azi1;
    } else if (fraction > 1 - 1E-10) {
        geod_.Inverse(point_a.getY(), point_a.getX(), point_b.getY(), point_b.getX(), s12, azi1, azi2);
        azi = azi2;
    } else {
        interpolate(point_a, point_b, fraction, tmp);
        geod_.Inverse(point_a.getY(), point_a.getX(), tmp.getY(), tmp.getX(), s12, azi1, azi2);
        azi = azi2;
    }
    
    return azi < 0.0 ? azi + 360.0 : azi;
}

double Spatial::azimuth(const OGRLineString& line_sting, double fraction) const {
    return azimuth(line_sting, length(line_sting), fraction);
}

double Spatial::azimuth(const OGRLineString& line_string, double length, double fraction) const {
    double d = length * fraction;
    double s = 0.0;
    double ds = 0.0;
    OGRPoint point_a; 
    OGRPoint point_b; 

    if (fraction < 0 + 1E-10) {
        line_string.getPoint(0, &point_a);
        line_string.getPoint(1, &point_b);

        return azimuth(point_a, point_b, 0.0);
    }

    if (fraction > 1 - 1E-10) {
        line_string.getPoint(line_string.getNumPoints() - 2, &point_a);
        line_string.getPoint(line_string.getNumPoints() - 1, &point_b);

        return azimuth(point_a, point_b, fraction);
    }
    
    line_string.getPoint(0, &point_a);

    for (int i = 1; i < line_string.getNumPoints(); ++i) {
        line_string.getPoint(i, &point_b); 
        ds = distance(point_a, point_b);

        if ((s + ds) >= d) {
            return azimuth(point_a, point_b, (d - s) / ds);
        }

        s = s + ds;
        point_a.setX(point_b.getX());
        point_a.setY(point_b.getY());
    }

    return std::nan("");
}

void Spatial::envelope_for_radius(const OGRPoint& point, double radius, OGREnvelope& envelope) const {
    double tmp;
    
    point_from_bearing(point.getY(), point.getX(), radius, 0.0, &envelope.MaxY, &tmp);
    point_from_bearing(point.getY(), point.getX(), radius, -180.0, &envelope.MinY, &tmp);
    point_from_bearing(point.getY(), point.getX(), radius, 90.0, &tmp, &envelope.MaxX);
    point_from_bearing(point.getY(), point.getX(), radius, -90.0, &tmp, &envelope.MinX);
}

void Spatial::rect_for_radius(const OGRPoint& point, double radius, CPLRectObj& rect) const {
    double tmp;
    
    point_from_bearing(point.getY(), point.getX(), radius, 0.0, &rect.maxy, &tmp);
    point_from_bearing(point.getY(), point.getX(), radius, -180.0, &rect.miny, &tmp);
    point_from_bearing(point.getY(), point.getX(), radius, 90.0, &tmp, &rect.maxx);
    point_from_bearing(point.getY(), point.getX(), radius, -90.0, &tmp, &rect.minx);
}

void Spatial::point_from_bearing(double start_lat, double start_lon, double distance, double bearing, double *lat_out, double *lon_out) const {
    double latr;
    double lonr;
    double start_latr = to_radians(start_lat);
    double start_lonr = to_radians(start_lon);

    distance /= kEarthRadiusM;
    bearing = to_radians(bearing);
    
    latr = std::asin( std::sin(start_latr) * std::cos(distance) + std::cos(start_latr) * std::sin(distance) * std::cos(bearing));
    lonr = start_lonr + std::atan2( std::sin(bearing) * std::sin(distance) * std::cos(start_latr), std::cos(distance) - std::sin(start_latr) * std::sin(start_latr));
    *lon_out = to_degrees(lonr);
    *lon_out = std::fmod(*lon_out + 540.0, 360.0) - 180.0;
    *lat_out = to_degrees(latr);
}

void Spatial::rect_ring(const OGRPoint& a, const OGRPoint& b, double width, double extension, OGRLinearRing& out) const {
    double half_width;
    double ab_bearing;
    double x_bearing;
    double y_bearing;
    double a_lat, a_lon, b_lat, b_lon;
    double a1_lat, a1_lon, a2_lat, a2_lon, b1_lat, b1_lon, b2_lat, b2_lon;
    int n_points;

    half_width = width / 2.0;
    ab_bearing = azimuth(a, b, 1.0);

    a_lat = a.getY();
    a_lon = a.getX();
    b_lat = b.getY();
    b_lon = b.getX();

    if (extension > 0.0) {
        // Extend the nodes of this edge.
        point_from_bearing(a_lat, a_lon, extension, std::fmod(ab_bearing - 180.0, 360.0), &a_lat, &a_lon);
        point_from_bearing(b_lat, b_lon, extension, ab_bearing, &b_lat, &b_lon);
    }

    // Get the bearing to the area corners.
    x_bearing = std::fmod(ab_bearing - 90.0, 360.0);
    y_bearing = std::fmod(ab_bearing + 90.0, 360.0);
    
    // Get the locations of the corners and return the area.
    point_from_bearing(a_lat, a_lon, half_width, x_bearing, &a1_lat, &a1_lon);
    point_from_bearing(a_lat, a_lon, half_width, y_bearing, &a2_lat, &a2_lon);
    point_from_bearing(b_lat, b_lon, half_width, x_bearing, &b1_lat, &b1_lon);
    point_from_bearing(b_lat, b_lon, half_width, y_bearing, &b2_lat, &b2_lon);

    out.setNumPoints(5);

    out.setPoint(0, a1_lon, a1_lat);
    out.setPoint(1, a2_lon, a2_lat);
    out.setPoint(2, b2_lon, b2_lat);
    out.setPoint(3, b1_lon, b1_lat);
    out.setPoint(4, a1_lon, a1_lat);
}

Road::Road(long gid, long osm_id, long source, long target, double reverse, long class_id, float priority, int maxspeed_forward, int maxspeed_backward, double width, bool is_excluded, const std::string geom_string, bool is_valid, const std::string error_msg) :
    gid_(gid),
    osm_id_(osm_id),
    source_(source),
    target_(target),
    reverse_(reverse),
    class_id_(class_id),
    priority_(priority),
    maxspeed_forward_(maxspeed_forward),
    maxspeed_backward_(maxspeed_backward),
    width_(width),
    is_excluded_(is_excluded),
    geom_string_(geom_string),
    is_valid_(is_valid),
    error_msg_(error_msg)
{
    int n_points = 0;
    bounds.minx = 0.0;
    bounds.maxx = 0.0;
    bounds.miny = 0.0;
    bounds.maxy = 0.0;
    Spatial spatial_;
    length_ = -1.0;

    if (is_valid_) {
        is_oneway_ = reverse_ >= 0.0 ? false : true;
        unsigned char* geom_bytes = geom_bytes_from_hex_string(geom_string_);
        OGRErr err = line_string.importFromWkb(geom_bytes);

        if (err != OGRERR_NONE) {
            is_valid_ = false;
            error_msg_ = "Error converting from WKB string.";
        } else {
            length_ = spatial_.length(line_string);
        }
        
        delete geom_bytes;

        n_points = line_string.getNumPoints();
    } else {
        is_oneway_ = false;
        length_ = -1;
    }

    if (n_points == 1) {
        is_valid_ = false;
        error_msg_ = "Single point road.";

        return;
    }

    if (n_points == 0) {
        is_valid_ = false;
        error_msg_ = "Empty road.";

        return;
    }

    bounds.maxx = line_string.getX(0);
    bounds.minx = line_string.getX(0);
    bounds.maxy = line_string.getY(0);
    bounds.miny = line_string.getY(0);

    for (int i = 1; i < n_points; ++i) {
        if (line_string.getX(i) > bounds.maxx) {
            bounds.maxx = line_string.getX(i);
        }

        if (line_string.getX(i) < bounds.minx) {
            bounds.minx = line_string.getX(i);
        }

        if (line_string.getY(i) > bounds.maxy) {
            bounds.maxy = line_string.getY(i);
        }

        if (line_string.getY(i) < bounds.miny) {
            bounds.miny = line_string.getY(i);
        }
    }
}

long Road::id() const {
    return gid_;
}

long Road::osm_id() const {
    return osm_id_;
}

long Road::source() const {
    return source_;
}

long Road::target() const {
    return target_;
}

long Road::type() const {
    return class_id_;
}

float Road::priority() const {
    return priority_;
}

bool Road::is_oneway() const {
    return is_oneway_;
}

bool Road::is_valid() const {
    return is_valid_;
}

long Road::maxspeed_forward() const {
    return maxspeed_forward_;
}

long Road::maxspeed_backward() const {
    return maxspeed_backward_;
}

double Road::length() const {
    return length_;
}

double Road::width() const {
    return width_;
}

bool Road::is_excluded() const {
    return is_excluded_;
}

const std::string Road::error_msg() const {
    return error_msg_;
}

std::ostream& operator<< (std::ostream& os, const Road& road) {
    os << road.gid_ << ",";
    os << road.source_ << ",";
    os << road.target_ << ",";
    os << road.osm_id_ << ",";
    os << road.reverse_ << ",";
    os << road.class_id_ << ",";
    os << road.priority_ << ",";
    os << road.maxspeed_forward_ << ",";
    os << road.maxspeed_backward_ << ",";
    os << road.width_ << ",";
    os << road.is_excluded_ << ",";
    os << road.geom_string_ << ",";
    os << road.is_valid_ << ",";
    os << road.error_msg_;
}

Edge::Edge(Road::Ptr road_ptr, Heading heading) :
    road_ptr_(road_ptr),
    heading_(heading),
    neighbor_(nullptr),
    successor_(nullptr)
{
    if (!road_ptr_) {
        throw std::invalid_argument("Base road for edge not defined.");
    }

    if (!road_ptr_->is_valid()) {
        throw std::invalid_argument("Base road for edge not valid: " + road_ptr_->error_msg());
    }

    id_ = heading == Heading::FORWARD ? road_ptr_->id() * 2 : road_ptr_->id() * 2 + 1;  
    source_ = heading == Heading::FORWARD ? road_ptr_->source() : road_ptr_->target();
    target_ = heading == Heading::FORWARD ? road_ptr_->target() : road_ptr_->source();
    type_ = road_ptr_->type();
    priority_ = road_ptr_->priority();
    maxspeed_ = heading == Heading::FORWARD ? road_ptr_->maxspeed_forward() : road_ptr_->maxspeed_backward();
    length_ = road_ptr_->length();
    width_ = road_ptr_->width();
    copy_line_string(road_ptr_->line_string, heading != Heading::FORWARD, line_string);
}

Edge::Edge(long id) :
    road_ptr_(nullptr),
    heading_(Heading::FORWARD),
    id_(id),
    source_(-1),
    target_(-1),
    type_(-1),
    priority_(0.0),
    maxspeed_(0),
    length_(0.0),
    width_(0.0),
    neighbor_(nullptr),
    successor_(nullptr)
{
}

long Edge::id() const {
    return id_;
}

long Edge::target() const {
    return target_;
}

long Edge::source() const {
    return source_;
}

void Edge::neighbor(Edge::Ptr neighbor) {
    neighbor_ = neighbor;
} 

void Edge::successor(Edge::Ptr successor) {
    successor_ = successor;
}

double Edge::length() const {
    return length_;
}

float Edge::priority() const {
    return priority_;
}

double Edge::width() const {
    return width_;
}

int Edge::maxspeed() const {
    return maxspeed_;
}

Heading Edge::heading() const {
    return heading_;
}

Edge::Ptr Edge::successor() const {
    return successor_;
}

Edge::Ptr Edge::neighbor() const {
    return neighbor_;
}

Road::Ptr Edge::road() const {
    return road_ptr_;
}

long Edge::type() const {
    return type_;
}

const EdgeList split_road(Road::Ptr road_ptr) {
    EdgeList split;

    split.push_back(std::make_shared<Edge>(road_ptr, Heading::FORWARD));   

    if (!road_ptr->is_oneway()) {
        split.push_back(std::make_shared<Edge>(road_ptr, Heading::BACKWARD));
    }

    return split;
}

Area::Area(const OGRLineString& line_string, double width, double extension) :
    width_(width),
    extension_(extension)    
{
    geo::Spatial spatial;

    int n_points = line_string.getNumPoints();
    OGRPoint a;
    OGRPoint b;

    is_valid_ = true;

    if (n_points < 2 || width <= 0.0 || extension < 0.0) {
        is_valid_ = false;
        
        return;
    }

    for (int i = 0; i < n_points - 1; ++i) {
        a.setX(line_string.getX(i));
        a.setY(line_string.getY(i));
        b.setX(line_string.getX(i + 1));
        b.setY(line_string.getY(i + 1));
    
        rings.emplace_back();
        spatial.rect_ring(a, b, width, extension, rings[i]);
    }
}

double Area::width() const {
    return width_;
}

double Area::extension() const {
    return extension_;
}

bool Area::is_valid() const {
    return is_valid_;
}

bool Area::is_within(const OGRPoint& point) const {
    for (auto ring : rings) {
        if (ring.isPointInRing(&point)) {
            return true;
        }
    }

    return false;
}
}
