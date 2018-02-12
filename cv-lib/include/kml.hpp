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
#ifndef CTES_KML_HPP
#define CTES_KML_HPP

#include "entity.hpp"
#include "trajectory.hpp"
#include "mapfit.hpp"

#include <string>
#include <vector>
#include <fstream>

namespace KML {

    /**
     * KML Color Values are specified as follows:
     * color_value is easiest to express in hex: 0xaabbggrr
     * where aa -> is the transparency alpha. 00 is transparent; ff is fully opaque.
     * where bb -> BLUE
     * where gg -> GREEN
     * where rr -> RED
     * This is backwards from how it is usually specified, i.e., RGB.
     */

    /**
     * \brief A KML file.
     */
    class File
    {
        public:
            using StreamPtr = std::shared_ptr<std::ostream>;
            static const double MAX_SPEED;
        
            /**
             * \brief Construct a KML file.
             *
             * \param stream the output stream to write the KML.
             * \param doc_name the name of the KML document
             * \param visibility when true elements will be rendered when the KML loads.
             */
            File(std::ostream& stream, const std::string& doc_name, bool visibility = true);

            /**
             * \brief Finalize the KML file (write the ending tags).
             */
            void finish();

            /**
             * \brief Write a style for a line to the KML output stream.
             *
             * \param name The style ID.
             * \param color_value the KML color value as an integer (will be converted to hex).
             * \param width the width of the line in pixels.
             */
            void write_line_style( const std::string& name, unsigned int color_value, int width );

            /**
             * \brief Write a style for an icon to the KML output stream.
             *
             * \param name The style ID
             * \param href The URL for the icon graphic.
             * \param scale The size adjustment of the icon relative to its original size.
             */
            void write_icon_style( const std::string& name, const std::string& href, float scale = 1.0 );

            /**
             * \brief Write a style for a polygon to the KML output stream.
             *
             * \param name The style ID.
             * \param color_value the KML color value as an integer (will be converted to hex).
             * \param width the width of the line in pixels.
             * \param fill true indicates the polygon should be color filled.
             * \param outline true indicates the polygon should be outlined.
             */
            void write_poly_style( const std::string& name, unsigned int color_value, int width = 2, bool fill = true, bool outline = true );

            /**
             * \brief Write a polygon figure to the KML output stream that looks like the circle geometry provided. The
             * circle is defined by a center point and a radius.
             *
             * \param circle the circle instance that will be the basis for the KML polygon.
             * \param The ID of the style to use for the polygon.
             * \param The number of lines to use for the outline of the circle.
             */
            void write_circle(const geo::Circle& circle, const std::string& style, uint32_t n_segments = 50);

            /**
             * \brief Write a rectangle (a polygon) figure to the KML output stream based on the bounds instance.
             *
             * \param bounds the rectangle to use as the basis for the KML polygon.
             * \param style the ID of the style to use for the KML figure.
             */
            void write_bounds(const geo::Bounds& bounds, const std::string& style);

            /**
             * \brief Write a single point to the KML output stream using the point instance.
             *
             * \param point the point to represent in KML.
             * \param style_name the ID of the style for the point.
             */
            void write_point( const geo::Point& point, const std::string& style_name );

            /**
             * \brief Write a list of points as a segmented line to the KML output stream.
             *
             * \param name the name of the segmented line (shown with rollover).
             * \param stylename the ID of the style to use for the line.
             * \param points a vector of points to use as the beginning / end points of the segmented line (first point
             * in the list is first point, last point in the list is last point).
             */
            void write_line_string( const std::string& name, const std::string& stylename, const std::vector<geo::Point>& points );

            /**
             * \brief Write a simple rendering of a trajectory to the KML output stream. The simple rendering has a 
             * single style across all segments and does not exclude any portion of the trajectory.
             *
             * \param name the name of the segmented line that represents the trajectory.
             * \param stylename the ID of the style to use for the line.
             * \param traj The trajectory to base the line on.
             * \param stride The number of trajectory points to skip while rendering the line as a sequence of segments.
             */
            void write_trajectory_simple( const std::string& name, const std::string& stylename, const trajectory::Trajectory& traj, int stride = 20 );

            /**
             * \brief Write a rendering of a trajectory to the KML output stream considering speed and optionally special intervals. 
             * The rendering colors points based on the speed of the traveler and privacy interval point and cricial
             * interval points can be excluded.
             *
             * \param traj The trajectory to base the line on.
             * \param de_identify flag to remove the privacy and critical intervals.
             * \param stride The number of trajectory points to skip while rendering the line as a sequence of segments.
             */
            void write_trajectory( const trajectory::Trajectory& traj, bool de_identify = false, int stride = 20 );

            /**
             * \brief Write the critical and privacy interavls to the KML output stream.
             *
             * The intervals contain indexes into the provided trajectory.
             *
             * \param intervals the list of intervals to render in the KML.
             * \param traj the trajectory that generated the intervals.
             * \param stylename the style ID for the interval trip points.
             * \param marker_style the style ID to use for the first point in the interval (highlights the interval type
             * or what triggered it).
             * \param stride the number of trip points to skip to minimize the amount of data rendered.
             */
            void write_intervals( const trajectory::Interval::PtrList& intervals, const trajectory::Trajectory& traj, const std::string& stylename, const std::string& marker_style, int stride = 10 );

            /**
             * \brief Write the critical and privacy interavls to the KML output stream.
             *
             * The intervals contain indexes into the provided trajectory. This method does NOT mark the first point in
             * the interval.
             *
             * \param intervals the list of intervals to render in the KML.
             * \param traj the trajectory that generated the intervals.
             * \param stylename the style ID for the interval trip points.
             * \param stride the number of trip points to skip to minimize the amount of data rendered.
             */
            void write_intervals( const trajectory::Interval::PtrList& intervals, const trajectory::Trajectory& traj, const std::string& stylename, int stride = 10 );

            /**
             * \brief Write a set of Areas to the KML output stream.
             *
             * \param aptrset The set of areas to render.
             * \param stylename The style to use for the polygon rendering.
             */
            void write_areas( const std::unordered_set<geo::AreaCPtr>& aptrset, const std::string& stylename );

            /**
             * \brief Write a vector of Areas to the KML output stream.
             *
             * \param aptrset The set of areas to render.
             * \param stylename The style to use for the polygon rendering.
             */
            void write_areas( const std::vector<geo::AreaCPtr>& areas, const std::string& stylename );

        private:
            std::ostream& stream_;
            std::vector<unsigned int> colors;

            unsigned int get_speed_color( double speed );
            void start_folder( const std::string& name, const std::string& description, const std::string& id, const bool open );
            void stop_folder();
    };
}
#endif
