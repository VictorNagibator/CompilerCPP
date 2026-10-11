#include <iostream>
#include <string>
#include <vector>
#include <exception>

#ifdef _WIN32
#include <windows.h>
#endif

#include "Diagram.h"

using namespace std;

static void printUsage(const char* program) {
    cout << "Usage: " << program << " [options] [input_file]\n"
        << "\n"
        << "Options:\n"
        << "  -h, --help        Show this help message and exit\n"
        << "  --no-interp       Only analyze the program and print the symbol tree\n"
        << "  --debug           Enable runtime diagnostics (default)\n"
        << "  --no-debug, -q    Disable runtime diagnostics\n"
        << "\n"
        << "If no input file is given, 'input.txt' is used.\n";
}

int main(int argc, char** argv) {
    // Use UTF-8 on the Windows console so all output is displayed correctly.
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    bool interp = true;
    bool debug = true;
    string fileName = "input.txt";
    vector<string> positional;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "--no-interp" || arg == "--analyze") {
            interp = false;
        }
        else if (arg == "--debug") {
            debug = true;
        }
        else if (arg == "--no-debug" || arg == "-q") {
            debug = false;
        }
        else if (!arg.empty() && arg[0] == '-') {
            cerr << "Error: unknown option '" << arg << "'\n";
            printUsage(argv[0]);
            return 2;
        }
        else {
            positional.push_back(arg);
        }
    }

    if (!positional.empty()) {
        fileName = positional[0];
    }

    Scanner sc;
    if (!sc.loadFile(fileName)) {
        cerr << "Error: cannot open file: " << fileName << endl;
        return 1;
    }

    try {
        Diagram dg(&sc);
        dg.ParseProgram(interp, debug);
    }
    catch (const std::exception&) {
        // Diagnostics have already been printed; report a non-zero exit code.
        return 1;
    }

    return 0;
}
