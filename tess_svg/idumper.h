#ifndef IDUMPER_H
#define IDUMPER_H
#include <iostream>
#include "GlDefs.h"
#include <vector>
#include "SvgProcessor.h"

class IDumper
{
protected:
    std::ostream& outstr;
    const std::string namePrefix;
    virtual void dumpPath(const SvgProcessor::group_t &what) const = 0;
public:

    IDumper() = delete;
    IDumper(const IDumper&) = delete;

    explicit IDumper(std::ostream& out, const SvgProcessor& pr, std::string  namePrefix);
    virtual ~IDumper() = default;
};

using IDumperPtr = std::shared_ptr<IDumper>;

class JavaDumper: public IDumper
{
protected:
    void dumpPath(const SvgProcessor::group_t &what) const override;
    void dumpVertexes(const Vertexes& what) const;
public:
    explicit JavaDumper(std::ostream& out, const SvgProcessor& pr, const std::string& namePrefix = "");
    JavaDumper() = delete;
    JavaDumper(const JavaDumper&) = delete;
};


class JsonDumper: public IDumper
{
protected:
    const bool pretty;
    void dumpPath(const SvgProcessor::group_t &what) const override;
    void dumpVertexes(const Vertexes& what) const;
public:
    explicit JsonDumper(std::ostream& out, const SvgProcessor& pr, const std::string& namePrefix = "", bool pretty = true);
    JsonDumper() = delete;
    JsonDumper(const JsonDumper&) = delete;
};

class SFMLDumper: public IDumper
{
protected:
    void dumpPath(const SvgProcessor::group_t &what) const override;
    void dumpVertexes(const Vertexes& what) const;
public:
    explicit SFMLDumper(std::ostream& out, const SvgProcessor& pr, const std::string& namePrefix = "");
    SFMLDumper() = delete;
    SFMLDumper(const SFMLDumper&) = delete;
};

template<class T, class ... Args>
IDumperPtr dumperFactory(std::ostream& out, const SvgProcessor& pr, std::string namePrefix, Args...args)
{
    return std::make_shared<T>(out, pr, namePrefix, args...);
}
#endif // IDUMPER_H
