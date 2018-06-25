//
// Created by alex on 7/14/15.
//

#include "Tesselate.h"
#include <cstring>
#include "callbacks.h"

Tesselate::Tesselate() : tess(nullptr)
{
}


const Vertexes & Tesselate::process(const Loops& vertexes, bool contourOnly)
{
    tess.reset(gluNewTess(),
               [](auto ptr)
    {
        if (ptr != nullptr)
            gluDeleteTess(ptr);
    });
    gluTessNormal(tess.get(), 0, 0, 1);
    gluTessProperty(tess.get(), GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
    gluTessProperty(tess.get(), GLU_TESS_BOUNDARY_ONLY, (int)contourOnly);

    GLenum tess_style;
    tlist.clear();
    spareverts.clear();

    //must store shared_ptr on local variables, to ensure pointer will be valid till function ends
    auto l_vertex = to_glu_callback<GLU_TESS_VERTEX>([&](GLdouble * vertex)
    {
        // std::cout<<"l_vertex: "<<*vertex<<" "<<*(vertex+1)<<std::endl;
        curr_shape.push_back(GlVertex(*vertex, *(vertex + 1)));
    });

    auto l_begin = to_glu_callback<GLU_TESS_BEGIN>([&](GLenum w)
    {
        //std::cout<<"l_begin: "<<w<<std::endl;
        tess_style = w;
        curr_shape.clear();
    });

    auto l_error = to_glu_callback<GLU_TESS_ERROR>([](GLenum code)
    {
        std::cerr << "GLU Tesselation Error: " << gluErrorString(code) << std::endl;
        exit(code);
    });

    auto l_end = to_glu_callback<GLU_TESS_END>([&]()
    {
        switch (tess_style)
        {
            default:
                std::cerr << "Unknown tesselation style: " << tess_style << std::endl;
                break;
            case GL_LINE_LOOP:
            case GL_TRIANGLES:
                tlist.insert(tlist.end(), curr_shape.cbegin(), curr_shape.cend());
                break;
            case GL_TRIANGLE_STRIP:
                while (curr_shape.size() > 2)
                {
                    std::for_each(curr_shape.cbegin(), curr_shape.cbegin() + 3, [&](auto & v)
                    {
                        tlist.push_back(v);
                    });
                    curr_shape.erase(curr_shape.begin());
                }
                break;
            case GL_TRIANGLE_FAN:
            {
                auto c = curr_shape.front();
                curr_shape.erase(curr_shape.begin());
                while (curr_shape.size() > 1)
                {
                    tlist.push_back(c);
                    std::for_each(curr_shape.cbegin(), curr_shape.cbegin() + 2, [&](auto & v)
                    {
                        tlist.push_back(v);
                    });
                    curr_shape.erase(curr_shape.begin());
                }
            }
            break;

        }
        curr_shape.clear();
    });

    auto l_combine = to_glu_callback<GLU_TESS_COMBINE>([&](GLdouble * coords, GLdouble **, GLfloat*, GLdouble **dataOut)
    {
        size_t size = 3 * sizeof(GLdouble);
        *dataOut = (GLdouble *) malloc(size);
        memcpy(*dataOut, coords, size);
        spareverts.push_back(GlVertex(*coords, *(coords + 1)));
    });

    gluTessCallback(tess.get(), GLU_TESS_VERTEX,  l_vertex);
    gluTessCallback(tess.get(), GLU_TESS_BEGIN,   l_begin);
    gluTessCallback(tess.get(), GLU_TESS_ERROR,   l_error);
    gluTessCallback(tess.get(), GLU_TESS_END,     l_end);
    gluTessCallback(tess.get(), GLU_TESS_COMBINE, l_combine);

    GlPolygon arr_vert;
    std::for_each(vertexes.cbegin(), vertexes.cend(), [&](auto & vert)
    {
        GlContour cntr;
        std::for_each(vert.cbegin(), vert.cend(), [&](auto & v)
        {
            cntr.push_back(v.x());
            cntr.push_back(v.y());
            cntr.push_back(0);
        });
        arr_vert.push_back(cntr);
    });

    gluTessBeginPolygon(tess.get(), nullptr);
    for (auto & c : arr_vert)
    {
        gluTessBeginContour(tess.get());
        for (size_t j = 0, nj = c.size(); j < nj; j += 3)
            gluTessVertex(tess.get(), c.data() + j, c.data() + j);
        gluTessEndContour(tess.get());
    }
    gluTessEndPolygon(tess.get());
    tess.reset();

    return tlist;
}

const Vertexes &Tesselate::getTesselated() const
{
    return tlist;
}
