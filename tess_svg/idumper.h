#pragma once

#include "GlDefs.h"
#include "SvgProcessor.h"

#include <iostream>
#include <memory>
#include <string>

class IDumper
{
  protected:
    std::ostream &outstr;
    const std::string namePrefix;

  public:
    virtual ~IDumper();
    IDumper() = delete;
    IDumper(const IDumper &) = delete;
    IDumper &operator=(const IDumper &) = delete;
    IDumper(IDumper &&) = delete;
    IDumper &operator=(IDumper &&) = delete;

    explicit IDumper(std::ostream &out, const SvgProcessor &pr, std::string namePrefix);
};

using IDumperPtr = std::shared_ptr<IDumper>;

class JsonDumper : public IDumper
{
  protected:
    const bool pretty;
    void dumpPath(const SvgProcessor::group_t &what) const;
    void dumpVertexes(const Vertexes &what) const;

  public:
    explicit JsonDumper(std::ostream &out, const SvgProcessor &pr,
                        const std::string &namePrefix = "", bool pretty = true);
};

class LuaDumper : public IDumper
{
  protected:
    void dumpPath(const SvgProcessor::group_t &what) const;
    void dumpVertexes(const Vertexes &what) const;
    const bool use_local;

  public:
    explicit LuaDumper(std::ostream &out, const SvgProcessor &pr,
                       const std::string &namePrefix = "", const bool use_local = true);
};

template <class T, class... Args>
IDumperPtr dumperFactory(std::ostream &out, const SvgProcessor &pr, std::string namePrefix,
                         Args... args)
{
    return std::make_shared<T>(out, pr, namePrefix, args...);
}
