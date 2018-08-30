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
#ifndef GEO_HPP
#define GEO_HPP

#include <cstdint>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

#include <gdal/ogr_geometry.h>
#include <gdal/cpl_port.h>
#include <gdal/cpl_quad_tree.h>
#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/Constants.hpp>
#include <GeographicLib/Gnomonic.hpp>

/**
 * \brief Namespace for geospatial and geodesic tool functions.
 */
namespace geo {

/**
 * \brief Hex map used to decode strings into hex bytes.
 */
const std::unordered_map<char, unsigned char> kHexMap({{'0',0x0}, {'1',0x1}, {'2',0x2}, {'3',0x3}, {'4',0x4}, {'5',0x5}, {'6',0x6}, {'7',0x7}, {'8',0x8}, {'9',0x9}, {'A',0xa}, {'a',0xa}, {'b',0xb}, {'B',0xb}, {'c',0xc}, {'C',0xc}, {'d',0xd}, {'D',0xd}, {'e',0xe}, {'E',0xe}, {'f',0xf}, {'F',0xf}});
constexpr double kNAN = std::nan("");

/**
 * \brief Heading types for directed Edges.
 */
enum class Heading {
    FORWARD = 0,
    BACKWARD = 1
};

/**
 * \brief Copy a OGRLineString to another line string. Allows for reversal (inversion) of the string.
 * 
 * \param line_string the OGRLineString to copy
 * \param invert option to reverse the order of points for the new line
 * \param out the output OGRLineString
 */
void copy_line_string(const OGRLineString& line_string, bool invert, OGRLineString& out);

/** 
 * \brief Simple 3 dimensional (x,y,z) vector class used to assist in geospatial and geodesic computations.
 */
struct Vector {
        /**
         * \brief Construct a vector. Will construct the unit vector if no parameters are given.
         * 
         * \param x the x value of the vector
         * \param y the y value of the vector
         * \param z the z value of the vector
         */
        Vector(double x = 1.0, double y = 1.0, double z = 1.0);
    
        /**
         * \brief Add this vector with another vector.
         *
         * \param in the vector to add to this vector 
         * \param out the vector to store the result of the addition
         */
        void add(const Vector& in, Vector& out) const;
    
        /**
         * \brief Multiply this vector with a scalar. 
         * 
         * \param scalar the scalar value to multiply this vector by
         * \param out the vector to store the result of the multiplication
         */
        void multiply(double scalar, Vector& out) const;

        /**
         * \brief Compute the cross product of this vector with another vector.
         * 
         * \param in the vector for RHS of this cross product
         * \param out the vector to store the result of the cross product
         */
        void cross(const Vector& in, Vector& out) const;

        /**
         * \brief Compute the dot product of this vector with another vector.
         * 
         * \param in the vector to compute the dot product with
         * \return the computed dot product
         */
        double dot(const Vector& in) const;

        /** \brief The x value of the vector */ 
        double x;
        /** \brief The y value of the vector */ 
        double y;
        /** \brief The z value of the vector */ 
        double z;
};

/** 
 * \brief Spatial operator class.
 */
class Spatial {
    public:
        /** \brief Standard PI value used in degrees to radians conversions. */
        static constexpr double kPi = 3.14159265358979323846;

        /** \brief GPS epsilon for comparing decimal GPS coordinates. */
        static constexpr double kGPSEpsilon = std::numeric_limits<double>::epsilon() * 100.0;

        /** \brief The epsilon for comparing geospatial intercepts. */
        static constexpr double kInterceptEpsilon = std::sqrt(std::numeric_limits<double>::epsilon()) * 0.01;

        /** \brief The Earth's radius in meters. */
        static constexpr double kEarthRadiusM = 6378137.0;

        /** \brief The precision used to round double values. */
        static constexpr double kPrecision = 1E-8;

        /** 
         * \brief Uses epsilon values to compare double precision equality. 
         * 
         * \param a the first double
         * \param b the second double
         * \param epsilon the epsilon used for the equality comparison
         * \return true if the absolute value difference between the doubles is less than the epsilon, false otherwise
         */
        static inline bool doubles_are_equal(double a, double b, double epsilon) { return std::fabs(a - b) < epsilon; }

        /** 
         * \brief Convert decimal degrees to radians. 
         * 
         * \param degrees the decimal degree value to convert
         * \return the converted value in radians
         */
        static inline double to_radians(double degrees) { return degrees * kPi / 180.0; }

