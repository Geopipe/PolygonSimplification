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
		namespace PolySimp {
			using CGALKernel = CGAL::Exact_predicates_exact_constructions_kernel;
			using CGALPoint = CGALKernel::Point_2;
			using CGALPolygon = CGAL::Polygon_2<CGALKernel>;
			
			using CGALBoundedKernel = CGAL::Bounded_kernel<CGALKernel>;
			using NefPolyhedron = CGAL::Nef_polyhedron_2<CGALBoundedKernel>;
			using PolyExplorer = NefPolyhedron::Explorer;
			
			namespace detail {
				using std::vector;
				
				auto identity = [](vector<CGALPoint> &p){ return p; };
				auto points2Poly = [](vector<CGALPoint> &p){ return CGALPolygon(p.cbegin(), p.cend()); };
				template<typename T> using BareType = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
				
				template<typename T, typename F = decltype(identity)> void extractFiniteFaces(const PolyExplorer &explorer, T& faces, F f = identity) {
					using Face = BareType<decltype(*explorer.faces_begin())>;
					std::transform(++(explorer.faces_begin()), explorer.faces_end(), std::back_inserter(faces), [&f](const Face &face){
						/*
						if (std::distance(face.fc_begin(), face.fc_end()) > 0) {
							std::cerr << ("There shouldn't be a hole in the interior of this face, "
										  "because it's the result of tracing a single polyline into the plane\n") << std::endl;
						}
						*/
						
						CGAL::Container_from_circulator<PolyExplorer::Halfedge_around_face_const_circulator> edges(face.halfedge());
						using Edge = BareType<decltype(*edges.begin())>;
						vector<CGALPoint> points;
						// As long as we are using a Bounded_kernel, we don't need to worry that the vertex is actually a ray.
						std::transform(edges.begin(), edges.end(), std::back_inserter(points), [](const Edge &edge){ return edge.vertex()->point(); });
						
						return f(points);
					});
				}
				
				void debugNef(const NefPolyhedron &poly) {
					vector<CGALPolygon> temp;
					PolyExplorer explorer = poly.explorer();
					extractFiniteFaces(poly.explorer(), temp, points2Poly);
					std::for_each(temp.cbegin(), temp.cend(), [](const CGALPolygon &poly){
						std::cout << "\t" << poly << " ( is simple? " << poly.is_simple() << " )" << std::endl;
					});
				}
			}
			
			std::vector<CGALPolygon> simplifyPolygon(const CGALPolygon &inp, bool auto_close = true){
				using std::pair;
				using std::vector;
				using PointIter = vector<CGALPoint>::const_iterator;
				using PointIterPair = pair<PointIter, PointIter>;
				
				vector<CGALPoint> points(inp.container());
				vector<CGALPolygon> polys;
				
				if(auto_close){
					points.push_back(inp.container().front());
				}
				
				PointIterPair inp_plines[1] = { {points.cbegin(), points.cend()} };
				
				NefPolyhedron whole_plane(NefPolyhedron::COMPLETE);
				NefPolyhedron boundary_pointset(inp_plines, inp_plines + 1, NefPolyhedron::POLYLINES);
				// Stage 1: resolve self-intersections at points.
				NefPolyhedron area_pointset = (whole_plane - boundary_pointset).interior();
				
				vector<vector<CGALPoint> > components;
				detail::extractFiniteFaces(area_pointset.explorer(), components);
				
				vector<PointIterPair> component_polys;
				std::transform(components.cbegin(), components.cend(), std::back_inserter(component_polys), [](const vector<CGALPoint> &component) {
					return (PointIterPair){component.cbegin(), component.cend()};
				});
				
				// Stage 2 remove collinearities
				NefPolyhedron remainder = std::accumulate(component_polys.cbegin(), component_polys.cend(), NefPolyhedron(NefPolyhedron::EMPTY), [](const NefPolyhedron& left, const PointIterPair& right){
					return left.join(NefPolyhedron(right.first, right.second));
				});
				//detail::debugNef(remainder);
				
				NefPolyhedron remainder_clean = remainder.regularization().interior();
				//detail::debugNef(remainder_clean);
				
				detail::extractFiniteFaces(remainder_clean.explorer(), polys, detail::points2Poly);
				
				return polys;
			}
			
			
		};
	};
};
