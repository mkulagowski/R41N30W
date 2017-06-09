#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <map>
#include <string>
#include <time.h>
#include "RainbowTable.hpp"
#include "Utils.hpp"
#include <openssl/sha.h>

using namespace std;

int main(void)
{
    cout << "Threads used: " << Utils::hardwareConcurrency() << endl;
    RainbowTable* table = new RainbowTable(15000000, 6, 8000);

    //table->LoadPasswords("passes.txt");
    table->CreateTable();
    //cout << "table created, size: " << table->GetSize() << endl;
    //table->Print();
    table->Save("testtable_15M_6_8K.txt");
    //table->Load("testtable_frompass2.txt");
    cout << "\n::Give password hash to look for or 'exit' to terminate" << endl;

    string inputHash, pass;
    do
    {
        cout << ">>";
        cin >> inputHash;
        if (inputHash == "exit")
            break;
        cout << "Looking for hash now..." << endl;
        pass = table->FindPasswordParallel(inputHash);
        if (pass.empty())
            cout << "PASSWORD NOT FOUND! :(\n";
        else
            cout << "PASSWORD FOUND: \"" << pass << "\"\n";
    } while (true);
    cout << "Terminating" << endl;
}