        /** 
         * \brief Convert radians to decimal degrees. 
         * 
         * \param radians the radians value to convert
         * \return the converted value in degrees
         */
        static inline double to_degrees(double radians) { return radians * 180.0 / kPi; }

        /** 
         * \brief Round to a double precision number to kPrecision.
         * 
         * \param value the double value to round
         * \return the rounded value
         */
        static inline double round(double value) { return std::round(value / kPrecision) * kPrecision ; }  

        /** 
         * \brief Compute the difference between two heading values in degrees.
         * 
         * \param a the first heading
         * \param b the second heading
         * \return the difference between the two headings in degrees
         */
        static inline double heading_delta(double a, double b) { double delta = std::abs(a - b); return (delta < 180.0 ? delta : 360.0 - delta); } 

        /** 
         * \brief Compute the geospatial intercept of point and a line string. The intercept value represents the point's interpolation within the interval [0,1]. If the return value is not within this interval then the point does not intercept the line. A value of 0 means the point is on one end of the line and a value of 1 means the point is on the other end.
         * 
         * \param line_string the line string
         * \param point the point
         * \return the intercept value
         */
        double intercept(const OGRLineString& line_string, const OGRPoint& point) const;

        /**
         * \brief Compute the geospatial intercept of a line formed by points A and B with point C. The intercept value represents point C's interpolation within the interval [0,1]. If the return value is not within this interval then the point does not intercept the line segment. A value of 0 means C is equal to A and a value of 1 means the C is equal to B.
         * 
         * \param point_a the first point of the line
         * \param point_b the second point of the line
         * \param point_c the third point, not on the line
         * \return the intercept value
         */
        double intercept(const OGRPoint& point_a, const OGRPoint& point_b, const OGRPoint& point_c) const;

        /**
         * \brief Compute the geospatial intercept point of a line formed by points A and B with point C. The intercept point represents the original point's interpolated position on the line. The intercept point will be either A or B if the point C does not intercept the line segment (i.e. interpolated as an end point)
         * 
         * \param point_a the first point of the line
         * \param point_b the second point of the line
         * \param point_c the third point, not on the line
         * \param out the intercept point of point_c the line
         */
        void intercept(const OGRPoint& point_a, const OGRPoint& point_b, const OGRPoint& point_c, OGRPoint& out) const;

        /**
         * \brief Get the point on a line given by points A and B and a fraction. If the fraction is less than 0 then point A is returned and if the fraction is greater than 1 then point B is returned.
         * 
         * \param point_a the first point of the line
         * \param point_b the second point of the line
         * \param fraction the fraction representing the position of the point on the line
         * \param out the interpolated point between point_a and point_b
         */
        void interpolate(const OGRPoint& point_a, const OGRPoint& point_b, double fraction, OGRPoint& out) const;

        /**
         * \brief Get the point on a line string of given length and fraction. If the fraction is less than 0 then the first point of the line string is returned and if the fraction is greater than 1 then the last point of the line string is returned.
         * 
         * \param line_string the line string
         * \param length the length of the line string to use
         * \param fraction the fraction representing the position of the point on the line, within the given length
         * \param out the interpolated point on line_string
         */
        bool interpolate(const OGRLineString& line_string, double length, double fraction, OGRPoint& out) const;

        /**
         * \brief Get the point on a line string of given fraction. If the fraction is less than 0 then the first point of the line string is returned and if the fraction is greater than 1 then the last point of the line string is returned.
         * 
         * \param line_string the line string
         * \param fraction the fraction representing the position of the point on the line, within the given length
         * \param out the interpolated point
         */
        bool interpolate(const OGRLineString& line_string, double fraction, OGRPoint& out) const;

        /**
         * \brief Get the distance in meters between two points.
         * 
         * \param point_a the first point
         * \param point_a the second point
         * \return the distance between point_a and point_b
         */
        double distance(const OGRPoint& point_a, const OGRPoint& point_b) const;

        /**
         * Get the length in meters of a line string.
         * 
         * \param line_string the line string
         * \return the length of line_string
         */
        double length(const OGRLineString& line_string) const;
        
