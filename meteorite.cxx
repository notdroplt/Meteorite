#include <iostream>
#include "meteorite_core.h"
#ifndef METEORITE_VERSION
#define METEORITE_VERSION ""
#endif

void print_help() {
    std::cout <<
    "Meteorite v" METEORITE_VERSION ": supernova assembler\n"
    "Usage: meteorite [flags] input [output]\n"
    "If not provided, output will be \"name_without_extension\".snv\n\n"
    "Flags:\n"
    " -h --help    | print this help message\n";
}



int main(int argc, char** argv) {

    if (argc < 2 || argv[1] == std::string_view("-h") || argv[1] == std::string_view("--help")) {
        print_help();
        return 0;
    }

    meteorite::generate_code(argv[1]);
}