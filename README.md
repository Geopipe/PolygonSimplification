# Polygon Simplification library

Has CGAL as a dependency.

You should be able to build the example program with a command like this (adjusted for your compiler of choice + library locations):
`clang++ -std=c++11 -Iinclude/ -I/opt/local/include/ -L/opt/local/lib -lCGAL_Core -lCGAL -lgmp -lmpfr  main.cpp -o polygon-simplification`

To test out different simplifications, try:

 - Bowties: `./polygon-simplification 0 0 1 1 1 0 0 1`
 - Collinear edges: `./polygon-simplification 0 0 1 0 0.5 0 1 1 1 0 0 1`