        /**
         * Make an OGREnvelope bounding box for a point and given radius.
         * 
         * \param point the center of the bounding box 
         * \param radius the half-width of the box in meters
         * \param envelope the resulting bounding box as an OGREnvelope
         */
        void envelope_for_radius(const OGRPoint& point, double radius, OGREnvelope& envelope) const;

        /**
         * Make a CPLRectObj bounding box for a point and given radius.
         * 
         * \param point the center of the bounding box
         * \param radius the half-width of the box in meters
         * \param envelope the resulting bounding box as a CPLRectObj
         */
        void rect_for_radius(const OGRPoint& point, double radius, CPLRectObj& rect) const;

        /**
         * Find a new point from start position, given a bearing and distance.
         * 
         * \param start_lat the latitude of the starting position
         * \param start_lon the longitude of the starting position
         * \param distance the distance between the new point and starting position
         * \param bearing the bearing from the starting position and new point
         * \param lat_out pointer to the new points latitude
         * \param lon_out pointer to the new point longitude
         */
        void point_from_bearing(double start_lat, double start_lon, double distance, double bearing, double *lat_out, double *lon_out) const;

        /**
         * Compute the azimuth (bearing or heading) as degrees from north between two points A and B within the given fraction. If the fraction is less than 0 then the azimuth between A and B is returned and if the fraction is greater than 1 then the azimuth between B and A is returned.
         * 
         * \param point_a the first point
         * \param point_b the second point
         * \param fraction the fraction in the line formed by point_a and point_b within the interval [0,1]
         * \return the azimuth in degrees from north between point_a and point_b
         */
        double azimuth(const OGRPoint& point_a, const OGRPoint& point_b, double fraction) const;

        /**
         * Compute the azimuth (bearing or heading) as degrees from north for a line string with the given length and fraction. If the fraction is less than 0 then the azimuth between the first two points of the line string is returned. If the fraction is greater than 1 then the azimuth between the last two points is returned.
         * 
         * \param line_string the line string 
         * \param fraction the fraction on the line string within the interval [0,1]
         * \param length the length of the line string to use
         * \return the azimuth in degrees from north of the line string
         */
        double azimuth(const OGRLineString& line_string, double length, double fraction) const;

        /**
         * Compute the azimuth (bearing or heading) as degrees from north for a line string with the given fraction. If the fraction is less than 0 then the azimuth between the first two points of the line string is returned. If the fraction is greater than 1 then the azimuth between the last two points is returned.
         * 
         * \param line_string the line string 
         * \param fraction the fraction on the line string within the interval [0,1]
         * \return the azimuth in degrees from north of the line string
         */
        double azimuth(const OGRLineString& line_string, double fraction) const;

        /**
         * Create a rectangle about two points A and B as an OGRLinearRing. The rectangle will have a width equal to the given width and length equal to the distance between A and B plus the given extension.
         * 
         * \param a the first point
         * \param b the second point
         * \param width the width of rectangle in meters
         * \param extension the length extension of the rectangle in meters
         * \param out the OGRLinearRing representing the newly created rectangle
         */
        void rect_ring(const OGRPoint& a, const OGRPoint& b, double width, double extension, OGRLinearRing& out) const;

    private:
        const GeographicLib::Geodesic geod_ = GeographicLib::Geodesic::WGS84();
        const GeographicLib::Gnomonic gnom_;
};

/** 
 * \brief Road object containing a basic line string and meta data associated with the underlying real world road. A road can be one or two way and is used in spatial indexing. A road should not be confused with an edge used in the graph topology.
 */
class Road {
    public:
        using Ptr = std::shared_ptr<Road>;

        /**
         * \brief Construct a road. Most fields are copied to the new object. The geometric string is used to build the underlying line string. This string should be a valid WKB representation of a line string, otherwise the road will be set to invalid. This also constructs a bounds for this road so that it can be spatially indexed (e.g. by a quad tree).
         * 
         * \param gid the unique GID of the road
         * \param osm_id the OSM way ID of the road
         * \param source the ID of the entry vertex to this road
         * \param target the ID of the exit vertex road to this road
         * \param reverse direction of the road (one way if negative)
         * \param class_id the OSM way type of the road
         * \param priority the priority of the road used to compute the cost of traversing this road
         * \param maxspeed_forward the maximum forward speed limit for this road as kilometers per hour
         * \param maxspeed_backward the maximum backward speed limit for this road as kilometers per hour
         * \param width the approximate width of the this road in meters
         * \param is_excluded is this road type should be excluded from the graph topology
         * \param geom_string the WKB hex string representing the line string of this road
         * \param is_valid indicates the road has a valid line string
         * \param error_msg error message about the line string creation
         */
        Road(long gid, long osm_id, long source, long target, double reverse, long class_id, float priority, int maxspeed_forward, int maxspeed_backward, double width, bool is_excluded, const std::string geom_string, bool is_valid = true, const std::string error_msg = "");

