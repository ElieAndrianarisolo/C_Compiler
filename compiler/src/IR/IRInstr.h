/*************************************************************************
                          PLD Compilateur: IRInstr
                          ---------------------------
    start   : 26/03/2024
    authors : ANDRIANARISOLO Elie, FARHAT Widad,
			  SARR Seynabou, TONG An Jun
*************************************************************************/

//------ Interface of class <IRInstr> (file IRInstr.h) -------/

//--------------------------------------------------- Called interfaces
#include "../SymbolTable.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <list>

class BasicBlock;

//------------------------------------------------------------------------
//
// Goal of class <IRInstr> : Define individual instructions for the intermediate representation (IR)
//
// This class represents individual instructions for the intermediate representation (IR).
// It defines various operations that can be performed in the IR, such as arithmetic operations,
// comparisons, control flow instructions, etc.
//
//------------------------------------------------------------------------

class IRInstr 
{
	public:

		// All allowed operations 
        typedef enum {
                        ldconst,            // Load constant value into register
                        copy,               // Copy value from one register to another
                        aff,                // Assign value to a variable
                        wparam,             // Write parameter value into stack
                        rparam,             // Read parameter value from stack
                        call,               // Call a function
                        cmp_eq,             // Compare if two values are equal
                        cmp_neq,            // Compare if two values are not equal
                        cmp_lt,             // Compare if one value is less than another
                        cmp_gt,             // Compare if one value is greater than another
                        cmp_eqlt,           // Compare if one value is equal to or less than another
                        cmp_eqgt,           // Compare if one value is equal to or greater than another
                        op_or,              // Bitwise OR operation
                        op_xor,             // Bitwise XOR operation
                        op_and,             // Bitwise AND operation
                        op_add,             // Addition operation
                        op_sub,             // Subtraction operation
                        op_mul,             // Multiplication operation
                        op_div,             // Division operation
                        op_mod,             // Modulo operation
                        op_not,             // Bitwise NOT operation
                        op_minus,           // Unary minus operation
                        op_plus_equal,      // Plus equal operation (e.g., a += b)
                        op_sub_equal,       // Minus equal operation (e.g., a -= b)
                        op_mult_equal,      // Multiply equal operation (e.g., a *= b)
                        op_div_equal,       // Divide equal operation (e.g., a /= b)
                        ret,                // Return from function
                        prologue,           // Prologue
                        conditional_jump,   // Conditional jump instruction
                        absolute_jump       // Unconditional jump instruction
                    } Operation;

		// Constructor
		IRInstr(BasicBlock * bb, IRInstr::Operation op, vector<string> parameters, SymbolTable * sT);
		
		// Generate Assembly code for the instruction
		void generateASM(ostream &o); 

		// Getter for the associated symbol table
		SymbolTable* getSymbolTable()
		{ 
			return symbolTable; 
		};

		// Getter for the operation
		Operation getOp() 
		{ 
			return op; 
		};

		// Getter for the parameters
        vector<string> getParameters() 
		{ 
			return parameters;
		};

		// Mapping of parameter registers for x86 architecture
		static unordered_map<string, vector<string>> AMD86_paramRegisters;

	private:

		BasicBlock* bb; 			// The BB this instruction belongs to, which provides a pointer to the CFG this instruction belong to
		Operation op;				// Operator of the instruction
		SymbolTable* symbolTable; 	// Associated symbol table
		vector<string> parameters; 	// Parameters of the instructions (typically src, dest, tmpVar)
};