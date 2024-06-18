/*************************************************************************
                          PLD Compilateur: Main
                          ---------------------------
    start   : 26/03/2024
    authors : ANDRIANARISOLO Elie, FARHAT Widad,
			  SARR Seynabou, TONG An Jun
*************************************************************************/

//---- Implementation of class <Main> (file main.cpp) -----/

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

// Include ANTLR4 headers
#include "antlr4-runtime.h"
#include "../generated/ifccLexer.h"
#include "../generated/ifccParser.h"
#include "../generated/ifccBaseVisitor.h"

// Include custom headers
#include "CodeGenVisitor.h"
#include "IR/CFG.h"

using namespace antlr4;
using namespace std;

// Main function
int main(int argn, const char **argv)
{
    // String stream to hold input source code
    stringstream in;

    // Check if the correct number of arguments is provided
    if (argn==2)
    {
        // Open the file for reading
        ifstream lecture(argv[1]);

        // Check if the file is opened successfully
        if( !lecture.good() )
        {
            cerr<<"error: cannot read file: " << argv[1] << endl ;
            exit(1);
        }

        // Read the contents of the file into the stringstream
        in << lecture.rdbuf();
    }
    else
    {
        cerr << "usage: ifcc path/to/file.c" << endl ;
        exit(1);
    }

    // Parse and construct tree
    ANTLRInputStream input(in.str());

    // Create lexer and token stream
    ifccLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    tokens.fill();

    // Check the syntax by parsing
    ifccParser parser(&tokens);
    tree::ParseTree* tree = parser.axiom();

    // Check for syntax errors
    if(parser.getNumberOfSyntaxErrors() != 0)
    {
        cerr << "ERROR: syntax error during parsing" << endl;
        cout.flush();
        exit(1);
    }

    // Create an error handler and a Control Flow Graph (CFG)
    ErrorHandler errorHandler;
    CFG cfg;

    // Visit the parse tree and generate intermediate representation (IR) code
    CodeGenVisitor v(errorHandler, cfg);
    v.visit(tree);

    // Perform static analysis on functions
    v.getGlobalSymbolTable()->checkUsedFunctions(errorHandler);

    // Check for errors and exit if present
    bool hasErrors = errorHandler.hasError();
    if (hasErrors)
    {
        cout.flush();
        exit(1);
    }

    // Place standard functions in the code if needed
    cfg.initStandardFunctions(v.getGlobalSymbolTable());

    // Generate ASM instructions
    stringstream out;
    cfg.generateASM(out);
    
    // Output the generated assembly code
    cout << out.str();
    
    return 0;
}