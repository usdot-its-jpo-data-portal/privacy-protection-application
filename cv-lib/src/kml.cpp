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
#include "kml.hpp"
#include "quad.hpp"
#include <iomanip>

namespace KML {

    // meters per second - 36 m/s ~= 80 MPH.
    const double File::MAX_SPEED = 36.0;

    File::File(std::ostream& stream, const std::string& doc_name, bool visibility ) :
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

    void File::finish()
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
    unsigned int File::get_speed_color( double speed )
    {
        int n_colors = static_cast<int>(colors.size());
        int i = static_cast<int>(speed / MAX_SPEED * static_cast<double>(n_colors));
        if (i >= n_colors) i = n_colors-1;
        return colors[i];
    }

    void File::write_circle(const geo::Circle& circle, const std::string& style, uint32_t n_segments) {
        if (n_segments < 3) {
            throw std::invalid_argument("KML circle must be made up of 3 or more segments!");
        } 

        double arc_len = 360.0 / static_cast<double>(n_segments);
        double curr_degree = 0.0;
        std::stringstream coord_str;
        bool first = true;
        geo::Location::CPtr next_loc, curr_loc, first_loc;

        for (uint32_t i = 0; i < n_segments; ++i) {
            geo::Location next = geo::Location::project_position(circle, curr_degree, circle.radius);
            next_loc = std::make_shared<geo::Location>(next.lat, next.lon);

            if (first) {
                //first_loc = std::make_shard<geo::Location>(next.lat, next.lon);
                first_loc = next_loc;
                first = false;
            } else {
                coord_str << std::setprecision(16) << " " << curr_loc->lon << "," << curr_loc->lat << ",0 " << next_loc->lon << "," << next_loc->lat << ",0";
            }

            //curr_loc = std::make_shard<geo::Location>(next.lat, next.lon);
            curr_loc = next_loc;
            curr_degree += arc_len;
        }
        
        coord_str << std::setprecision(16) << " " << curr_loc->lon << "," << curr_loc->lat << ",0 " << first_loc->lon << "," << first_loc->lat << ",0";

        stream_ << "<Placemark>" << std::endl;
        stream_ << "<name>" << circle.uid << "</name>" << std::endl;
        stream_ << "<styleUrl>#" << style << "</styleUrl>" << std::endl;
        stream_ << "<Polygon>" << std::endl;
        stream_ << "<tessellate>1</tessellate>" << std::endl;
        stream_ << "<gx:altitudeMode>clampToGround</gx:altitudeMode>" << std::endl;
        stream_ << "<outerBoundaryIs>" << std::endl;
        stream_ << "<LinearRing>" << std::endl;
        stream_ << "<coordinates>" << coord_str.str() << "</coordinates>" << std::endl;
        stream_ << "</LinearRing>" << std::endl;
        stream_ << "</outerBoundaryIs>" << std::endl;
        stream_ << "</Polygon>" << std::endl;
        stream_ << "</Placemark>" << std::endl;
    }

    void File::write_bounds(const geo::Bounds& bounds, const std::string& style) {
        std::stringstream coord_str;

        coord_str << std::setprecision(16) << bounds.sw.lon << "," << bounds.sw.lat << ",0 ";
        coord_str << std::setprecision(16) << bounds.se.lon << "," << bounds.se.lat << ",0 ";
        coord_str << std::setprecision(16) << bounds.ne.lon << "," << bounds.ne.lat << ",0 ";
        coord_str << std::setprecision(16) << bounds.nw.lon << "," << bounds.nw.lat << ",0 ";
        coord_str << std::setprecision(16) << bounds.sw.lon << "," << bounds.sw.lat << ",0";
        
        stream_ << "<Placemark>" << std::endl;
        stream_ << "<styleUrl>#" << style << "</styleUrl>" << std::endl;
        stream_ << "<Polygon>" << std::endl;
        stream_ << "<extrude>0</extrude>" << std::endl;
        stream_ << "<altitudeMode>clampToGround</altitudeMode>" << std::endl;
        stream_ << "<outerBoundaryIs>" << std::endl;
        stream_ << "<LinearRing>" << std::endl;
        stream_ << "<coordinates>" << std::endl;
        stream_ << coord_str.str() << std::endl;;
        stream_ << "\n</coordinates>" << std::endl;
        stream_ << "</LinearRing>" << std::endl;
        stream_ << "</outerBoundaryIs>" << std::endl;
        stream_ << "</Polygon>" << std::endl;
        stream_ << "</Placemark>" << std::endl;
    }