        /**
         * \brief Get the unique ID of this road.
         * 
         * \return the unique ID
         */
        long id() const;

        /**
         * \brief Get the OSM way ID of this road.
         * 
         * \return the OSM way ID
         */
        long osm_id() const;
    
        /**
         * \brief Get the ID of the source vertex of this road.
         *  
         * \return the ID of the source vertex
         */
        long source() const;

        /**
         * \brief Get the ID of the target vertex of this road.
         * 
         * \return the ID of the target vertex
         */
        long target() const;

        /**
         * \brief Get the OSM way type of this road.
         * 
         * \return the OSM way type
         */
        long type() const;

        /**
         * \brief Get the maximum forward speed for the road as kilometers per hour.
         * 
         * \return the maximum forward speed as kilometers per hour
         */
        long maxspeed_forward() const;

        /**
         * \brief Get the maximum backward speed for the road as kilometers per hour.
         * 
         * \return the maximum backward speed as kilometers per hour
         */
        long maxspeed_backward() const;

        /**
         * \brief Get the length of this road in meters.
         * 
         * \return the length in meters
         */
        double length() const;

        /**
         * \brief Get the use priority for this road.
         * 
         * \return the use priority 
         */
        float priority() const;
    
        /**
         * \brief Get the approximate width in meters of this road.
         * 
         * \return the approximate width in meters 
         */
        double width() const;
        
        /**
         * \brief Return true if this road should be excluded from the graph topology. 
         * 
         * \return true if excluded, false otherwise
         */
        bool is_excluded() const;

        /**
         * \brief Return true if this road is a one way road
         *  
         * \return true if one way, false otherwise
         */
        bool is_oneway() const;
    
        /**
         * \brief Return true if this road has a valid line string.
         * 
         * \return true if the line string is valid, false otherwise
         */
        bool is_valid() const;

        /**
         * \brief Get the error message for invalid road line strings.
         * 
         * \return the error message for the line string
         */
        const std::string error_msg() const;

        /** \brief The underlying line string of this road as an OGRLineString. */
        OGRLineString line_string;

        /** \brief The underlying bounds of this road as a CPLRectObj. */
        CPLRectObj bounds;

        /**
         * \brief Write this Road to a stream formatted for CSV output.
         * 
         * \param os the stream object where the point will be written
         * \param road the road to write
         * \return returns the given stream object
         */
        friend std::ostream& operator<< (std::ostream& os, const Road& road);
    private:
        long gid_;
        long osm_id_;
        long source_;
        long target_;
        double reverse_;
        long class_id_;
        float priority_;
        int maxspeed_forward_;
        int maxspeed_backward_;
        const std::string geom_string_;
        bool is_valid_;
        std::string error_msg_;
        bool is_oneway_;
        bool is_excluded_; 
        double length_;
        double width_;
};

/** 
 * \brief Edge class containing a basic line string and meta data associated with the underlying real world road. Edge objects are similar to Road objects except that they are one-directional and used in the graph topology. Because and edge is part of a topology, each edge will point to a successor edge and possible a neighbor edge. 
 * 
 */
class Edge {
    public:
        using Ptr = std::shared_ptr<Edge>;
    
        /**
         * \brief Construct an Edge object from a Road object pointer, and heading of the Edge.
         * 
         * \param road_ptr Pointer to Road object
         * \param heading The Heading value for the edge
         */
        Edge(Road::Ptr road_ptr, Heading heading);

        /**
         * \brief Construct an abstract Edge. This will create an empty line string, which can be used to build edges that don't represent real world roads.
         */
        Edge(long id);

        /**
         * \brief Get the unique ID of this edge.
         * 
         * \return the unique ID
         */
        long id() const;

        /**
         * \brief Get the ID of the source vertex of this edge.
         *  
         * \return the source of the vertex ID
         */
        long source() const;

