#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

#include <PolygonSimplification.hpp>

using namespace com::geopipe;

int main(int argc, const char* argv[]) {
	using simp = PolySimp;

	int ret;
	if((!argc % 2)){
		std::cout << argv[0] << " x0 y0 [... xn yn]" << std::endl;
		ret = -1;
	} else {
		typedef const char * (*CStrPair)[2];
		CStrPair arg_pairs = (CStrPair)(argv + 1);
		std::vector<simp.CGALPoint> problemNodes;
		
		std::transform(arg_pairs, arg_pairs + (argc / 2), std::back_inserter(problemNodes),
					   [](const char *(arg[2])){
						   return CGALPoint(std::atof(arg[0]), std::atof(arg[1]));
					   });
		
		simp.CGALPolygon test(problemNodes.cbegin(), problemNodes.cend());
		
		std::vector<simp.CGALPolygon> fixed = simp.simplifyPolygon(test);
		std::cout << "Finished set: " <<  std::endl;
		std::for_each(fixed.cbegin(), fixed.cend(), [](const CGALPolygon &poly){
			std::cout << "\t" << poly << " ( is simple? " << poly.is_simple() << " )" << std::endl;
		});
		
		ret = 0;
	}

	return ret;
}