    void File::start_folder( const std::string& name, const std::string& description, const std::string& id, const bool open )
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

    void File::stop_folder( )
    {
        stream_ << "</Folder>\n";
    }

    void File::write_point( const geo::Point& point, const std::string& style_name )
    {
        stream_ << "<Placemark>\n";
        stream_ << "<styleUrl>#" << style_name << "</styleUrl>\n";
        stream_ << "<description>" << style_name << "</description>\n";
        stream_ << "<Point>\n";
        stream_ << "<gx:altitudeMode>clampToGround</gx:altitudeMode>\n";
        stream_ << "<coordinates>" << point.lon << "," << point.lat << ",0</coordinates>\n";
        stream_ << "</Point>\n";
        stream_ << "</Placemark>\n";
    }

    void File::write_icon_style( const std::string& name, const std::string& href, float scale )
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

    void File::write_line_style( const std::string& name, unsigned int color_value, int width )
    {
        stream_ << "<Style id=\"" << name << "\">\n";
        stream_ << "<LineStyle>\n";
        stream_ << "<color>" << std::hex << color_value << std::dec << "</color>\n";
        stream_ << "<width>" << width << "</width>\n";
        stream_ << "<gx:labelVisibility>" << true << "</gx:labelVisibility>\n";
        stream_ << "</LineStyle>\n";
        stream_ << "</Style>\n";
    }

    void File::write_poly_style( const std::string& name, unsigned int color_value, int width, bool fill, bool outline )
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

    void File::write_line_string( const std::string& name, const std::string& stylename, const std::vector<geo::Point>& points )
    {
        stream_ << "<Placemark>\n";

        if (!name.empty()) {
            stream_ << "<name>" << name << "</name>\n";
        }

        if (!stylename.empty()) {
            stream_ << "<styleUrl>#" << stylename << "</styleUrl>\n";
        }

        if (points.size() > 1) {
            stream_ << "<LineString>\n";

            stream_ << "<coordinates>\n";
            for (auto& pt : points) {
                stream_ << pt.lon << "," << pt.lat << ",0 ";
            }
            stream_ << "\n</coordinates>\n";
            stream_ << "</LineString>\n";
        }

        stream_ << "</Placemark>\n";
    }

    void File::write_trajectory( const trajectory::Trajectory& traj, bool de_identify, int stride )
    {
        int next, c;
        double speed;
        unsigned int color, previous_color;
        bool is_private;
    
        int n_trippoints = static_cast<int>(traj.size());

        if (traj.size() == 0) {
            return;
        }

        start_folder( "trajectory-full", "trip point list", "TRAJ", false );
        write_point( *(traj.front()), "start_style" );

        next = 0;

        while (next < n_trippoints) {
            // initial speed color.
            speed = traj[next]->get_speed();
            previous_color = get_speed_color(speed);

            // Determine is we are removing private trip points.
            is_private = de_identify ? traj[next]->is_critical() || traj[next]->is_private() : false;

            if (is_private)
            {
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
                is_private = de_identify ? traj[next]->is_critical() || traj[next]->is_private() : false;
                speed = traj[next]->get_speed();
                color = get_speed_color(speed);
                stream_ << traj[next]->lon << "," << traj[next]->lat << ",0 ";
                next += stride;
                ++c;
            } while (!is_private && next < n_trippoints && (c < 2 || color == previous_color));
            
            if (!is_private && next >= n_trippoints) {
                // write the segment that extends to the last point.
                stream_ << traj.back()->lon << "," << traj.back()->lat << ",0";
            } else {
                // so the next segment starts correctly.
                next -= stride;
            }

            stream_ << "\n</coordinates>\n";
            stream_ << "</LineString>\n";
            stream_ << "</Placemark>\n";

            previous_color = color;
        }

        write_point( *(traj.back()), "end_style" );
        stop_folder( );
    }

