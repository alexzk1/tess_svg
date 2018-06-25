# tess_svg
Converts SVG files to vertexes in different formats, like Java code/Json/C++ etc.
It uses opengl tesselate to produce "triangular" shapes out of "pathes". Other format dumpers can be added later.

Prior using this program ALL svg objects(like ellipse) must be converted to "path".

Complex things like groups are particulary supported by dumpers.

P.S. Sorry for using qmake as build system only, but I got used to it - you're free to go to rewrite to cmake/etc.
