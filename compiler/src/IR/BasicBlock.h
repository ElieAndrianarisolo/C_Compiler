/*************************************************************************
                          PLD Compilateur: BasicBlock
                          ---------------------------
    start   : 26/03/2024
    authors : ANDRIANARISOLO Elie, FARHAT Widad,
			  SARR Seynabou, TONG An Jun
*************************************************************************/

//------ Interface of class <BasicBlock> (file BasicBlock.h) -------/

//--------------------------------------------------- Called interfaces
#include <vector>
#include <string>
#include <list>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include <unordered_set>
#include <algorithm>
#include "../SymbolTable.h"
#include "IRInstr.h"

class CFG;

using namespace std;

//------------------------------------------------------------------------
//
// Goal of class <BasicBlock> : 
//
// Represents a basic block in the control flow graph (CFG).
// A basic block consists of a sequence of instructions and connections
// to other basic blocks.
//
//------------------------------------------------------------------------

class BasicBlock 
{
	public:

		// Constructor.
		BasicBlock(CFG * cfg, string label) : cfg(cfg), label(label) {};
		
		// Destructor.
		~BasicBlock();

		// Generate Assembly code for the basic block.
		void generateASM(ostream &o); 

		// Add an instruction to the basic block.
		void addInstruction(IRInstr::Operation op, vector<string> parameters, SymbolTable * sT);

		// Set the exit true pointer of the basic block.
		void setExitTrue(BasicBlock * bb);
		
		// Get the exit true pointer of the basic block.
		BasicBlock* getExitTrue();
		
		// Set the exit false pointer of the basic block.
		void setExitFalse(BasicBlock * bb);
		
		// Get the exit false pointer of the basic block.
		BasicBlock* getExitFalse();

		// Set the name of the test variable associated with this basic block.
		void setTestVariableName(string n) 
		{ 
			testVariableName = n;
		};

		// Get the name of the test variable associated with this basic block.
		string getTestVariableName() 
		{ 
			return testVariableName; 
		};

		// Get the list of instructions in this basic block.
        list<IRInstr*> getInstructionList() 
		{ 
			return instructionList; 
		};

		// Get the CFG containing this basic block.
		CFG* getCFG() 
		{ 
			return cfg; 
		};

		// Get the label of this basic block.
		string getLabel();

	protected:

		BasicBlock* exit_true = nullptr;   	// Pointer to the true exit of this bloc
		BasicBlock* exit_false = nullptr; 	// Pointer to the false exit of this block
		string label; 						// Label of the basic block
		CFG* cfg; 							// Pointer to the CFG containing this basic block
		list<IRInstr*> instructionList;		// List of instructions in this basic block
		string testVariableName;			// Name of the test variable associated with this basic block
};