    void File::write_trajectory_simple( const std::string& name, const std::string& stylename, const trajectory::Trajectory& traj, int stride )
    {
        int n_trippoints = static_cast<int>(traj.size());

        if (traj.size() > 0) {
            start_folder( "trajectory-simple", "trip point list", "TRAJ", false );
            int next = 0;
            write_point( *(traj.front()), "start_style" );

            stream_ << "<Placemark>\n";
            if (!name.empty()) {
                stream_ << "<name>" << name << "</name>\n";
            }

            if (!stylename.empty()) {
                stream_ << "<styleUrl>#" << stylename << "</styleUrl>\n";
            }

            stream_ << "<LineString>\n";
            stream_ << "<coordinates>\n";
            while (next < n_trippoints) {
                stream_ << traj[next]->lon << "," << traj[next]->lat << ",0 ";
                next += stride;
            }
            if (next >= n_trippoints) {
                stream_ << traj.back()->lon << "," << traj.back()->lat << ",0";
            }
            stream_ << "\n</coordinates>\n";
            stream_ << "</LineString>\n";

            stream_ << "</Placemark>\n";
            write_point( *(traj.back()), "end_style" );
            stop_folder( );
        }
    }

    void File::write_intervals( const trajectory::Interval::PtrList& intervals, const trajectory::Trajectory& traj, const std::string& stylename, const std::string& marker_style, int stride )
    {
        // for scoping reasons.
        trajectory::Index i, last;

        start_folder( marker_style, marker_style, "intervals", false );
        // makes the assumption that intervals are not out of trajectory bounds.
        for ( auto& intptr : intervals ) {

            write_point( *(traj[intptr->left()]), marker_style );

            stream_ << "<Placemark>\n";
            stream_ << "<name>" << stylename << "</name>\n";
            stream_ << "<styleUrl>#" << stylename << "</styleUrl>\n";
            stream_ << "<LineString>\n";
            stream_ << "<coordinates>\n";

            last = intptr->right();
            for ( i = intptr->left(); i < last; i+=stride ) {
                stream_ << traj[i]->lon << "," << traj[i]->lat << ",0 ";
            }

            // open on the right.
            --last;

            if (i >= last) {
                // skipped the last point of the interval (one before the spec); write it.
                stream_ << traj[last]->lon << "," << traj[last]->lat << ",0 ";
            } 

            stream_ << "\n</coordinates>\n";
            stream_ << "</LineString>\n";
            stream_ << "</Placemark>\n";
        }
        stop_folder();
    }

    void File::write_intervals( const trajectory::Interval::PtrList& intervals, const trajectory::Trajectory& traj, const std::string& stylename, int stride )
    {
        // for scoping reasons.
        trajectory::Index i, last;

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
                stream_ << traj[i]->lon << "," << traj[i]->lat << ",0 ";
            }

            // open on the right.
            --last;

            if (i >= last) {
                // skipped the last point of the interval (one before the spec); write it.
                stream_ << traj[last]->lon << "," << traj[last]->lat << ",0 ";
            } 

            stream_ << "\n</coordinates>\n";
            stream_ << "</LineString>\n";
            stream_ << "</Placemark>\n";
        }

        stop_folder();
    }

    void File::write_areas( const std::unordered_set<geo::AreaCPtr>& aptrset, const std::string& stylename )
    {
        start_folder( stylename, stylename, "areas", false );

        for (auto& aptr : aptrset) {
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
            stream_ << aptr->get_poly_string() << "\n";
            stream_ << "\n</coordinates>\n";

            stream_ << "</LinearRing>\n";
            stream_ << "</outerBoundaryIs>\n";
            stream_ << "</Polygon>\n";

            stream_ << "</Placemark>\n";
        }

        stop_folder();
    }

    void File::write_areas( const std::vector<geo::AreaCPtr>& areas, const std::string& stylename )
    {
        start_folder( stylename, stylename, "areas", false );

        for (auto& aptr : areas) {
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
            stream_ << aptr->get_poly_string() << "\n";
            stream_ << "\n</coordinates>\n";

            stream_ << "</LinearRing>\n";
            stream_ << "</outerBoundaryIs>\n";
            stream_ << "</Polygon>\n";

            stream_ << "</Placemark>\n";
        }

        stop_folder();
    }
}
