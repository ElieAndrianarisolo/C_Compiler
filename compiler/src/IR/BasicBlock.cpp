/*************************************************************************
                          PLD Compilateur: BasicBlock
                          ---------------------------
    start   : 26/03/2024
    authors : ANDRIANARISOLO Elie, FARHAT Widad,
			  SARR Seynabou, TONG An Jun
*************************************************************************/

//---- Implementation of class <BasicBlock> (file BasicBlock.cpp) -----/

#include "BasicBlock.h"

using namespace std;

// Destructor
BasicBlock::~BasicBlock() 
{
	// Delete all instructions in the instruction list
	for (IRInstr * i : instructionList) 
	{
		delete i;
	}
}

// Adds an instruction to the basic block
void BasicBlock::addInstruction(IRInstr::Operation op, vector<string> parameters, SymbolTable * sT) 
{
	// Create a new instruction and add it to the instruction list
	IRInstr * instruction = new IRInstr(this, op, parameters, sT);
	instructionList.push_back(instruction);
}

// Generates Assembly code for the basic block
void BasicBlock::generateASM(ostream &o) 
{
	// Write the label of the basic block
	o << label << ":" << endl;

	// Generate Assembly code for each instruction in the instruction list
	for (IRInstr * i : instructionList) 
	{
		i->generateASM(o);
	}
}

// Sets the true exit pointer of the basic block
void BasicBlock::setExitTrue(BasicBlock * bb) 
{
	this->exit_true = bb;
}

// Gets the true exit pointer of the basic block
BasicBlock* BasicBlock::getExitTrue()
{
	return this->exit_true;
}

// Sets the false exit pointer of the basic block
void BasicBlock::setExitFalse(BasicBlock * bb) 
{
	this->exit_false = bb;
}

// Gets the false exit pointer of the basic block
BasicBlock* BasicBlock::getExitFalse()
{
	return this->exit_false;
}

// Gets the label of the basic block
string BasicBlock::getLabel()
{
	return this->label;
}