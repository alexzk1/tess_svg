#include "tess_svg/SvgProcessor.h"
#include <fstream>
#include <sys/stat.h>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "tess_svg/idumper.h"

const auto shiftToZero = [](SvgProcessor::BoundedGroup& path)
{

    GlVertex flip(0, path.bounds.height());
    GlVertex shift(path.bounds.xmin, path.bounds.ymin);

    for (auto& v : path.vertexes)
    {
        auto shifted = (v.get() - shift.get());
        shifted.y *= -1;
        v = GlVertex(shifted  + flip.get());
    }

    for (auto& p : path.pathes)
    {
        for (auto& v : p.second.vertexes)
        {
            auto shifted = (v.get() - shift.get());
            shifted.y *= -1;
            v = GlVertex(shifted  + flip.get());
        }
    }
};

const auto scale = [](SvgProcessor::BoundedGroup& path)
{

    for (auto& v : path.vertexes)
        v = GlVertex(v.get() / path.bounds.scaleFactor());

    for (auto& p : path.pathes)
    {
        auto s = p.second.bounds.scaleFactor();
        for (auto& v : p.second.vertexes)
            v = GlVertex(v.get() / s);
    }
};

bool fexists(const std::string& name)
{
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}


extern int BEIZER_PARTS;
extern int ELLIPSE_POINTS;
extern bool USE_PATH_COMMENT;

int main(const int ac, const char** av)
{
    using namespace boost::program_options;
    using namespace boost::filesystem;

    // Declare a group of options that will be
    // allowed only on command line
    options_description generic("Generic options");
    generic.add_options()
    ("help", "produce help message")
    ("input,i", value<std::string>(), "input file name, if not set - using stdin")
    ("output,o", value<std::string>(), "output file name, if not set - using stdout")
    ("preffix,p", value<std::string>(), "custom prefix of the generated constants names, otherwise tries to use filenames, then empty.")

    ("bparts", value<int>()->default_value(10), "amount of beizer parts to use on rasterize")
    ("epoints", value<int>()->default_value(1024), "amount of points on ellipse to use on rasterize")
    ("nocomment", "if set disables commenting out of separated generated pathes in case of grouping")
    //should be same options as below in map keys
    ("java,j", "output as Java")
    ("json,J", "output as JSON (packed)")
    ("prettyjson,P", "output as pretty JSON")
    ("sfml,s", "output as C++ declaration (initializer list), sfml based(sf_polygon.h).")
    ("sfmlmap,S", "output as C++ declaration in form of std::map (initializer list), sfml based(sf_polygon.h).")
    ("lua,l", "output as Lua with local variables")
    ("lua-no-local,L", "output as Lua without local variables")
    ;

    variables_map vm;
    bool error_opts = false;
    try
    {
        if (ac < 2)
            throw std::runtime_error("No options given.");
        store(parse_command_line(ac, av, generic), vm);
        notify(vm);
        BEIZER_PARTS   = vm["bparts"].as<int>();
        ELLIPSE_POINTS = vm["epoints"].as<int>();
        USE_PATH_COMMENT = !vm.count("nocomment");
    }
    catch (...)
    {
        error_opts = true;
    }


    if (vm.count("help") || error_opts)
    {
        std::cout << generic << "\n\n";
        std::cout << "Warning! Source svg (image) files must assume that initial zero rotation of Actor points to right --->>!!!!\n";
        //std::cout << "If SVG file contains object \"SCREEN\" it will be used as reference for sizes and removed from output.\n";
        return (error_opts) ? 1 : 0;
    }


    auto fn = (vm.count("input")) ? vm["input"].as<std::string>() : "";
    std::fstream inp;
    if (!fn.empty())
        inp.open(fn, std::fstream::in);
    std::istream& sptr = (fn.empty()) ? std::cin : inp;

    //wana use input file name as prefix
    const std::string prefix = (vm.count("prefix")) ? vm["prefix"].as<std::string>() : ((fn.empty()) ? "" : path(fn).stem().string());

    fn = (vm.count("output")) ? vm["output"].as<std::string>() : "";
    std::fstream out;
    if (!fn.empty())
        out.open(fn, std::fstream::out);
    std::ostream& optr = (fn.empty()) ? std::cout : out;


    SvgProcessor test(sptr);
    std::map<std::string, std::function<IDumperPtr()>> factories =
    {
        //should be same "options" as above in menu
        {"java", [&] { return dumperFactory<JavaDumper>(optr, test, prefix);}},
        {"json", [&] { return dumperFactory<JsonDumper>(optr, test, prefix, false);}},
        {"prettyjson", [&] { return dumperFactory<JsonDumper>(optr, test, prefix, true);}},
        {"sfml", [&] { return dumperFactory<SFMLDumper>(optr, test, prefix);}},
        {"sfmlmap", [&] { return dumperFactory<SFMLMapDumper>(optr, test, prefix);}},
        {"lua", [&] { return dumperFactory<LuaDumper>(optr, test, prefix, true);}},
        {"lua-no-local", [&] { return dumperFactory<LuaDumper>(optr, test, prefix, false);}},
    };

    test.postProcessTesselatedVerteces(shiftToZero);
    test.postProcessTesselatedVerteces(scale);

    bool ok = false;
    for (const auto& kv : factories)
    {
        if (vm.count(kv.first))
        {
            kv.second();
            ok = true;
            break; //only 1 output lang at once can be
        }
    }
    if (!ok)
    {
        std::cerr << "You should specify exactly 1 output type." << std::endl << generic << std::endl;
        return 1;
    }

    return 0;
}
