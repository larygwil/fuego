//----------------------------------------------------------------------------
/** @file FuegoMain.cpp
    Main function for Fuego
*/
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include <iostream>
#include "FuegoMainEngine.h"
#include "FuegoMainUtil.h"
#include "GoInit.h"
#include "SgCmdLineOpt.h"
#include "SgDebug.h"
#include "SgException.h"
#include "SgInit.h"

using namespace std;

//----------------------------------------------------------------------------

namespace {

/** @name Settings from command line options */
// @{

bool g_noHandicap;

bool g_quiet;

int g_fixedBoardSize;

int g_maxGames;

string g_config;

const char* g_programPath;

int g_srand;

// @} // @name

void MainLoop()
{
    FuegoMainEngine engine(cin, cout, g_fixedBoardSize, g_programPath,
                           g_noHandicap);
    GoGtpAssertionHandler assertionHandler(engine);
    if (g_maxGames >= 0)
        engine.SetMaxClearBoard(g_maxGames);
    if (g_config != "")
        engine.ExecuteFile(g_config);
    engine.MainLoop();
}

void ParseOptions(int argc, char** argv)
{
    SgCmdLineOpt opt;
    vector<string> specs;
    specs.push_back("config:");
    specs.push_back("help");
    specs.push_back("maxgames:");
    specs.push_back("nohandicap");
    specs.push_back("quiet");
    specs.push_back("srand:");
    specs.push_back("size:");
    opt.Parse(argc, argv, specs);
    if (opt.GetArguments().size() > 0)
        throw SgException("No arguments allowed");
    if (opt.Contains("help"))
    {
        cout <<
            "Options:\n"
            "  -config file execute GTP commands from file before\n"
            "               starting main command loop\n"
            "  -help        display this help and exit\n"
            "  -maxgames n  make clear_board fail after n invocations\n"
            "  -nohandicap  don't support handicap commands\n"
            "  -quiet       don't print debug messages\n"
            "  -size        initial (and fixed) board size\n"
            "  -srand       set random seed (-1:none, 0:time(0))\n";
        exit(0);
    }
    g_config = opt.GetString("config", "");
    g_maxGames = opt.GetInteger("maxgames", -1);
    g_quiet = opt.Contains("quiet");
    g_noHandicap = opt.Contains("nohandicap");
    // Don't be deterministic by default (0 means non-deterministic seed)
    g_srand = opt.GetInteger("srand", 0);
    g_fixedBoardSize = opt.GetInteger("size", 0);
}

void PrintStartupMessage()
{
    SgDebug()
        << "Fuego " << FuegoMainUtil::Version() << '\n' <<
        "Copyright by the authors of the Fuego project.\n"
        "See http://fuego.sf.net for information about Fuego. Fuego comes\n"
        "with NO WARRANTY to the extent permitted by law. This program is\n"
        "free software; you can redistribute it and/or modify it under the\n"
        "terms of the GNU Lesser General Public License as published by the\n"
        "Free Software Foundation - version 3. For more information about\n"
        "these matters, see the files named COPYING and COPYING.LESSER\n"
        "\n";
}

} // namespace

//----------------------------------------------------------------------------

int main(int argc, char** argv)
{
    if (argc > 0 && argv != 0)
    {
        g_programPath = argv[0];
        try
        {
            ParseOptions(argc, argv);
        }
        catch (const SgException& e)
        {
            SgDebug() << e.what() << "\n";
            return 1;
        }
    }
    if (g_quiet)
        SgDebugToNull();
    try
    {
        SgInit();
        GoInit();
        PrintStartupMessage();
        SgRandom::SetSeed(g_srand);
        MainLoop();
        GoFini();
        SgFini();
    }
    catch (const GtpFailure& e)
    {
        SgDebug() << e.Response() << '\n';
        return 1;
    }
    catch (const std::exception& e)
    {
        SgDebug() << e.what() << '\n';
        return 1;
    }
    return 0;
}

//----------------------------------------------------------------------------
