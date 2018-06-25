//
// Created by alex on 7/15/15.
//

#ifndef TESSVG_SVGPROCESSOR_H
#define TESSVG_SVGPROCESSOR_H
#include <string>
#include <map>
#include "GlDefs.h"
#include "pugixml.hpp"
#include "SvgPath.h"
#include <functional>
#include "Tesselate.h"
#include "Bounds.h"
#include <iostream>

class xmlerror : public std::runtime_error
{
public:
    explicit xmlerror(const std::string &__arg) : runtime_error(__arg)
    { }
};

class SvgProcessor
{
protected:
    struct WithBounds
    {
        GlBounds   bounds;
        Vertexes   vertexes;

        void makeBounds()
        {
            bounds.reset();

            //need to clean vertexes, model cannot have 2 same points 1 by 1

            if (vertexes.size() > 2)
            {
                auto result = vertexes.begin();
                for (auto first = vertexes.begin(), last = vertexes.end() - 1; first != last; ++first)
                {
                    if (!(*first == *(first + 1)))
                    {
                        *result = *first;
                        ++result;
                    }
                }

                vertexes.erase(result + 1, vertexes.end());
                if (vertexes.size() < 3)
                    throw std::runtime_error("Lost too many points doing cleanse of the sames.");
            }
            updateBounds();
        }
        virtual ~WithBounds()
        {
            bounds.reset();
        }
    protected:
        virtual void updateBounds()
        {
            for (auto& v : vertexes)
                bounds.add_point(v.x(), v.y());
        }
    };

public:
    struct TessResult : public WithBounds
    {
        std::map<std::string, std::string> attributes;
        void setAttributes(const pugi::xml_node& node)
        {
            attributes.clear();

            for (auto attr = node.attributes_begin(); attr != node.attributes_end(); attr++)
                attributes[std::string(attr->name())] = std::string(attr->as_string());
        }

    protected:
    };

    using pathes_t = std::vector<std::pair<std::string, TessResult>>;

    struct BoundedGroup : public WithBounds
    {
        pathes_t pathes;
        ~BoundedGroup() override
        {
            pathes.clear();
        }

    protected:
        void updateBounds() override
        {
            WithBounds::updateBounds();
            for (auto& p : pathes)
                p.second.makeBounds();
        }
    };

    using group_t = std::vector<std::pair<std::string, BoundedGroup>>;

private:
    Tesselate ts;
    pugi::xml_document doc;
    group_t tesselated;
    void parse(const pugi::xml_node&node, const pugi::xml_node &parent, Loops *loops, Loops *total_loops);
public:

    SvgProcessor() = default;
    explicit SvgProcessor(std::istream& src);
    void parse_svg_file(std::istream& src);
    const group_t& getTesselated() const;
    void postProcessTesselatedVerteces(const std::function<void(BoundedGroup &)> &func);
};


#endif //TESSVG_SVGPROCESSOR_H
