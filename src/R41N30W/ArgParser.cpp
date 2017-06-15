#include "ArgParser.hpp"
#include <iostream>

ArgParser::ArgParser()
{
}

ArgParser::~ArgParser()
{
}

ArgParser& ArgParser::Add(const std::string& name, ArgType type)
{
    return Add(name, "", type);
}

ArgParser& ArgParser::Add(const std::string& name, ArgType type, uint32_t defaultValue)
{
    return Add(name, "", type, defaultValue);
}

ArgParser& ArgParser::Add(const std::string& name, ArgType type, const std::string& defaultString)
{
    return Add(name, "", type, defaultString);
}

ArgParser& ArgParser::Add(const std::string& name, const std::string& desc, ArgType type)
{
    // TODO name/shortname collision check
    Arg a;
    a.desc = desc;
    a.type = type;
    ParseArgName(name, a);

    mArgVector.push_back(a);

    return *this;
}

ArgParser& ArgParser::Add(const std::string& name, const std::string& desc, ArgType type, uint32_t defaultValue)
{
    // TODO name/shortname collision check
    Arg a;
    a.desc = desc;
    a.type = type;
    a.value = defaultValue;
    ParseArgName(name, a);

    mArgVector.push_back(a);

    return *this;
}

ArgParser& ArgParser::Add(const std::string& name, const std::string& desc, ArgType type, const std::string& defaultString)
{
    // TODO name/shortname collision check
    Arg a;
    a.desc = desc;
    a.type = type;
    a.string = defaultString;
    ParseArgName(name, a);

    mArgVector.push_back(a);

    return *this;
}

bool ArgParser::Parse(int argc, char* argv[])
{
    bool success = true;
    for (int i = 1; i < argc; ++i)
    {
        // arg must start with a dash
        if (argv[i][0] != '-')
        {
            std::cout << "Invalid argument: " << argv[i] << std::endl;
            success = true;
            continue;
        }

        ArgIt it;
        if (argv[i][1] == '-')
            it = FindName(&argv[i][2]);
        else
            it = FindShortName(argv[i][1]);

        if (it == mArgVector.end())
        {
            std::cout << "Unrecognised argument: " << argv[i] << std::endl;
            success = false;
            continue;
        }

        // it now points to our argument, check it further
        if (it->type == ArgType::VALUE)
        {
            i++;
            if (i == argc)
            {
                std::cout << "Not enough arguments provided" << std::endl;
                success = false;
                continue;
            }

            try
            {
                it->value = std::stoi(argv[i]);
            }
            catch (...)
            {
                std::cout << "Invalid value for argument " << it->name << " provided: " << argv[i];
                success = false;
                continue;
            }
        }
        else if (it->type == ArgType::STRING)
        {
            i++;
            if (i == argc)
            {
                std::cout << "Not enough arguments provided" << std::endl;
                success = false;
                continue;
            }

            it->string = argv[i];
        }
        else if (it->type == ArgType::FLAG)
        {
            // just raise the value to non-zero that we got the arg
            it->value = 1;
        }

        it->acquired = true;
    }

    return success;
}

void ArgParser::PrintUsage()
{
    std::cout << "Arguments are: " << std::endl;

    for (auto arg: mArgVector)
    {
        std::cout << "  ";
        if (arg.shortName != '\0')
            std::cout << "-" << arg.shortName << " ";
        if (!arg.name.empty())
            std::cout << "--" << arg.name << std::endl;
        if (!arg.desc.empty())
            std::cout << "\t" << arg.desc << std::endl;
    }
    std::cout << std::endl;
}

bool ArgParser::GetFlag(const std::string& name)
{
    ArgIt it = FindName(name);
    if (it == mArgVector.end())
        return false;

    if (it->type != ArgType::FLAG)
        return false;

    return it->value;
}

bool ArgParser::GetFlag(const char shortName)
{
    ArgIt it = FindShortName(shortName);
    if (it == mArgVector.end())
        return false;

    if (it->type != ArgType::FLAG)
        return false;

    return it->value;
}


uint32_t ArgParser::GetValue(const std::string& name)
{
    ArgIt it = FindName(name);
    if (it == mArgVector.end())
        return 0;

    if (it->type != ArgType::VALUE)
        return 0;

    return it->value;
}

uint32_t ArgParser::GetValue(const char shortName)
{
    ArgIt it = FindShortName(shortName);
    if (it == mArgVector.end())
        return 0;

    if (it->type != ArgType::VALUE)
        return 0;

    return it->value;
}

std::string ArgParser::GetString(const std::string& name)
{
    ArgIt it = FindName(name);
    if (it == mArgVector.end())
        return "";

    if (it->type != ArgType::STRING)
        return "";

    return it->string;
}

std::string ArgParser::GetString(const char shortName)
{
    ArgIt it = FindShortName(shortName);
    if (it == mArgVector.end())
        return "";

    if (it->type != ArgType::STRING)
        return "";

    return it->string;
}

void ArgParser::ParseArgName(const std::string& name, Arg& a)
{
    // find a comma in our arg definition
    size_t comma = name.find(',');
    if (comma == std::string::npos)
    {
        // now we have two options - it's either a short arg, or a long arg
        // short arg will be one-char only
        if (name.length() == 1)
            a.shortName = name[0];
        else
            a.name = name;
    }
    else
    {
        // comma is here - check where is the short arg
        // it should be at the beginning, or at an end, otherwise it is an error
        if (comma == name.length() - 2)
        {
            a.shortName = name.c_str()[comma+1];
            a.name = name.substr(0, comma);
        }
        else if (comma == 1)
        {
            a.shortName = name.c_str()[0];
            a.name = name.substr(comma+1);
        }
        else
            std::cout << "Cannot add argument - incorrect format " << name << std::endl;
    }
}

ArgParser::ArgIt ArgParser::FindShortName(const char shortName)
{
    ArgIt it = mArgVector.begin();
    for (it; it != mArgVector.end(); ++it)
        if (it->shortName == shortName)
            break;

    return it;
}

ArgParser::ArgIt ArgParser::FindName(const std::string& name)
{
    ArgIt it = mArgVector.begin();
    for (it; it != mArgVector.end(); ++it)
        if (it->name == name)
            break;

    return it;
}
