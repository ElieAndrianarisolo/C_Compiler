/*************************************************************************
                          PLD Compilateur: SymbolTable
                          ---------------------------
    start   : 26/03/2024
    authors : ANDRIANARISOLO Elie, FARHAT Widad,
			  SARR Seynabou, TONG An Jun
*************************************************************************/

//------ Interface of class <SymbolTable> (file SymbolTable.h) -------/
#pragma once

//--------------------------------------------------- Called interfaces
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_set>
#include "ErrorHandler.h"

using namespace std;

//------------------------------------------------------------------ Types

// Structure to represent variables in the symbol table
struct varStruct 
{
	string variableName; 			// Variable name
	int memoryOffset;				// Offset (in memory) to the base pointer 
	string variableType;			// Type of the variable
	int variableLine;				// Line of code where the variable is declared
	bool isUsed;					// Whether the variable is used in the code
	bool isCorrect; 				// False when a stupid struct is returned to avoid bad cast
};

// Structure to represent functions in the symbol table
struct funcStruct 
{
	string functionName;			// Function
	string returnType; 				// Return type 
	int nbParameters;				// Number of input parameters
	vector<string> parameterTypes;	// Type of every input parameter
	vector<string> parameterNames; 	// Names of every parameter
	int functionLine; 				// The line of code where the function is declared
	bool isCalled;					// Whether the function is called or not
};

//------------------------------------------------------------------------
// 
// Goal of class <SymbolTable> :
//
// The goal of this class is to store all symbols encountered while
// parsing a code using a subset of C.
//
//------------------------------------------------------------------------

class SymbolTable 
{
	public:

		// Constructor
		SymbolTable(int sP = 0, SymbolTable* parent = nullptr) : stackPointer(sP), parentSymbolTable(parent) 
        {
			if (parentSymbolTable != nullptr) 
			{
				parentSymbolTable->childSymbolTables.push_back(this);
			}
		}

		// Check if a variable with a given name exists in the current symbol table
		int hasVariable(string name);

		// Check if a parameter with a given name exists in the current symbol table
		int hasParameter(string name);

		// Check if a function with a given name exists in the current symbol table
		bool hasFunction(string name);

		// Retrieve the variable with a given name from the symbol table
		varStruct* getVariable(string name, bool searchParents = true);

		// Retrieve the function with a given name from the symbol table
		funcStruct* getFunction(string name);
		
		// Add a variable to the symbol table
		void addVariable(string name, string variableType, int lineNumber);
		
		// Add a function to the symbol table
		void addFunction(string name, string returnType, int nbParameters, vector<string> parametersTypes, vector<string> parametersNames, int functionLine);
		
		// Get the parent symbol table
		SymbolTable* getParent();

		// Check if a return statement has been encountered in the current function scope
		bool hasReturned();

		// Set whether a return statement has been encountered in the current function scope
		void setReturned(bool returnStatement);

		// Get the stack pointer value
		int getStackPointer();

		// Set the stack pointer value
		void setStackPointer(int s);

		// Get the memory space allocated for variables in the current function scope
		int getMemorySpace();
		
		// Check for unused variables and report errors using the provided error handler
		void checkUsedVariables(ErrorHandler& errorHandler);

		// Check for unused functions and report errors using the provided error handler
		void checkUsedFunctions(ErrorHandler& errorHandler);

		// Perform type casting for a given type and value
		static int getCast(string type, int value);

		// Static member to store type sizes
		static unordered_map<string, int> typeSizes;

		// Static member to store type operation moves
		static unordered_map<string, string> typeOpeMoves;

		// Static member representing a dummy variable structure used for error handling
		static varStruct stupidVarStruct;

	protected:

		int stackPointer;								// The current position of the memory stack pointer 
		bool hasReturnStatement; 						// Whether the scope has a return statement
		SymbolTable* parentSymbolTable;					// Pointer to the parent symbol table
		vector<SymbolTable*> childSymbolTables; 		// Vector storing children symbol tables
		unordered_map<string, varStruct> variableMap; 	// Hashtable containing the encountered variable declarations
		unordered_map<string, funcStruct> functionMap; 	// Hashtable containing the encountered function declarations
};