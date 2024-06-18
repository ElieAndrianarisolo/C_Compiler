/*************************************************************************
                          PLD Compilateur: IRInstr
                          ---------------------------
    start   : 26/03/2024
    authors : ANDRIANARISOLO Elie, FARHAT Widad,
			  SARR Seynabou, TONG An Jun
*************************************************************************/

//---- Implementation of class <IRInstr> (file IRInstr.cpp) -----/

#include "IRInstr.h"

class BasicBlock;

using namespace std;

unordered_map<string, vector<string>> IRInstr::AMD86_paramRegisters = {{"int", {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"}}, {"char", {"%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"}}};

// Constructor for IR instruction
IRInstr::IRInstr(BasicBlock * bb, IRInstr::Operation op, vector<string> parameters, SymbolTable * sT) : bb(bb), op(op), parameters(parameters), symbolTable(sT) {}

// Generate assembly code for the IR instruction
void IRInstr::generateASM(ostream &o)
{
	switch (op)
	{
		// Load constant value into register
		case IRInstr::ldconst:
		{
			// Get parameters
			string constant = parameters.at(1);
			string variableName = parameters.at(2);
			string variableType = parameters.at(0);

			// Get constant value
			int constValue = stoi(constant);
			string movInstr;

			varStruct * variable = symbolTable->getVariable(variableName);
			bool isChar = variableType == "char";
			movInstr = isChar ? "movb" : "movl";

			// Write ASM instructions
			o << "\t" << movInstr << "\t $" << SymbolTable::getCast(variable->variableType, constValue) << ", " << variable->memoryOffset << "(%rbp)"
			  << "\t\t# [ldconst] load " << constValue << " into " << variableName << endl;

			break;
		}

		case IRInstr::aff: // Assign value to a variable
		case IRInstr::copy: // Copy value from one register to another
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);

			string movInstr1, movInstr2;
			string reg;

			if (variable2->variableType == "int" && variable1->variableType == "int")
			{
				// int = int : movl
				movInstr1 = "movl";
				movInstr2 = "movl";
				reg = "eax";
			}
			else if (variable2->variableType == "char" && variable1->variableType == "char")
			{
				// char = char : movb
				movInstr1 = "movb";
				movInstr2 = "movb";
				reg = "al";
			}
			else if (variable2->variableType == "char" && variable1->variableType == "int")
			{
				// char = int : movb
				movInstr1 = "movb";
				movInstr2 = "movb";
				reg = "al";
			}
			else if (variable2->variableType == "int" && variable1->variableType == "char")
			{
				// int = char : movzbl
				movInstr1 = "movzbl";
				movInstr2 = "movl";
				reg = "eax";
			}
			
			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable1->memoryOffset << "(%rbp), %" << reg
			  << "\t\t# [copy/aff] load " << variableName1 << " into " << "%" << reg << endl;
			o << "\t" << movInstr2 << "\t %" << reg << ", " << variable2->memoryOffset << "(%rbp)"
			  << "\t\t# [copy/aff] load " << "%" << reg << " into " << variableName2 << endl;

			break;
		}

		// Bitwise NOT operation
		case IRInstr::op_not:
		{
			// Get parameters
			string variableName = parameters.at(0);
			string tmpName = parameters.at(1);

			varStruct * variable = symbolTable->getVariable(variableName);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(tmp->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable->memoryOffset << "(%rbp), %eax"
              << "\t\t# [op_not] load " << variable->variableName << " into " << "%eax" << endl;
            o << "\tcmpl\t $0, %eax" << endl;
            o << "\tsete\t %al" << endl;
            o << "\tmovzbl\t %al, %eax" << endl;
            o << "\t" << movInstr2 << "\t %eax, " << tmp->memoryOffset << "(%rbp)"
        	  << "\t\t# [op_not] load " << "%eax" << " into " << tmp->variableName << endl;

			break;
		}

		// Unary minus operation
		case IRInstr::op_minus:
		{
			// Get parameters
			string variableName = parameters.at(0);
			string tmpName = parameters.at(1);

			varStruct * variable = symbolTable->getVariable(variableName);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(tmp->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable->memoryOffset << "(%rbp), %eax"
              << "\t\t# [op_minus] load " << variable->variableName << " into " << "%eax" << endl;
            o << "\tnegl\t %eax" << endl;
            o << "\t" << movInstr2 << "\t %eax, " << tmp->memoryOffset << "(%rbp)"
        	  << "\t\t# [op_minus] load " << "%eax" << " into " << tmp->variableName << endl;

			break;
		}

		// Addition operation
		case IRInstr::op_add:
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);
			string tmpName = parameters.at(2);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable1->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(variable2->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable1->memoryOffset << "(%rbp), %eax"
			  << "\t\t# [op_add] load " << variableName1 << " into " << "%eax" << endl;
			o << "\t" << movInstr2 << "\t " << variable2->memoryOffset << "(%rbp), %edx"
			  << "\t\t# [op_add] load " << variableName2 << " into " << "%edx" << endl;
			o << "\t\taddl\t %edx, %eax" << endl;
			o << "\tmovl\t %eax, " << tmp->memoryOffset << "(%rbp)"
			  << "\t\t# [op_add] load " << "%eax" << " into " << tmpName << endl;

			break;
		}

		// Subtraction operation
		case IRInstr::op_sub:
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);
			string tmpName = parameters.at(2);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable1->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(variable2->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable1->memoryOffset << "(%rbp), %eax"
			  << "\t\t# [op_sub] load " << variableName1 << " into " << "%eax" << endl;
			o << "\t" << movInstr2 << "\t " << variable2->memoryOffset << "(%rbp), %edx"
			  << "\t\t# [op_sub] load " << variableName2 << " into " << "%edx" << endl;
			o << "\tsubl\t %edx, %eax" << endl;
			o << "\tmovl\t %eax, " << tmp->memoryOffset << "(%rbp)"
			  << "\t\t# [op_sub] load " << "%eax" << " into " << tmpName << endl;

			break;
		}

		// Multiplication operation
		case IRInstr::op_mul:
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);
			string tmpName = parameters.at(2);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable1->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(variable2->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable1->memoryOffset << "(%rbp), %eax"
              << "\t\t# [op_mul] load " << variableName1 << " into " << "%eax" << endl;
            o << "\t" << movInstr2 << "\t " << variable2->memoryOffset << "(%rbp), %edx"
              << "\t\t# [op_mul] load " << variableName2 << " into " << "%edx" << endl;
            o << "\timull\t %edx, %eax" << endl;
            o << "\tmovl\t %eax, " << tmp->memoryOffset << "(%rbp)"
              << "\t\t# [op_mul] load " << "%eax" << " into " << tmpName << endl;

			break;
		}

		// Division operation
		case IRInstr::op_div:
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);
			string tmpName = parameters.at(2);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable1->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(variable2->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable1->memoryOffset << "(%rbp), %eax"
              << "\t\t# [op_div] load " << variableName1 << " into " << "%eax" << endl;
            o << "\t" << movInstr2 << "\t " << variable2->memoryOffset << "(%rbp), %edx"
              << "\t\t# [op_div] load " << variableName2 << " into " << "%edx" << endl;
            o << "\tcltd" << endl;
            o << "\tidivl\t " << variable2->memoryOffset << "(%rbp)" << endl;
            o << "\tmovl\t %eax, " << tmp->memoryOffset << "(%rbp)"
              << "\t\t# [op_div] load " << "%eax" << " into " << tmpName << endl;

			break;
		}

		// Modulo operation
		case IRInstr::op_mod:
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);
			string tmpName = parameters.at(2);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable1->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(variable2->variableType);

			if(variable1->variableType == "char")
			{
				movInstr1 = "movsbl";
			}

			if(variable2->variableType == "char")
			{
				movInstr2 = "movsbl";
			}

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable1->memoryOffset << "(%rbp), %eax"
              << "\t\t# [op_mod] load " << variableName1 << " into " << "%eax" << endl;
            o << "\t" << movInstr2 << "\t " << variable2->memoryOffset << "(%rbp), %ebx"
              << "\t\t# [op_mod] load " << variableName2 << " into " << "%ebx" << endl;
            o << "\tcltd" << endl;
            o << "\tidivl\t %ebx" << endl;
            o << "\tmovl\t %edx, " << tmp->memoryOffset << "(%rbp)"
              << "\t\t# [op_mod] load " << "%eax" << " into " << tmpName << endl;

			break;
		}

		// Compare if two values are equal
		case IRInstr::cmp_eq:
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);
			string tmpName = parameters.at(2);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable1->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(variable2->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable1->memoryOffset << "(%rbp), %eax" << endl;
            o << "\t" << movInstr2 << "\t " << variable2->memoryOffset << "(%rbp), %edx" << endl;
            o << "\tcmpl\t %edx, %eax" << endl;
            o << "\tsete\t %al" << endl;
            o << "\tmovzbl\t %al, %eax" << endl;
            o << "\tmovl\t %eax, " << tmp->memoryOffset << "(%rbp)" << endl;

			break;
		}

		// Compare if two values are not equal
		case IRInstr::cmp_neq:
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);
			string tmpName = parameters.at(2);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable1->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(variable2->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable1->memoryOffset << "(%rbp), %eax" << endl;
		    o << "\t" << movInstr2 << "\t " << variable2->memoryOffset << "(%rbp), %edx" << endl;
		    o << "\tcmpl\t %edx, %eax" << endl;
		    o << "\tsetne\t %al" << endl;
		    o << "\tmovzbl\t %al, %eax" << endl;
		    o << "\tmovl\t %eax, " << tmp->memoryOffset << "(%rbp)" << endl;

			break;
		}

		// Compare if one value is less than another
		case IRInstr::cmp_lt:
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);
			string tmpName = parameters.at(2);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable1->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(variable2->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable1->memoryOffset << "(%rbp), %eax" << endl;
		    o << "\t" << movInstr2 << "\t " << variable2->memoryOffset << "(%rbp), %edx" << endl;
		    o << "\tcmpl\t %edx, %eax" << endl;
		    o << "\tsetl\t %al" << endl;
		    o << "\tmovzbl\t %al, %eax" << endl;
		    o << "\tmovl\t %eax, " << tmp->memoryOffset << "(%rbp)" << endl;
		
			break;
		}

		// Compare if one value is greater than another
		case IRInstr::cmp_gt:
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);
			string tmpName = parameters.at(2);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable1->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(variable2->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable1->memoryOffset << "(%rbp), %eax" << endl;
		    o << "\t" << movInstr2 << "\t " << variable2->memoryOffset << "(%rbp), %edx" << endl;
		    o << "\tcmpl\t %edx, %eax" << endl;
		    o << "\tsetg\t %al" << endl;
		    o << "\tmovzbl\t %al, %eax" << endl;
		    o << "\tmovl\t %eax, " << tmp->memoryOffset << "(%rbp)" << endl;
            
			break;
		}

		// Compare if one value is equal to or less than another
		case IRInstr::cmp_eqlt:
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);
			string tmpName = parameters.at(2);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable1->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(variable2->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable1->memoryOffset << "(%rbp), %eax" << endl;
		    o << "\t" << movInstr2 << "\t " << variable2->memoryOffset << "(%rbp), %edx" << endl;
		    o << "\tcmpl\t %edx, %eax" << endl;
		    o << "\tsetle\t %al" << endl;
		    o << "\tmovzbl\t %al, %eax" << endl;
		    o << "\tmovl\t %eax, " << tmp->memoryOffset << "(%rbp)" << endl;
            
			break;
		}

		// Compare if one value is equal to or greater than another
		case IRInstr::cmp_eqgt:
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);
			string tmpName = parameters.at(2);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable1->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(variable2->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable1->memoryOffset << "(%rbp), %eax" << endl;
		    o << "\t" << movInstr2 << "\t " << variable2->memoryOffset << "(%rbp), %edx" << endl;
		    o << "\tcmpl\t %edx, %eax" << endl;
		    o << "\tsetge\t %al" << endl;
		    o << "\tmovzbl\t %al, %eax" << endl;
		    o << "\tmovl\t %eax, " << tmp->memoryOffset << "(%rbp)" << endl;
            
			break;
		}

		// Bitwise AND operation
		case IRInstr::op_and:
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);
			string tmpName = parameters.at(2);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable1->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(variable2->variableType);

			// Write ASM instructions
			o << "\tmovl\t " << variable1->memoryOffset << "(%rbp), %eax"
		 	  << "\t\t# [op_and] load " << variableName1 << " into " << "%eax" << endl;
			o << "\tandl\t " << variable2->memoryOffset << "(%rbp), %eax"
	     	  << "\t\t# [op_and] and(" << variableName2 << ", " << "%eax)" << endl;
			o << "\tmovl\t %eax, " << tmp->memoryOffset << "(%rbp)"
		 	  << "\t\t# [op_and] load " << "%eax" << " into " << tmpName << endl;
	
			break;
		}

		// Bitwise OR operation
		case IRInstr::op_or:
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);
			string tmpName = parameters.at(2);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable1->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(variable2->variableType);

			// Write ASM instructions
			o << "\tmovl\t " << variable1->memoryOffset << "(%rbp), %eax"
			  << "\t\t# [op_or] load " << variableName1 << " into " << "%eax" << endl;
			o << "\torl\t " << variable2->memoryOffset << "(%rbp), %eax"
			  << "\t\t# [op_or] or(" << variableName2 << ", " << "%eax)" << endl;
			o << "\tmovl\t %eax, " << tmp->memoryOffset << "(%rbp)"
			  << "\t\t# [op_or] load " << "%eax" << " into " << tmpName << endl;
    
			break;
		}

		// Bitwise XOR operation
		case IRInstr::op_xor:
		{
			// Get parameters
			string variableName1 = parameters.at(0);
			string variableName2 = parameters.at(1);
			string tmpName = parameters.at(2);

			varStruct * variable1 = symbolTable->getVariable(variableName1);
			varStruct * variable2 = symbolTable->getVariable(variableName2);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable1->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(variable2->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable1->memoryOffset << "(%rbp), %eax"
		 	  << "\t\t# [op_xor] load " << variableName1 << " into " << "%eax" << endl;
			o << "\t" << movInstr2 << "\t " << variable2->memoryOffset << "(%rbp), %edx"
			  << "\t\t# [op_xor] load " << variableName2  << " into " << "%edx" << endl;
			o << "\txorl\t %edx, %eax" << endl;
			o << "\tmovl\t %eax, " << tmp->memoryOffset << "(%rbp)"
		 	  << "\t\t# [op_xor] load " << "%eax" << " into " << tmpName << endl;
    
			break;
		}

		// Plus equal operation (e.g., a += b)
		case IRInstr::op_plus_equal:
		{
			// Get parameters
			string variableName = parameters.at(0);
			string tmpName = parameters.at(1);

			varStruct * variable = symbolTable->getVariable(variableName);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(tmp->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable->memoryOffset << "(%rbp), %eax"
			<< "\t\t# [op_plus_equal] load " << variableName << " into " << "%eax" << endl;
			o << "\t" << movInstr2 << "\t " << tmp->memoryOffset << "(%rbp), %edx"
			<< "\t\t# [op_plus_equal] load " << tmpName << " into " << "%edx" << endl;
			o << "\taddl\t %edx, %eax" << endl;
			o << "\t" << movInstr1 << "\t %eax, " << variable->memoryOffset << "(%rbp)"
			<< "\t\t# [op_plus_equal] load %eax into " << variableName << endl;

			break;
		}

		// Minus equal operation (e.g., a -= b)
		case IRInstr::op_sub_equal:
		{
			// Get parameters
			string variableName = parameters.at(0);
			string tmpName = parameters.at(1);

			varStruct * variable = symbolTable->getVariable(variableName);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(tmp->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable->memoryOffset << "(%rbp), %eax"
			<< "\t\t# [op_sub_equal] load " << variableName << " into " << "%eax" << endl;
			o << "\t" << movInstr2 << "\t " << tmp->memoryOffset << "(%rbp), %edx"
			<< "\t\t# [op_sub_equal] load " << tmpName << " into " << "%edx" << endl;
			o << "\tsubl\t %edx, %eax" << endl;
			o << "\t" << movInstr1 << "\t %eax, " << variable->memoryOffset << "(%rbp)"
			<< "\t\t # [op_sub_equal] load %eax into " << variableName << endl;
			break;
		}

		// Multiply equal operation (e.g., a *= b)
		case IRInstr::op_mult_equal:
		{
			// Get parameters
			string variableName = parameters.at(0);
			string tmpName = parameters.at(1);

			varStruct * variable = symbolTable->getVariable(variableName);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(tmp->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable->memoryOffset << "(%rbp), %eax"
			<< "\t\t# [op_mult_equal] load " << variableName << " into " << "%eax" << endl;
			o << "\t" << movInstr2 << "\t " << tmp->memoryOffset << "(%rbp), %edx"
			<< "\t\t# [op_mult_equal] load " << tmpName << " into " << "%edx" << endl;
			o << "\timull\t %edx, %eax" << endl;
			o << "\t" << movInstr1 << "\t %eax, " << variable->memoryOffset << "(%rbp)"
			<< "\t\t# [op_mult_equal] load %eax into " << variableName << endl;

			break;
		}

		// Divide equal operation (e.g., a /= b)
		case IRInstr::op_div_equal:
		{
			// Get parameters
			string variableName = parameters.at(0);
			string tmpName = parameters.at(1);

			varStruct * variable = symbolTable->getVariable(variableName);
			varStruct * tmp = symbolTable->getVariable(tmpName);

			string movInstr1 = SymbolTable::typeOpeMoves.at(variable->variableType);
			string movInstr2 = SymbolTable::typeOpeMoves.at(tmp->variableType);

			// Write ASM instructions
			o << "\t" << movInstr1 << "\t " << variable->memoryOffset << "(%rbp), %eax"
			<< "\t\t # [op_div_equal] load " << variableName << " into " << "%eax" << endl;
			o << "\t" << movInstr2 << "\t " << tmp->memoryOffset << "(%rbp), %edx"
			<< "\t\t # [op_div_equal] load " << tmpName << " into " << "%edx" << endl;
			o << "\tcltd" << endl;
			o << "\tidivl\t "	<< tmp->memoryOffset << "(%rbp)" << endl;
			o << "\t" << movInstr1 << "\t %eax, " << variable->memoryOffset << "(%rbp)"
			<< "\t\t # [op_div_equal] load %eax into " << variableName << endl;
			
			break;
		}	

		// Conditional jump instruction
		case IRInstr::conditional_jump:
		{
			// Get params
			string testVariableName = parameters.at(0);
			string falseExitBlockLabel = parameters.at(1);
			string trueExitBlockLabel = parameters.at(2);

			// Write ASM instructions
			o << "\tcmpl\t $0, " << symbolTable->getVariable(testVariableName)->memoryOffset << "(%rbp)" << endl;
			o << "\tje\t " << falseExitBlockLabel << endl;
			o << "\tjmp\t " << trueExitBlockLabel << endl;

			break;
		}

		// Unconditional jump instruction
		case IRInstr::absolute_jump:
		{
			// Get params
			string blockLabel = parameters.at(0);

			// Write ASM instructions
			o << "\tjmp\t " << blockLabel << endl;

			break;
		}

		// Call a function
		case IRInstr::call:
		{
			// Get params
			string label = parameters.at(0);
			string tmp = parameters.at(1);
			int nbParams = stoi(parameters.at(2));
			int sub = max((nbParams-6)*8, 0);

			// Write ASM instructions
			o << "\tcall\t " << label << endl;

			if (sub > 0) 
			{
				o << "\tsubq\t $" << sub << ", %rsp" << endl;
			}

			o << "\tmovl\t %eax, " << symbolTable->getVariable(tmp)->memoryOffset << "(%rbp)"
			  << "\t\t# [call] load " << "%eax" << " into " << tmp << endl;

			break;
		}

		// Write parameter value into stack
		case IRInstr::wparam:
		{
			// Get params
			string variableName = parameters.at(0);
			int nbParams = stoi(parameters.at(1));
			varStruct * variable = symbolTable->getVariable(variableName);

			if (nbParams < 6)  // Use registers for less than 6 parameters
			{
				// Get param register
				string reg = IRInstr::AMD86_paramRegisters[variable->variableType][nbParams];
				string movInstr = (variable->variableType == "char") ? "movb" : "movl";

				// Write ASM instructions
				o << "\t" << movInstr << "\t " << variable->memoryOffset << "(%rbp), " << reg \
				  << "\t\t# [wparam] load " << variableName << " into " << reg << endl;
			}
			else  // Pass parameters on the stack if more than 6 parameters
			{
				// Write ASM instructions
				if (variable->variableType == "char") 
				{ 
					o << "\tmovzbl\t " << variable->memoryOffset << "(%rbp)" << ", %eax" << endl;
					o << "\tpushq\t %rax" \
					  << "\t\t# [wparam] push " << variableName << " onto the stack" << endl;
				}
				else 
				{
					o << "\tpushq\t " << variable->memoryOffset << "(%rbp)" \
					  << "\t\t# [wparam] push " << variableName << " onto the stack" << endl;
				}
			}

			break;	
		}

		// Read parameter value from stack
		case IRInstr::rparam:
		{
			// Get params
			string variableName = parameters.at(0);
			int nbParams = stoi(parameters.at(1));
			int offset = stoi(parameters.at(2));
			varStruct * variable = symbolTable->getVariable(variableName);

			if (nbParams < 6)  // Use registers for less than 6 parameters
			{ 
				// Get param register
				string reg = IRInstr::AMD86_paramRegisters[variable->variableType][nbParams];
				string movInstr = (variable->variableType == "char") ? "movb" : "movl";

				// Write ASM instructions
				o << "\t" << movInstr << "\t " << reg << ", " << variable->memoryOffset << "(%rbp)"
				  << "\t\t# [rparam] load " << reg << " into " << "°" + variableName << endl;
			}
			else // Load parameters from stack if more than 6 parameters 
			{
				string movInstr1, movInstr2;
				string reg;

				if (variable->variableType == "int")
				{
					// int = int : movl
					movInstr1 = "movl";
					movInstr2 = "movl";
					reg = "eax";
				}
				else if (variable->variableType == "char")
				{
					// char = char : movb
					movInstr1 = "movb";
					movInstr2 = "movb";
					reg = "al";
				}

				// Write ASM instructions
				o << "\t" << movInstr1 << "\t " << offset << "(%rbp), %" << reg
				  << "\t\t# [rparam] load param " << nbParams << " into " << "%" << reg << endl;
				o << "\t" << movInstr2 << "\t %" << reg << ", " << variable->memoryOffset << "(%rbp)"
				  << "\t\t# [rparam] load " << "%" << reg << " into " << "°" + variableName << endl;
			}

			break;
		}

		// Prologue
		case IRInstr::prologue:
		{
			// Get parameters
			string label = parameters.at(0);
			
			// Write ASM instructions
			o << ".globl\t " << label << endl;
			o << ".type\t " << label << ", @function" << endl;
			o << label << ":" << endl;

			o << "\t# prologue\n";
    		o << "\tpushq\t %rbp \t\t\t# save %rbp on the stack\n";
    		o << "\tmovq\t %rsp, %rbp \t\t# define %rbp for the current function\n";

			// Get the memory size needed to store the function's local variables (must be multiple of 16)
			int memSize = symbolTable->getMemorySpace();
			int remainder = memSize % 16;
			memSize += (remainder > 0) ? 16 - remainder : 0;

			o << "\tsubq\t $" << memSize << ", %rsp" << endl << endl;

			break;
		}

		// Return from function
		case IRInstr::ret:
		{
			// Get parameters
			string param = parameters.at(0);

			if (symbolTable->hasVariable(param)) // If we're returning a var
			{
				varStruct * variable = symbolTable->getVariable(param);

				// Write ASM instructions
				o << "\tmovl\t " << variable->memoryOffset << "(%rbp), %eax"
				<< "\t\t# [ret] load " << param << " into "
				<< "%eax" << endl;
			}
			else // If we're returning a const
			{
				int constValue = stoi(param.substr(1, param.size() - 1));

				// Write ASM instructions
				o << "\tmovl\t $" << constValue << ", %eax"
				  << "\t\t# [ret] load " << constValue << " into "
				  << "%eax" << endl;
			}

			o << "\n\t# epilogue" << endl ;
			o << "\tmovq\t %rbp, %rsp" << endl;
			o << "\tpopq\t %rbp \t\t\t# restore %rbp from the stack" << endl;
			o << "\tret \t\t\t\t# return to the caller" << endl << endl;

			break;
		}
	}
}