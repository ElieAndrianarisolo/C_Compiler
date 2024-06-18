/*************************************************************************
                          PLD Compilateur: SymbolTable
                          ---------------------------
    start   : 26/03/2024
    authors : ANDRIANARISOLO Elie, FARHAT Widad,
			  SARR Seynabou, TONG An Jun
*************************************************************************/

//---- Implementation of class <SymbolTable> (file SymbolTable.cpp) -----/

#include "SymbolTable.h"

using namespace std;

// Static initialization of type sizes for known data types
unordered_map<string, int> SymbolTable::typeSizes = {{"int" , 4}, {"char", 1}};

// Static initialization of type operation moves for known data types
unordered_map<string, string> SymbolTable::typeOpeMoves = {{"int", "movl"}, {"char", "movzbl"}};

// Static initialization of a dummy variable structure for error handling
varStruct SymbolTable::stupidVarStruct = {"", 0, "", 0, false, false};

// Check if a variable with a given name exists in the current symbol table or its parent tables
int SymbolTable::hasVariable(string name) 
{
	// Check if the variable exists in the current symbol table
	bool hasVariableInOwnMap = variableMap.find(name) != variableMap.end();
	
	// Check if the variable exists in the parent symbol table
	bool hasVariableInParentMap = (parentSymbolTable != nullptr && parentSymbolTable->hasVariable(name) > 0);
	
	int level = 0;
	
	// Determine the level of existence of the variable
	if (hasVariableInOwnMap) 
	{
		level = 1;
	}
	else if (hasVariableInParentMap) 
	{
		level = 2;
	}

	return level;
}

// Check if a parameter with a given name exists in the current symbol table
int SymbolTable::hasParameter(string name) 
{
	return hasVariable("°"+name); // Prefix the parameter name with the special character ° for identification
}

// Check if a function with a given name exists in the current symbol table or its parent tables
bool SymbolTable::hasFunction(string name)
{
	// Check if the function exists in the current symbol table
	bool hasFunctionInOwnMap = functionMap.find(name) != functionMap.end();

	// Check if the function exists in the parent symbol table
	bool hasFunctionInParentMap = (parentSymbolTable != nullptr && parentSymbolTable->hasFunction(name));
	
	// Return true if the function exists in either the current or parent symbol table
    return hasFunctionInOwnMap || hasFunctionInParentMap;
}

// Retrieve the variable with a given name from the symbol table (searching parent tables if specified)
varStruct* SymbolTable::getVariable(string name, bool searchParents) 
{
	// Check if the variable is a parameter and handle it accordingly
	if (hasParameter(name)) 
	{
		return getVariable("°"+name);
	}

	// Check if the variable exists in the current symbol table
	bool hasVariableInOwnMap = variableMap.find(name) != variableMap.end();
	
	if (hasVariableInOwnMap) // Return the variable if found in the current symbol table
	{
		return &variableMap[name];
	}
	else if(searchParents) 	// Search parent symbol tables if specified
	{
		return parentSymbolTable->getVariable(name);
	} 
	else 
	{
		return nullptr;
	}
}

// Retrieve the function with a given name from the symbol table (searching parent tables)
funcStruct* SymbolTable::getFunction(string name) 
{
	// Check if the function exists in the current symbol table
	bool hasFunctionInOwnMap = functionMap.find(name) != functionMap.end();
	
	if (hasFunctionInOwnMap) // Return the function if found in the current symbol table
	{
		return &functionMap[name];
	}
	else // Search parent symbol tables
	{
		return parentSymbolTable->getFunction(name);
	}
}

// Compute the total memory space allocated for variables in the symbol table and its children
int SymbolTable::getMemorySpace() 
{
	int memSize = 0;

	// Compute memory size of own symbol table
	for (auto variable : variableMap) 
	{
		memSize += typeSizes[variable.second.variableType];
	}

	// Add memory size of child symbol tables
	for (SymbolTable* sT : childSymbolTables) 
	{
		memSize += sT->getMemorySpace();
	}

	return memSize;
}

// Add a variable to the symbol table
void SymbolTable::addVariable(string name, string variableType, int lineNumber) 
{
	// Decrement the stack pointer based on the size of the variable type
	stackPointer -= typeSizes[variableType];

	// Create a variable structure and add it to the map
	struct varStruct s = {
                            name,
                            stackPointer,
                            variableType,
							lineNumber,
							false,
							true
                          };

	variableMap[name] = s;
}

// Add a function to the symbol table
void SymbolTable::addFunction(string name, string returnType, int nbParameters, vector<string> parametersType, vector<string> parametersNames, int functionLine) 
{
	// Create a function structure and add it to the map
	struct funcStruct function = {
									name,
									returnType,
									nbParameters,
									parametersType,
									parametersNames,
									functionLine,
									false
								 };
	
	functionMap[name] = function;
}

// Get the parent symbol table
SymbolTable* SymbolTable::getParent() 
{ 
	return parentSymbolTable; 
}

// Check if a return statement has been encountered in the current function scope
bool SymbolTable::hasReturned() 
{ 
	return hasReturnStatement;
}

// Set whether a return statement has been encountered in the current function scope
void SymbolTable::setReturned(bool returnStatement) 
{
	hasReturnStatement = returnStatement;
}

// Get the stack pointer value
int SymbolTable::getStackPointer() 
{
	return stackPointer; 
}

// Set the stack pointer value
void SymbolTable::setStackPointer(int s) 
{ 
	stackPointer = s; 
}

// Check for unused variables and report errors using the provided error handler
void SymbolTable::checkUsedVariables(ErrorHandler& errorHandler) 
{
	for (auto variable : variableMap) 
	{
		if (!variable.second.isUsed) 
		{
			string message = "";

			if (variable.first[0] == '^') 
			{
				message =  "Parameter '" + variable.first.substr(1) + "' is not used";
			}
			else 
			{
				message =  "Variable '" + variable.first + "' declared at line " + to_string(variable.second.variableLine) + " is not used";
			}

			errorHandler.signal(WARNING, message, -1);
		}
	}
}

// Check for unused functions and report errors using the provided error handler
void SymbolTable::checkUsedFunctions(ErrorHandler& errorHandler) 
{
	// Set of global functions that are considered always used
	unordered_set<string> globalFunctions = {"main", "putchar", "getchar"};
	
	for (auto variable : functionMap) 
	{
		 // Skip global functions
		if (globalFunctions.find(variable.second.functionName) != globalFunctions.end()) 
		{ 
			continue; 
		}

		// Report if the function is not called
		if (!variable.second.isCalled) 
		{
			string message =  "Function '" + variable.first + "' declared at line " + to_string(variable.second.functionLine) + " is not used";
			errorHandler.signal(WARNING, message, -1);
		}
	}
}

// Perform type casting for a given type and value
int SymbolTable::getCast(string type, int value) 
{
	// Perform casting based on the provided type
	if(!type.compare("int"))
    {
		return (int) value;
	} 
	else if (!type.compare("char"))
	{
		return (char) value;
	} 
    
	// If the type is unknown, return the original value
	return value;
}

