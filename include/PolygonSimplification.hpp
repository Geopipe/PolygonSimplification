/************************************************************************************
 *
 * Author: Thomas Dickerson
 * Copyright: 2018, Geopipe, Inc.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ************************************************************************************/


#include <algorithm>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <vector>

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>

#include <CGAL/Bounded_kernel.h>
#include <CGAL/Nef_polyhedron_2.h>

namespace com {
	namespace geopipe {
		namespace PolySimpCustomization { 
			template<class Point>
			struct cgal_infer_point_kernel {
				using type = typename Point::Kernel;
			};
			
			template<class Kernel>
			struct cgal_infer_point_kernel<CGAL::Point_2<Kernel> > {
				using type = Kernel;
			};
			
			template<class Point>
			using cgal_infer_point_kernel_t = typename cgal_infer_point_kernel<Point>::type;
		}

		template<typename Point = typename CGAL::Point_2<CGAL::Exact_predicates_exact_constructions_kernel>>
		class PolySimp {
			using CGALKernel = PolySimpCustomization::cgal_infer_point_kernel_t<Point>;
			using CGALPoint = Point;
			using CGALPolygon = CGAL::Polygon_2<CGALKernel, std::vector<Point>>;
			
			using CGALBoundedKernel = CGAL::Bounded_kernel<CGALKernel>;
			using NefPolyhedron = CGAL::Nef_polyhedron_2<CGALBoundedKernel>;
			using PolyExplorer = typename NefPolyhedron::Explorer;

			// Private methods - previously ::detail within namespace
			
			static constexpr auto identity = [](std::vector<CGALPoint> &p) constexpr { return p; };
			static constexpr auto points2Poly = [](std::vector<CGALPoint> &p) constexpr { return CGALPolygon(p.cbegin(), p.cend()); };
			template<typename T> using BareType = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
			
			/**************************************************
			 * Extracts the finite faces from a Nef polyhedron
			 * @arg explorer The `CGAL::Nef_polyhedron_2<CGALBoundedKernel>::Explorer
			 * to traverse.
			 * @arg faces The output container, must support
			 * `std::back_inserter`. Typically `std::vector<E>
			 * for some face-type `E`.
			 * @arg f A functor from 
			 * `std::vector<CGAL::Exact_predicates_exact_constructions_kernel::Point_2>` 
			 * to `E`, where `E` is the element type of `faces`.
			 **************************************************/
			template<typename T, typename F = decltype(identity)> static void extractFiniteFaces(const PolyExplorer &explorer, T& faces, F f = identity) {
				using Face = BareType<decltype(*explorer.faces_begin())>;
				std::transform(++(explorer.faces_begin()), explorer.faces_end(), std::back_inserter(faces), [&f](const Face &face){
					/*
					if (std::distance(face.fc_begin(), face.fc_end()) > 0) {
						std::cerr << ("There shouldn't be a hole in the interior of this face, "
									  "because it's the result of tracing a single polyline into the plane\n") << std::endl;
					}
					*/
					
					CGAL::Container_from_circulator<typename PolyExplorer::Halfedge_around_face_const_circulator> edges(face.halfedge());
					using Edge = BareType<decltype(*edges.begin())>;
					std::vector<CGALPoint> points;
					// As long as we are using a Bounded_kernel, we don't need to worry that the vertex is actually a ray.
					std::transform(edges.begin(), edges.end(), std::back_inserter(points), [](const Edge &edge){ return edge.vertex()->point(); });
					
					return f(points);
				});
			}
			/// Dump some info on `poly` to `std::cout`.
			void debugNef(const NefPolyhedron &poly) {
				std::vector<CGALPolygon> temp;
				PolyExplorer explorer = poly.explorer();
				extractFiniteFaces(poly.explorer(), temp, points2Poly);
				std::for_each(temp.cbegin(), temp.cend(), [](const CGALPolygon &poly){
					std::cout << "\t" << poly << " ( is simple? " << poly.is_simple() << " )" << std::endl;
				});
			}

		  public:
			/*******************************************************************************
			 * Converts an edge soup into a set of simple, orientable, finite-area polygons.
			 * 
			 * 1. Subtract the edges of `inp` from an infinite plane and extract the finite faces. 
			 * 2. Union result back together, and regularize (removing 0- and 1-D holes).
			 * 3. Take the interior (removing 0-D intersections, no 1-D intersections can 
			 * remain after step 2), and re-extract the finite faces.
			 * 
			 * Since this loses orientation information, it makes no assumptions
			 * about whether nested polygons are holes.
			 * 
			 * If `auto_close = true`, it first makes explicit the implicit edge between
			 * the first and last pointer of `inp`. Otherwise treats `inp` as a polyline
			 * instead of a polygon.
			 * 
			 * <pre class="markdeep">
			 * # Example 1: Repairing bowties
			 * ## Input:
			 * *************************************************************************
			 * *   Input (Unorientable)                 Output                         *
			 * *  .--------->.                         .--------->.                    *
			 * *  ^          |                         ^          |                    *
			 * *  |          |                         |  Face 1  |                    *
			 * *  |          |                         |          v                    *
			 * *  '<------------------'                '<--------. .------->.          *
			 * *             |        ^                           ^         |          *
			 * *             |        |                           | Face 2  |          *
			 * *             |        |                           |         v          *
			 * *             v.------>'                           '<--------.          *
			 * *                                                                       *
			 * *************************************************************************
			 * 
			 * </pre>
			 *******************************************************************************/
			static std::vector<CGALPolygon> simplifyPolygon(const CGALPolygon &inp, bool auto_close = true) {
				using std::pair;
				using std::vector;
				using PointIter = typename vector<CGALPoint>::const_iterator;
				using PointIterPair = pair<PointIter, PointIter>;
				
				vector<CGALPoint> points(inp.container());
				vector<CGALPolygon> polys;
				
				if (auto_close) {
					points.push_back(inp.container().front());
				}
				
				PointIterPair inp_plines[1] = { {points.cbegin(), points.cend()} };
				
				NefPolyhedron whole_plane(NefPolyhedron::COMPLETE);
				NefPolyhedron boundary_pointset(inp_plines, inp_plines + 1, NefPolyhedron::POLYLINES);
				// Stage 1: resolve self-intersections at points.
				NefPolyhedron area_pointset = (whole_plane - boundary_pointset).interior();
				
				vector<vector<CGALPoint> > components;
				extractFiniteFaces(area_pointset.explorer(), components);
				
				vector<PointIterPair> component_polys;
				std::transform(components.cbegin(), components.cend(), std::back_inserter(component_polys), [](const vector<CGALPoint> &component) {
					return (PointIterPair){component.cbegin(), component.cend()};
				});
				
				// Stage 2 remove collinearities
				NefPolyhedron remainder = std::accumulate(component_polys.cbegin(), component_polys.cend(), NefPolyhedron(NefPolyhedron::EMPTY), [](const NefPolyhedron& left, const PointIterPair& right){
					return left.join(NefPolyhedron(right.first, right.second));
				});
				//debugNef(remainder);
				
				NefPolyhedron remainder_clean = remainder.regularization().interior();
				//debugNef(remainder_clean);
				
				extractFiniteFaces(remainder_clean.explorer(), polys, points2Poly);
				
				return polys;
			}
		};
	};
};