        /**
         * \brief Get the ID of the target vertex of this edge.
         * 
         * \return the ID of the vertex ID
         */
        long target() const;

        /**
         * \brief Get the length of this edge in meters.
         * 
         * \return the length in meters
         */
        double length() const;

        /**
         * \brief Get the approximate width in meters of this edge.
         * 
         * \return the approximate width in meters 
         */
        double width() const;

        /**
         * \brief Get the use priority for this edge.
         * 
         * \return the use priority 
         */
        float priority() const;

        /**
         * \brief Get the maximum speed for this edge as kilometers per hour.
         * 
         * \return the maximum speed as kilometers per hour
         */
        int maxspeed() const;

        /**
         * \brief Get the heading (forward or backward) of this edge.
         * 
         * \return the heading (forward or backward)
         */
        Heading heading() const;

        /**
         * \brief Get the pointer to the successor edge for this edge.
         * 
         * \return the successor edge
         */
        Ptr successor() const;

        /**
         * \brief Get the pointer to the neighbor edge for this edge.
         * 
         * \return the neighbor edge
         */
        Ptr neighbor() const;

        /**
         * \brief Set the pointer to the successor edge for this edge
         *  
         * \param the successor edge
         */
        void successor(Ptr successor);

        /**
         * \brief Set the pointer to the neighbor edge for this edge
         *  
         * \param the neighbor edge
         */
        void neighbor(Ptr neighbor); 

        /**
         * \brief Get the pointer the base Road object for this edge.
         */
        Road::Ptr road() const;

        /**
         * \brief Get the OSM way type of this edge.
         * 
         * \return the OSM way type
         */
        long type() const;
        
        /** \brief The underlying line string of this edge as an OGRLineString. */
        OGRLineString line_string;
    private:
        Road::Ptr road_ptr_;
        Heading heading_;
        long id_;
        long source_;
        long target_;
        long type_;
        float priority_;
        int maxspeed_;
        double length_;
        double width_;
        Ptr neighbor_;
        Ptr successor_;
};

using EdgeList = std::vector<Edge::Ptr>;
using EdgeListPtr = std::shared_ptr<EdgeList>;
using EdgeMap = std::unordered_map<long, Edge::Ptr>;
using EdgeListMap = std::unordered_map<long, EdgeListPtr>;


/**
 * \brief Split a Road object into edges. If the road is two-way this will return two edges for each direction. Otherwise it will return one edge.
 * 
 * \param road_ptr pointer to the Road object
 * 
 * \return a list of edge pointers
 */
const EdgeList split_road(Road::Ptr road_ptr); 

/** 
 * \brief An Area object is a set of rectangular bounds used to check for geospatial point inclusion. Areas are constructed from line strings, where each segment has it's own rectangular linear ring. Areas are invalid if the line string has less than 2 points, the width is less than or equal to zero, or the extension is less than zero.
 */
class Area {
    public:
        using Ptr = std::shared_ptr<Area>;
        
        /** 
         * \brief Construct an Area object from a line string with the given width and extension. The width and extension are applied to each rectangular, linear ring (line segment).
         * 
         * \param line_string the line string from which to construct the area
         * \param width the width of each linear ring in meters
         * \param extension the length extension of each linear ring in meters
         */
        Area(const OGRLineString& line_string, double width, double extension = 0.0);   
        
        /**
         * \brief Get the width of the area in meters. This is the width of each linear ring.
         * 
         * \return the width in meters
         */
        double width() const;
        
        /**
         * \brief Get the extension of the area in meters. This is the length extension of each linear ring.
         * 
         * \return the extension in meters
         */
        double extension() const;
        
        /**
         * \brief Return true if the area is valid. Valid areas have at least one linear ring, a non-zero width, and a non-negative extension.
         * 
         * \return true if valid, otherwise false
         */
        bool is_valid() const;
    
        /**
         * \brief Return true if the given point is within the area. The point is within the area if it is within any of the area's linear rings.
         * 
         * \param point the point to check for inclusion
         * \return true if the point is included, false otherwise
         */
        bool is_within(const OGRPoint& point) const;

        /** \brief The std::vector of OGRLinearRing used to test for point inclusion. */
        std::vector<OGRLinearRing> rings;
    private:
        double width_;
        double extension_;
        bool is_valid_;
};

}

#endif
