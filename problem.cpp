#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

#include <algorithm>
#include <iostream>
#include <tuple>
#include <vector>

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>

#include <CGAL/Bounded_kernel.h>
#include <CGAL/Nef_polyhedron_2.h>

using CGALKernel = CGAL::Exact_predicates_exact_constructions_kernel;
using CGALPoint = CGALKernel::Point_2;
using CGALPolygon = CGAL::Polygon_2<CGALKernel>;
using CGALBoundedKernel = CGAL::Bounded_kernel<CGALKernel>;
using NefPolyhedron = CGAL::Nef_polyhedron_2<CGALBoundedKernel>;
using PolyExplorer = NefPolyhedron::Explorer;
using std::pair;
using std::vector;

static constexpr size_t numPolys = 5;

static constexpr size_t numPoints[numPolys] = {8,4,3,4,4};
// These are the intermediate polygons created by running 0 4 2 1 1 1 3 4 2 4 0 0 3 0 1 4 through the main example program.
const CGALPoint p0[numPoints[0]] = {CGALPoint(0,0), CGALPoint(3,0), CGALPoint(1.85714,2.28571), CGALPoint(1.5,1.75), CGALPoint(2,1), CGALPoint(1,1), CGALPoint(1.5,1.75), CGALPoint(1.14286,2.28571)};
const CGALPoint p1[numPoints[1]] = {CGALPoint(0,4), CGALPoint(1.14286,2.28571), CGALPoint(1.5,3), CGALPoint(1,4)};
const CGALPoint p2[numPoints[2]] = {CGALPoint(1,1), CGALPoint(2,1), CGALPoint(1.5,1.75)};
const CGALPoint p3[numPoints[3]] = {CGALPoint(1.14286,2.28571), CGALPoint(1.5,1.75), CGALPoint(1.85714,2.28571), CGALPoint(1.5,3)};
const CGALPoint p4[numPoints[4]] = {CGALPoint(1.5,3), CGALPoint(1.85714,2.28571), CGALPoint(3,4), CGALPoint(2,4)};

const pair<const CGALPoint*,const CGALPoint*> inp[numPolys] = {{p0,p0+numPoints[0]},
	{p1,p1+numPoints[1]},
	{p2,p2+numPoints[2]},
	{p3,p3+numPoints[3]},
	{p4,p4+numPoints[4]}};


int main(int argc, const char* argv[]) {
	NefPolyhedron should_be_union(inp, inp+numPolys, NefPolyhedron::POLYGONS);
	NefPolyhedron should_be_clean  = should_be_union.regularization();
	PolyExplorer explorer = should_be_clean.explorer();

	vector<CGALPolygon> fixed;
	using Face = decltype(*explorer.faces_begin());
	std::transform(++(explorer.faces_begin()), explorer.faces_end(), std::back_inserter(fixed), [](Face &face) -> CGALPolygon {
		CGAL::Container_from_circulator<PolyExplorer::Halfedge_around_face_const_circulator> edges(face.halfedge());
		using Edge = decltype(*edges.begin());
		vector<CGALPoint> points;
		// As long as we are using a Bounded_kernel, we don't need to worry that the vertex is actually a ray.
		std::transform(edges.begin(), edges.end(), std::back_inserter(points), [](Edge &edge) -> CGALPoint { return edge.vertex()->point(); });
				
		return CGALPolygon(points.cbegin(), points.cend());
	});

	std::cout << "Finished set: " <<  std::endl;
	std::for_each(fixed.cbegin(), fixed.cend(), [](const CGALPolygon &poly){
		std::cout << "\t" << poly << " ( is simple? " << poly.is_simple() << " )" << std::endl;
	});
		
	return 0;
}
