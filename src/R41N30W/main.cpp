#include <iostream>
#include <stdlib.h>
#include <string>
#include "RainbowTable.hpp"
#include "Utils.hpp"
#include "ArgParser.hpp"

using namespace std;

const char* desc = "Description:\n\
By default, R41N30W works in interactive cracking mode - it asks you for a hash to crack and\n\
attempts to find it in the table.\n\
\n\
There is no initial table provided with the project, as these take a very huge space and there\n\
are too many options to adjust. To make one for yourself, use -g/--generate option.\n\
\n\
You can use other options in conjunction with -g/--generate to specify the generated Rainbow\n\
Table to your needs. Keep in mind, however, that password generation process could take a very\n\
long time, depending on parameters you provide.\n\
";


int main(int argc, char* argv[])
{
    ArgParser parser;
    parser.Add("g,generate", "Generate new Rainbow Table (provide more arguments below to adjust the table)", ArgType::FLAG)
          .Add("t,table", "Table file to be used (either to save to, or to load from)", ArgType::STRING, "table.txt")
          .Add("threads", "Set thread count to use for calculations (default is all logical cores)", ArgType::VALUE, hardwareConcurrency())
          .Add("vertical", "Vertical size of the table (row count)", ArgType::VALUE, 150000)
          .Add("horizontal", "Horizontal size of the table (hash->reduce count)", ArgType::VALUE, 8000)
          .Add("length", "Length of password to be cracked", ArgType::VALUE, 6)
          .Add("hash", "Hash type (available: SHA1, SHA256, BLAKE512)", ArgType::STRING, "BLAKE512")
          .Add("h,help", "Display this message", ArgType::FLAG);

    if (!parser.Parse(argc, argv))
    {
        parser.PrintUsage();
        return 1;
    }

    if (parser.GetFlag('h'))
    {
        std::cout << "R41N3OW - Rainbow Table generator & hash cracker" << std::endl;
        std::cout << "Made by mkk13 & LK, 2017, uses OpenSSL" << std::endl << std::endl;
        parser.PrintUsage();
        std::cout << desc << std::endl;
        return 0;
    }

    OSSLHasher::HashType hashType = OSSLHasher::GetHashTypeFromString(parser.GetString("hash"));
    if (parser.GetFlag('g'))
    {
        RainbowTable table(parser.GetValue("vertical"), parser.GetValue("length"), parser.GetValue("horizontal"), hashType);
        table.SetThreadCount(parser.GetValue("threads"));
        cout << "Will output table to: " << parser.GetString('t') << std::endl;
        table.CreateTable();
        cout << endl;
        cout << "Table created, size: " << table.GetSize() << endl;
        table.Save(parser.GetString('t'));

        return 0;
    }

    if (parser.GetString('t').empty())
    {
        cout << "Provide table name to be loaded, or generate one using --generate option!" << std::endl;
        return 1;
    }

    RainbowTable table(0, 0, 0, hashType);
    table.SetThreadCount(parser.GetValue("threads"));
    if (!table.Load(parser.GetString('t')))
        return 1;

    cout << "\n::Give password hash to look for or 'exit' to terminate" << endl;
    string inputHash, pass;
    do
    {
        cout << ">> ";
        cin >> inputHash;
        if (inputHash == "exit")
            break;
        cout << "Looking for hash now..." << endl;
        pass = table.FindPassword(inputHash);
        if (pass.empty())
            cout << "PASSWORD NOT FOUND! :(\n";
        else
            cout << "PASSWORD FOUND: \"" << pass << "\"\n";
    } while (true);

    cout << "Terminating" << endl;
}
