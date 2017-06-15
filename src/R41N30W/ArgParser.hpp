#pragma once

#include <string>
#include <vector>
#include <memory>


enum class ArgType: unsigned char
{
    UNDEFINED = 0,
    FLAG,
    VALUE,
    STRING,
};

struct Arg
{
    friend class ArgParser;

    char shortName;
    std::string name;
    std::string desc;
    ArgType type;
    uint32_t value;
    std::string string;

    Arg()
        : shortName('\0')
        , name()
        , desc()
        , type(ArgType::UNDEFINED)
        , value(0)
        , string()
        , acquired(false)
    {
    }

private:
    bool acquired;
};


class ArgParser
{
public:
    ArgParser();
    ~ArgParser();

    // name should consist of either:
    //   * a single char (short arg)
    //   * a string (long arg)
    //   * a combination of both, separated by comma
    ArgParser& Add(const std::string& name, ArgType type);
    ArgParser& Add(const std::string& name, ArgType type, uint32_t defaultValue);
    ArgParser& Add(const std::string& name, ArgType type, const std::string& defaultString);
    ArgParser& Add(const std::string& name, const std::string& desc, ArgType type);
    ArgParser& Add(const std::string& name, const std::string& desc, ArgType type, uint32_t defaultValue);
    ArgParser& Add(const std::string& name, const std::string& desc, ArgType type, const std::string& defaultString);
    bool Parse(int argc, char* argv[]);
    void PrintUsage();

    bool GetFlag(const std::string& name);
    bool GetFlag(const char shortName);
    uint32_t GetValue(const std::string& name);
    uint32_t GetValue(const char shortName);
    std::string GetString(const std::string& name);
    std::string GetString(const char shortName);

private:
    using ArgIt = std::vector<Arg>::iterator;

    // modifies a in-place to supply it with name/shortname combo
    void ParseArgName(const std::string& name, Arg& a);

    ArgIt FindShortName(const char shortName);
    ArgIt FindName(const std::string& name);

    std::vector<Arg> mArgVector;
};
