/*************************************************************************
                          PLD Compilateur: CFG
                          ---------------------------
    start   : 26/03/2024
    authors : ANDRIANARISOLO Elie, FARHAT Widad,
			  SARR Seynabou, TONG An Jun
*************************************************************************/

//---- Implementation of class <CFG> (file CFG.cpp) -----/

#include "CFG.h"

using namespace std;

// Constructor
CFG::CFG() 
{
	setCurrentBB(createBB());
}

// Destructor
CFG::~CFG() 
{
	for (BasicBlock * bb : bbList) 
	{
		delete bb;
	}
}

// Method to initialize standard library functions in the CFG
void CFG::initStandardFunctions(SymbolTable * symbolTable) 
{
	// Get function information for putchar and getchar
	funcStruct * funcPutchar = symbolTable->getFunction("putchar");
	funcStruct * funcGetchar = symbolTable->getFunction("getchar");

	// Set flags indicating whether putchar and getchar are called
	this->mustWritePutchar = funcPutchar->isCalled;
	this->mustWriteGetchar = funcGetchar->isCalled;
}

// Method to create a new BasicBlock and add it to the CFG
BasicBlock* CFG::createBB() 
{
	// Generate a unique name for the BasicBlock
	string bbName = ".bb" + to_string(bbList.size());

	// Create a new BasicBlock
	BasicBlock * bb = new BasicBlock(this, bbName);

	// Add the BasicBlock to the list of BasicBlocks
	bbList.push_back(bb);

	// Set the current BasicBlock
	currentBB = bb;

	return bb;
}

// Method to generate Assembly code from the CFG
void CFG::generateASM(ostream& o) 
{
	// Generate the Assembly prologue
	generateASMPrologue(o);

	// Generate standard library functions
	generateStandardFunctions(o);

	// Generate Assembly code for each BasicBlock in the CFG
	for (BasicBlock * bb : bbList) 
	{
		bb->generateASM(o);
	}
}

// Method to generate the Assembly prologue
void CFG::generateASMPrologue(ostream& o) 
{
	o << ".text" << endl;
}

// Getter for the current BasicBlock
BasicBlock* CFG::getCurrentBB() 
{
	return currentBB;
}

// Setter for the current BasicBlock
void CFG::setCurrentBB(BasicBlock * bb) 
{
	currentBB = bb;
}

// Method to generate standard library functions in the Assembly code
void CFG::generateStandardFunctions(ostream& o) 
{
	// Generate putchar function if required
	if (this->mustWritePutchar) 
	{
		this->generatePutchar(o);
	}

	// Generate getchar function if required
	if (this->mustWriteGetchar) 
	{
		this->generateGetchar(o);
	}
}

// Method to generate Assembly code for putchar function
void CFG::generatePutchar(ostream& o) 
{
	o << "putchar:" << endl;
	o << "\tpushq\t %rbp" << endl;
	o << "\tmovq\t %rsp, %rbp" << endl;
	o << "\tpushq\t	%rdi" << endl;
	o << "\tmov\t $1, %rax" << endl;
	o << "\tmov\t $1, %rdi" << endl;
	o << "\tmov\t %rsp, %rsi" << endl;
	o << "\tmov\t $1, %rdx" << endl;
	o << "\tsyscall" << endl;
	o << "\tadd\t $8, %rsp" << endl;
	o << "\tmovl\t $1, %eax" << endl;
	o << "\tleave" << endl;
	o << "\tret" << endl << endl;
}

// Method to generate Assembly code for getchar function
void CFG::generateGetchar(ostream& o) 
{
	o << "getchar:" << endl;
	o << "\tpushq\t %rbp" << endl;
	o << "\tmovq\t %rsp, %rbp" << endl;
	o << "\txor\t %eax, %eax" << endl;
	o << "\txor\t %edi, %edi" << endl;
	o << "\tmovq\t 8(%rsp), %r8" << endl;
	o << "\tlea\t 8(%rsp), %rsi" << endl;
	o << "\tmovl\t $1, %edx" << endl;
	o << "\tsyscall" << endl;
	o << "\tmovzbl\t 8(%rsp), %eax" << endl;
	o << "\tmovq\t %r8, 8(%rsp)" << endl;
	o << "\tleave" << endl;
	o << "\tret" << endl << endl;
}