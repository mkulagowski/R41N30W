/*
*  Project: Rainbow Table Generator
*  File:   main.cpp
*  Author: Jason Papapanagiotakis
*  Github: https://github.com/JasonPap/Rainbow-Table-Generator
*/
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
    string x;
    RainbowTable* table = new RainbowTable(10000000, 6, 2);
    cout << "Threads used: " << Utils::hardwareConcurrency() << endl;
    //RainbowTable* table = new RainbowTable(10000, 6, 100);
    cout << "size before load: " << table->GetSize() << endl;
    /*
    RainbowTable::ucharVectorPtr shahash(new std::vector<unsigned char>());
    RainbowTable::ucharVectorPtr shaplain(new std::vector<unsigned char>());
    shahash->resize(SHA_DIGEST_LENGTH);
    std::string plain = "8oRNVg";
    shaplain->assign(plain.begin(), plain.end());
    simpleSHA1(shaplain, shahash);
    cout << "hash is === " << table->HashToStr(shahash) << endl;
    */
    table->LoadPasswords("passes.txt");
    table->CreateTable();
    cout << "table created, size: " << table->GetSize() << endl;
    //table->Print();
    table->Save("testtable_frompass.txt");
    //table->Load("testtable.txt");
    cout << "size after load: " << table->GetSize() << endl;
    cout << "give password hash to look for or exit to terminate" << endl;
    //std::string passToFind = "3cb10a562b8823ae690ea5a0aca0539d85edbcfbc8a2b751d6d78fe3173150a1cdba52fc07f1662383f87b94538e1eaefaa10d44f2ef940ae7428b589701056d";
    //cout << table->FindPasswordParallel(passToFind);
    do
    {
        cout << ">>";
        cin >> x;
        if (x == "exit")
            break;
        cout << "looking for hash now" << endl;
        cout << "FOUND: \"" << table->FindPasswordParallel(x) << "\"\n";
    } while (true);
    cout << "done" << endl;
}



