#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

#include <PolygonSimplification.hpp>

using namespace com::geopipe::PolySimp;
/*
const CGALPoint problemNodes[12] =
{	{68808., -15875.},
	{73923., -12489.},
	{73927., -12167.},
	{71378., -08331.},
	{69931., -09319.},
	{70021., -09546.},
	{69931., -09319.},
	{66568., -11450.},
	{67538., -13648.},
	{67927., -13387.},
	{68778., -15792.},
	{68808., -15875.}};
 */

// Recommended test case 1: bowties - 0 0 1 1 1 0 0 1
// Recommended test case 2: overlaps - 0 0 1 0 0.5 0 1 1 1 0 0 1

int main(int argc, const char* argv[]) {
	int ret;
	if((!argc % 2)){
		std::cout << argv[0] << " x0 y0 [... xn yn]" << std::endl;
		ret = -1;
	} else {
		typedef const char * (*CStrPair)[2];
		CStrPair arg_pairs = (CStrPair)(argv + 1);
		std::vector<CGALPoint> problemNodes;
		
		std::transform(arg_pairs, arg_pairs + (argc / 2), std::back_inserter(problemNodes),
					   [](const char *(arg[2])){
						   return CGALPoint(std::atof(arg[0]), std::atof(arg[1]));
					   });
		
		CGALPolygon test(problemNodes.cbegin(), problemNodes.cend());
		
		std::vector<CGALPolygon> fixed = simplifyPolygon(test);
		std::cout << "Finished set: " <<  std::endl;
		std::for_each(fixed.cbegin(), fixed.cend(), [](const CGALPolygon &poly){
			std::cout << "\t" << poly << " ( is simple? " << poly.is_simple() << " )" << std::endl;
		});
		
		ret = 0;
	}

	return ret;
}
