/*************************************************************************
                          PLD Compilateur: CFG
                          ---------------------------
    start   : 26/03/2024
    authors : ANDRIANARISOLO Elie, FARHAT Widad,
			  SARR Seynabou, TONG An Jun
*************************************************************************/

//------ Interface of class <CFG> (file CFG.h) -------/
#pragma once

//--------------------------------------------------- Called interfaces
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <stack>
#include <initializer_list>
#include "BasicBlock.h"

//------------------------------------------------------------------------
//
// Goal of class <CFG> : 
//
// The CFG class represents the Control Flow Graph (CFG) of the program being compiled.
// It is responsible for generating the Assembly code (ASM) from the CFG.
//
//------------------------------------------------------------------------

class CFG 
{
	public:

		// Constructor
		CFG();

		// Destructor
		~CFG();
		
		// Method to generate Assembly code from the CFG
		void generateASM(ostream& o);

		// Method to create a new BasicBlock and add it to the CFG
		BasicBlock* createBB(); 
		
		// Method to initialize standard library functions in the CFG
		void initStandardFunctions(SymbolTable * symbolTable);

		// Getter for the current BasicBlock
		BasicBlock* getCurrentBB();
		
		// Setter for the current BasicBlock
		void setCurrentBB(BasicBlock * bb);

	protected:

		// Method to generate standard library functions in the Assembly code
		void generateStandardFunctions(ostream& o);
		
		// Method to generate the putchar function in the Assembly code
		void generatePutchar(ostream& o);
		
		// Method to generate the getchar function in the Assembly code
		void generateGetchar(ostream& o);

		// Method to generate the Assembly prologue
		void generateASMPrologue(ostream& o);

		// Method to generate the Assembly epilogue
		void generateASMEpilogue(ostream& o);
		
		// List of BasicBlocks in the CFG
		vector<BasicBlock*> bbList; 

		// Pointer to the current BasicBlock being processed
		BasicBlock* currentBB;
	
	private:

		// Flags to indicate if putchar and getchar functions need to be generated
        bool mustWritePutchar = false;
        bool mustWriteGetchar = false;
};
