# tess_svg

Converts SVG files to vertexes in different formats, like Java code/Json/C++ etc.
It uses opengl tesselate to produce "triangular" shapes out of "pathes". Other format dumpers can be added later.

Prior using this program ALL svg objects(like ellipse) must be converted to "path".

Complex things like groups are particulary supported by dumpers.

### Why

Initial intention was to draw game's UI in InkScape than convert it into C++/Java/Lua code vertexes 
and use as compiled-in shapes. That was good idea as for 2016 about.

~P.S. Sorry for using qmake as build system only, but I got used to it - you're free to go to rewrite to cmake/etc.~

P.P.S Switched to `cmake` as it won the battle till 2026.
