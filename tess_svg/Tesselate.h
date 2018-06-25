//
// Created by alex on 7/14/15.
//

#ifndef TESSVG_TESSELACT_H
#define TESSVG_TESSELACT_H

#include <memory>
#include <functional>
#include "GlDefs.h"


class Tesselate
{
public:

private:
    std::shared_ptr<GLUtesselator> tess;
    Vertexes tlist;
    Vertexes spareverts;
    Vertexes curr_shape;

    std::vector<std::shared_ptr<void>> funcs;
protected:
     void callback_vertex(GLdouble *vertex);
public:
    Tesselate();
    const Vertexes& process(const Loops& vertexes, bool contourOnly);
    const Vertexes& getTesselated() const;
};


#endif //TESSVG_TESSELACT_H
