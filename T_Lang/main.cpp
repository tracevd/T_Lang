#pragma once

#include <iostream>
#include <fstream>

#include "T/Parser.h"

int main( int c, char** argv )
{
    std::string program_str;
    std::ifstream input( "C:/dev/T_Lang/CPP/T_Lang/test_lang.t", std::ios::in );

    if ( input.fail() )
    {
        std::cout << "Failed to open test file\n";
        exit( 1 );
    }

    for ( std::string line; getline( input, line ); )
    {
        program_str += line + '\n';
    }

    std::cout << program_str << '\n';

    t::Lexer lex { std::move( program_str ) };

    t::Parser p { lex.tokenize() };

    auto program = p.produceAST();

    std::cout << "Program AST:\n";

    program.print();

    return 0;
}