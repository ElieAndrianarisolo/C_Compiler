/*************************************************************************
                          PLD Compilateur: CodeGenVisitor
                          ---------------------------
    start   : 26/03/2024
    authors : ANDRIANARISOLO Elie, FARHAT Widad,
			  SARR Seynabou, TONG An Jun
*************************************************************************/

//---- Implementation of class <CodeGenVisitor> (file CodeGenVisitor.cpp) -----/

#include "CodeGenVisitor.h"

using namespace std;

// Constructor
CodeGenVisitor::CodeGenVisitor(ErrorHandler& eH, CFG& cfg) : errorHandler(eH), cfg(cfg) 
{
    // Initialize the global symbol table
    globalSymbolTable = new SymbolTable(0, nullptr);
    
    // Add standard library function symbols to the global symbol table
    this->addSymbolPutchar();
    this->addSymbolGetchar();
}

// Destructor
CodeGenVisitor::~CodeGenVisitor() 
{
    // Delete the global symbol table
    delete globalSymbolTable;

    // Delete symbol tables in the garbage stack
    while (!symbolTableGarbage.empty()) 
    {
        SymbolTable* _st = symbolTableGarbage.top();

        if (_st != nullptr) 
        {
            delete _st;
            symbolTableGarbage.pop();
        }
    }
}

// Visit the program node
antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) 
{
	int n = ctx->funcDeclare().size();

	// Visit all function declaration headers
	for (int i = 0; i < n; i++) 
    {
		visitFuncDeclareHeader(ctx->funcDeclare(i));
	}

	// Visit all function declarations
	for (int i = 0; i < n; i++) 
    {
		visitFuncDeclareBody(ctx->funcDeclare(i));
	}

	// Visit the main function
	visit(ctx->mainDeclare());

	return 0;
}

// Visit the main function declaration
antlrcpp::Any CodeGenVisitor::visitMainDeclare(ifccParser::MainDeclareContext *ctx) 
{
	// Visit the main function declaration header
	visit(ctx->mainDeclareHeader());
	
	// Set the current function name to 'main'
	currentFunction = "main";

	// Visit begin block (create the symbol table) 
	visit(ctx->beginBlock());
	SymbolTable * newSymbolTable = symbolTablesStack.top();

	// Create the prologue instructions
	cfg.getCurrentBB()->addInstruction(IRInstr::prologue, {"main"}, newSymbolTable);

	// Create the body instructions
	visit(ctx->body());

	// Create the default return instruction if no explicit return statement is found
    if (!newSymbolTable->hasReturned()) 
    {
        returnDefault(ctx);
    }

	// Visit end block (discard the symbol table)
	visit(ctx->endBlock());
	
	return 0;
}

// Visit the main function header with no return type specified
antlrcpp::Any CodeGenVisitor::visitMainDeclareHeaderNoReturn(ifccParser::MainDeclareHeaderNoReturnContext *ctx)
{
    // Create the main function in the global symbol table
    globalSymbolTable->addFunction("main", "int", 0, {}, {}, ctx->getStart()->getLine());

    // Generate a warning message for missing return type
    string message =  "No return type specified for the main function: defaults to 'int'";
    errorHandler.signal(WARNING, message, ctx->getStart()->getLine());

    return 0;
}

// Visit the main function header with a return type specified
antlrcpp::Any CodeGenVisitor::visitMainDeclareHeaderWithReturn(ifccParser::MainDeclareHeaderWithReturnContext *ctx) 
{
    // Create the main function in the global symbol table with the specified return type
    globalSymbolTable->addFunction("main", ctx->FTYPE->getText(), 0, {}, {}, ctx->getStart()->getLine());
    return 0;
}

// Visit a function declaration
antlrcpp::Any CodeGenVisitor::visitFuncDeclare(ifccParser::FuncDeclareContext *ctx) 
{
    return 0;
}

// Visit the header of a function declaration
antlrcpp::Any CodeGenVisitor::visitFuncDeclareHeader(ifccParser::FuncDeclareContext *ctx) 
{
	// Fetch the function name
	string functionName = ctx->VAR(0)->getText();

	// Fetch the return type
	string returnType = ctx->FTYPE->getText();

	// Fetch the parameter names and types
	vector<string> parametersTypes = {};
	vector<string> parametersNames = {};
	int nbParameters = ctx->VAR().size()-1;

	string paramName, paramType;

	// Iterate through the parameters
	for(int i = 0; i < nbParameters; i++) 
    {
		paramName = ctx->VAR(1+i)->getText();
		paramType = ctx->variableType(i)->getText();
		parametersTypes.push_back(paramType);
		parametersNames.push_back(paramName);
	}

	// Adjust the number of parameters if the function is declared as 'void'
	if (ctx->TVOID().size() == 2 && returnType == "void" || ctx->TVOID().size() == 1 && returnType != "void")
    {
		nbParameters = -1;
	}

	// Check for errors
	if (globalSymbolTable->hasFunction(functionName)) 
    {
		string message =  "Function '" + functionName + "' has already been declared";
		errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
		return 1; 
	}

	// Create the function in symbol table
	globalSymbolTable->addFunction(functionName, returnType, nbParameters, parametersTypes, parametersNames, ctx->getStart()->getLine());

	return 0;
}

// Visit the body of a function declaration
antlrcpp::Any CodeGenVisitor::visitFuncDeclareBody(ifccParser::FuncDeclareContext *ctx)
{
    // Visit the begin block (create the symbol table) 
	visit(ctx->beginBlock());
	SymbolTable * newSymbolTable = symbolTablesStack.top();

	// Fetch the function name
	string functionName = ctx->VAR(0)->getText();
	currentFunction = functionName;

	// Fetch the function from the symbol table (added during header visit)
	funcStruct * function = globalSymbolTable->getFunction(functionName);

	// Create parameters variables in the symbol table
	for(int i = 0 ; i < function->nbParameters ; i++) 
    {
		newSymbolTable->addVariable("°" + function->parameterNames[i], function->parameterTypes[i], ctx->getStart()->getLine());
	}

	// Create the prologue instructions
	cfg.getCurrentBB()->addInstruction(IRInstr::prologue, {functionName}, newSymbolTable); 
	
	int paramStackOffset = 16; // The size of the return adress stored on the stack when calling the function
	
	// Create instructions that loads register into variable
    for(int i = function->nbParameters-1 ; i >= 0 ; i--) 
    {
		cfg.getCurrentBB()->addInstruction(IRInstr::rparam, {function->parameterNames[i], to_string(i), to_string(paramStackOffset)}, newSymbolTable);
		paramStackOffset += 8;
	}

	// Create the body instructions
	visit(ctx->body());

	// Create default return instruction if no explicit return statement is found
	if (!newSymbolTable->hasReturned()) 
    {
        returnDefault(ctx);
    }

	// Visit the end block (discard symbol table)
	visit(ctx->endBlock());

	return 0;
}

// Visit the beginning of a block
antlrcpp::Any CodeGenVisitor::visitBeginBlock(ifccParser::BeginBlockContext *ctx) 
{
	// Fetch the parent symbol table
	SymbolTable * parentSymbolTable = globalSymbolTable;
	int startingStackPointer = 0;
	
    if (symbolTablesStack.size() > 0) 
    {
		parentSymbolTable = symbolTablesStack.top();
		startingStackPointer = parentSymbolTable->getStackPointer();
	}

	// Create a new symbol table
	SymbolTable * newSymbolTable = new SymbolTable(startingStackPointer, parentSymbolTable);
	symbolTablesStack.push(newSymbolTable);

	return 0;
}

// Visit the end of a block
antlrcpp::Any CodeGenVisitor::visitEndBlock(ifccParser::EndBlockContext *ctx) 
{
	// Perform static analysis on variables used within the block
	symbolTablesStack.top()->checkUsedVariables(errorHandler);

	// Get the symbol table and current basic block
	SymbolTable * symbolTable = symbolTablesStack.top();
	BasicBlock * currentBB = cfg.getCurrentBB();

	// Add conditional jump instruction if there is a false exit
	if (currentBB->getExitFalse()) 
    {
		currentBB->addInstruction(IRInstr::conditional_jump, {currentBB->getTestVariableName(), currentBB->getExitFalse()->getLabel(), currentBB->getExitTrue()->getLabel()}, symbolTable);
	}

	// Add absolute jump instruction if there is a true exit
	if (currentBB->getExitTrue())
    {
		currentBB->addInstruction(IRInstr::absolute_jump, {currentBB->getExitTrue()->getLabel()}, symbolTable);
	}

	// Remove the symbol table from the stack and push it to garbage for deletion
	symbolTableGarbage.push(symbolTable);
	symbolTablesStack.pop();

	return 0;
}

// Visit a variable declaration
antlrcpp::Any CodeGenVisitor::visitVarDeclare(ifccParser::VarDeclareContext *ctx)
{
	// Get the symbol table of the current block
	SymbolTable * symbolTable = symbolTablesStack.top();

	// Number of variables to declare
	int nbVariable = ctx->VAR().size();

	// Fetch the type
	string varType = ctx->variableType()->getText();
	string variableName;

	// Iterate through each variable to declare
	for(int i = 0; i < nbVariable; i++) 
	{
		// Fetch the variable
		variableName = ctx->VAR(i)->getText();

		// Check for errors
		if (symbolTable->hasVariable(variableName) == 1) 
		{
			string message = "Variable '" + variableName + "' has already been declared";
			errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
			return 1;
		}

		if (symbolTable->hasParameter(variableName) == 1) 
		{
			string message = "Variable '" + variableName + "' is already defined as a parameter of the function";
			errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
			return 1;
		}

		// Add the variable to the symbol table
		symbolTable->addVariable(variableName, varType, ctx->getStart()->getLine());
	}

	return 0;
}

// Visit a variable declaration and assignment
antlrcpp::Any CodeGenVisitor::visitVarDeclareAndAffect(ifccParser::VarDeclareAndAffectContext *ctx)
{
	// Get the symbol table of the current block
	SymbolTable * symbolTable = symbolTablesStack.top();

	// Fetch the variable
	string variableName = ctx->VAR()->getText();
	string varType = ctx->variableType()->getText();

	// Check for errors
	if (symbolTable->hasVariable(variableName) == 1) 
	{
		string message = "Variable '" + variableName + "' has already been declared";
		errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
        return 1;
	}

	if (symbolTable->hasParameter(variableName) == 1) 
	{
		string message = "Variable '" + variableName + "' is already defined as a parameter of the function";
		errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
		return 1;
	}

	// Add the variable to the symbol table
	symbolTable->addVariable(variableName, varType, ctx->getStart()->getLine());

	// Save the current stack pointer
	int currStackPointer = symbolTable->getStackPointer();

	// Compute the expression
	varStruct * result = visit(ctx->exprInstruction());

	// Check for void errors
	if (result->variableType == "void") 
	{
		string message =  "Cannot perform operations on void";
		errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
		return &SymbolTable::stupidVarStruct;
	}

	// Reset the stack pointer and temp variable counter after having evaluated the expression
	symbolTable->setStackPointer(currStackPointer);

	// Add ASM instructions to save expression in the variable 
	cfg.getCurrentBB()->addInstruction(IRInstr::aff, {result->variableName, variableName}, symbolTable);
	
	return 0;
}

// Visit an assignment expression
antlrcpp::Any CodeGenVisitor::visitAffectExpr(ifccParser::AffectExprContext *ctx)
{
	// Get the symbol table of the current block
	SymbolTable * symbolTable = symbolTablesStack.top();

	// Fetch the first variable
	string variableName = ctx->VAR()->getText();

	// Check for errors
	if (!symbolTable->hasVariable(variableName) && !symbolTable->hasParameter(variableName)) 
	{
		string message = "Variable '" + variableName + "' has not been declared";
		errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
		return &SymbolTable::stupidVarStruct;
	}
		
	// Save the current stack pointer
	int currStackPointer = symbolTable->getStackPointer();
	
	// Compute the expression
	varStruct * tmp = visit(ctx->exprInstruction());

	// Reset the stack pointer and temp variable counter after having evaluated the expression
	symbolTable->setStackPointer(currStackPointer);

	// Check for void errors
	if (tmp->variableType == "void") 
	{
		string message =  "Cannot perform operations on void";
		errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
		return &SymbolTable::stupidVarStruct;
	}

	// Add ASM instructions to save expression in the variable 
	cfg.getCurrentBB()->addInstruction(IRInstr::aff, {tmp->variableName, variableName}, symbolTable);

	return tmp;
}

// Visit a constant expression
antlrcpp::Any CodeGenVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx) 
{
	// Get the symbol table of the current block
	SymbolTable * symbolTable = symbolTablesStack.top();
	int constValue;

	// Size of INT
	long intSize = (long)INT_MAX - (long)INT_MIN + 1;

	// Fetch the constant
	string constStr = ctx->CONST()->getText();
	
	if (constStr.length() == 3 && constStr[0] == '\'' && constStr[2] == '\'')  // If it's a single character enclosed in single quotes
    {
		constValue = constStr[1];
	} 
	else if (constStr.length() > 3 && constStr[0] == '\'' && constStr[constStr.length()-1] == '\'')  // If it's a multi-character constant enclosed in single quotes
    {
		// Warn about usage of multi-character constant
		string message =  "Use of multi-character character constant";
		errorHandler.signal(WARNING, message, ctx->getStart()->getLine());
		
		// Compute the value of the multi-character constant
		constValue = 0;
		int constStrLength = constStr.length()-1;

		for (int i = 1; i < constStrLength; i++)
        {
			constValue = constValue*256 + constStr[i];
		}
	}
	else 
    {
		try // Deal with std::out_of_range and std::invalid_argument
        { 
			// Convert the constant's string representation to an unsigned long long
            unsigned long long ullConstValue = stoull(constStr);
			
			// Make sure it fits within the range of an int
			ullConstValue = ullConstValue % intSize;

			// If the value is greater than INT_MAX, handle it accordingly
			if(ullConstValue>INT_MAX) 
			{
				constValue = ullConstValue-intSize;
			}
			else 
			{
				constValue = ullConstValue;
			}
		} 
		catch(std::out_of_range& e) // Handle if the constant is too big for unsigned long long
        { 
			long lConstValue = 0;
			int currentDigit;

			// Iterate through each char in string (left to right)
			for (string::iterator it=constStr.begin(); it!=constStr.end(); ++it) 
            {	
				currentDigit = *it - '0';

				if (currentDigit >= 0 && currentDigit < 10) 
                {
					lConstValue = lConstValue*10 + currentDigit;

					// Handle overflow if the value exceeds INT_MAX
					if (lConstValue > INT_MAX) 
                    {
						lConstValue -= intSize;
					}
				}
			}

			// Convert the long value to int
			constValue = (int)lConstValue;

			// Warn about the integer constant being too large for its type
			string message = "Integer constant is too large for its type. Overflow in conversion to 'int' changes value from '" + constStr + "' to '" + to_string(constValue) + "'";
			errorHandler.signal(WARNING, message, ctx->getStart()->getLine());
		} 
		catch(std::invalid_argument& e) // Handle if the constant is not a valid integer
        {
			// Error about the invalid argument
			string message = "Integer constant threw invalid argument exception : " + constStr;
			errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
			return &SymbolTable::stupidVarStruct;
		}

	}

	varStruct * tmp;
 	
	// Add the constant instructions to the intermediate representation (IR)
	if (constStr[0] == '\'') // If it's a character constant
    {
		tmp = createTmpVariable(ctx, "char");
		cfg.getCurrentBB()->addInstruction(IRInstr::ldconst, {"char", to_string(constValue), tmp->variableName}, symbolTable);
	} 
    else // If it's an integer constant
    {
		tmp = createTmpVariable(ctx, "int");
		cfg.getCurrentBB()->addInstruction(IRInstr::ldconst, {"int", to_string(constValue), tmp->variableName}, symbolTable);
	}

	// Return the temporary variable
	return tmp;
}

// Visit a variable expression
antlrcpp::Any CodeGenVisitor::visitVarExpr(ifccParser::VarExprContext *ctx)
{
	// Get the symbol table of the current block
	SymbolTable* symbolTable = symbolTablesStack.top();

	// Fetch variable
	string variableName = ctx->VAR()->getText();

	// Throw an error if no corresponding variable or parameter has been found
	if (!symbolTable->hasVariable(variableName) && !symbolTable->hasParameter(variableName)) 
	{
		string message = "Variable '" + variableName + "' has not been declared";
		errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
		return &SymbolTable::stupidVarStruct;
	}

	// Mark the variable as used
	varStruct * variable = symbolTable->getVariable(variableName,true);
	variable->isUsed = true;

	// Return the variable
	return variable;
}

// Visit a function expression
antlrcpp::Any CodeGenVisitor::visitFuncExpr(ifccParser::FuncExprContext *ctx) 
{
	// Get the symbol table of the current block
	SymbolTable* symbolTable = symbolTablesStack.top();

	// Fetch function name
	string funcName = ctx->VAR()->getText();

	// Check if the function is declared 
	if (!globalSymbolTable->hasFunction(funcName)) 
	{
		string message =  "Function '" + funcName + "' has not been declared";
		errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
		return &SymbolTable::stupidVarStruct;
	}

	// Get function information
	funcStruct* func = globalSymbolTable->getFunction(funcName);

	// Check if it's an implicit declaration
	if (func->functionLine > ctx->getStart()->getLine()) 
	{
		string message =  "Function '" + funcName + "' might be declared implicitely";
		errorHandler.signal(WARNING, message, ctx->getStart()->getLine());
	}

	// Check parameter number
	int nbParams = ctx->expr().size();
	bool hasVoid = func->nbParameters < 0;

	if ((func->nbParameters > 0 && nbParams != func->nbParameters) || (hasVoid && nbParams > 0)) 
	{
		string message =  "Function '" + funcName + "' is called with the wrong number of parameters";
		errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
		return &SymbolTable::stupidVarStruct;
	}

	// Save the current stack pointer
	int currStackPointer = symbolTable->getStackPointer();

	// Iterate through parameters to evaluate and save them
	vector<varStruct*> params;

	for(int i = 0; i < nbParams; i++) 
	{
		varStruct * result = visit(ctx->expr(i));
		params.push_back(result);
	}

	// Reset the stack pointer after having evaluated the expression
	symbolTable->setStackPointer(currStackPointer);

	// Write ASM instructions to put the evaluated params into a param register
	for (int i = nbParams-1; i >= 0; i--) 
	{
		cfg.getCurrentBB()->addInstruction(IRInstr::wparam, {params[i]->variableName, to_string(i)}, symbolTable);
	}

	// Create a temporary variable to store the function result
	varStruct* tmp = createTmpVariable(ctx, func->returnType);

	// Write call instruction
	cfg.getCurrentBB()->addInstruction(IRInstr::call, {funcName, tmp->variableName, to_string(nbParams)}, symbolTable);
	func->isCalled = true;

	// Return the temporary variable holding the function result
	return tmp;
}

// Visit a parentheses expression
antlrcpp::Any CodeGenVisitor::visitParenthesesExpr(ifccParser::ParenthesesExprContext *ctx)
{
	// Visit the expression within parentheses and return its result
	return visit(ctx->exprInstruction()); 
}

// Visit an unary expression
antlrcpp::Any CodeGenVisitor::visitUnaryExpr(ifccParser::UnaryExprContext *ctx)
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable * symbolTable = symbolTablesStack.top();

	// Fetch sub-expressions
	varStruct * variable = visit(ctx->expr());

	// Create a temporary variable to store the result of the unary operation
	varStruct * tmp = createTmpVariable(ctx);
	
	// Check for void errors
	if (variable->variableType == "void") 
	{
		// Signal an error if attempting to perform operations on void
		string message =  "Cannot perform operations on void";
		errorHandler.signal(ERROR, message, ctx->getStart()->getLine());

		// Return a dummy variable indicating an error
		return &SymbolTable::stupidVarStruct;
	}

	// Get the operator
	char op = ctx->UNARY->getText()[0];

	// Apply the unary operator
	switch(op) 
	{
		case '!':
		{
			// Add instruction for logical negation (!)
			cfg.getCurrentBB()->addInstruction(IRInstr::op_not, {variable->variableName, tmp->variableName}, symbolTable);
			break;
		}

		case '-':
		{
			// Add instruction for unary minus (-)
			cfg.getCurrentBB()->addInstruction(IRInstr::op_minus, {variable->variableName, tmp->variableName}, symbolTable);
			break;
		}
	}
	
	// Return the temporary variable holding the result of the unary operation
	return tmp;
}

// Visit an addition or subtraction expression
antlrcpp::Any CodeGenVisitor::visitAddSubExpr(ifccParser::AddSubExprContext *ctx) 
{
    // Get the symbol table from the top of the symbol tables stack
    SymbolTable * symbolTable = symbolTablesStack.top();

    // Fetch the sub-expressions
    varStruct * variable1 = visit(ctx->expr(0));
    varStruct * variable2 = visit(ctx->expr(1));

    // Create a temporary variable to store the result of the addition/subtraction
    varStruct * tmp = createTmpVariable(ctx);

    // Check for errors
    if (variable1->variableType == "void" || variable2->variableType == "void") 
    {
        // Signal an error if attempting to perform operations on void
        string message =  "Cannot perform operations on void";
        errorHandler.signal(ERROR, message, ctx->getStart()->getLine());

        // Return a dummy variable indicating an error
        return &SymbolTable::stupidVarStruct;
    }

	// Check if either sub-expression is marked as incorrect due to previous errors
    if (!variable1->isCorrect || !variable2->isCorrect) 
    {
        return &SymbolTable::stupidVarStruct;
    }

    // Get the operator
    char op = ctx->OP2->getText()[0];

    // Apply the operator
    switch (op) 
    {
        case '+':
        {
            // Add instruction for addition
            cfg.getCurrentBB()->addInstruction(IRInstr::op_add, {variable1->variableName, variable2->variableName, tmp->variableName}, symbolTable);
            break;
        }

        case '-':
        {
            // Add instruction for subtraction
            cfg.getCurrentBB()->addInstruction(IRInstr::op_sub, {variable1->variableName, variable2->variableName, tmp->variableName}, symbolTable);
            break;
        }
    }

    // Return the temporary variable holding the result of the addition/subtraction
    return tmp;
}

// Visit a multiplication, division, or modulo expression
antlrcpp::Any CodeGenVisitor::visitMulDivModExpr(ifccParser::MulDivModExprContext *ctx)
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable * symbolTable = symbolTablesStack.top();

	// Fetch sub-expressions
	varStruct * variable1 = visit(ctx->expr(0));
	varStruct * variable2 = visit(ctx->expr(1));

	// Create a temporary variable to store the result of the addition/subtraction
	varStruct * tmp = createTmpVariable(ctx);

	// Check for errors
    if (variable1->variableType == "void" || variable2->variableType == "void") 
    {
        // Signal an error if attempting to perform operations on void
        string message =  "Cannot perform operations on void";
        errorHandler.signal(ERROR, message, ctx->getStart()->getLine());

        // Return a dummy variable indicating an error
        return &SymbolTable::stupidVarStruct;
    }

	// Check if either sub-expression is marked as incorrect due to previous errors
    if (!variable1->isCorrect || !variable2->isCorrect) 
    {
        return &SymbolTable::stupidVarStruct;
    }

	// Get the operator
	char op = ctx->OP1->getText()[0];
	
	// Apply the operators
	switch (op) 
	{
		case '*':
		{
			// Add instruction for multiplication
			cfg.getCurrentBB()->addInstruction(IRInstr::op_mul, {variable1->variableName, variable2->variableName, tmp->variableName}, symbolTable);
			break;
		}

		case '/':
		{
			// Add instruction for division
			cfg.getCurrentBB()->addInstruction(IRInstr::op_div, {variable1->variableName, variable2->variableName, tmp->variableName}, symbolTable);
			break;
		}

		case '%':
		{
			// Add instruction for modulo
			cfg.getCurrentBB()->addInstruction(IRInstr::op_mod, {variable1->variableName, variable2->variableName, tmp->variableName}, symbolTable);
			break;
		}
	}

	// Return the temporary variable holding the result of the operation
	return tmp;
}

// Visit an equality or inequality comparison expression
antlrcpp::Any CodeGenVisitor::visitCmpEqualityExpr(ifccParser::CmpEqualityExprContext *ctx)
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable * symbolTable = symbolTablesStack.top();

	// Fetch sub-expressions
	varStruct * variable1 = visit(ctx->expr(0));
	varStruct * variable2 = visit(ctx->expr(1));

	// Create a temporary variable to store the result of the comparison
	varStruct * tmp = createTmpVariable(ctx);

	// Check for errors
    if (variable1->variableType == "void" || variable2->variableType == "void") 
    {
        // Signal an error if attempting to perform operations on void
        string message =  "Cannot perform operations on void";
        errorHandler.signal(ERROR, message, ctx->getStart()->getLine());

        // Return a dummy variable indicating an error
        return &SymbolTable::stupidVarStruct;
    }

	// Check if either sub-expression is marked as incorrect due to previous errors
	if(!variable1->isCorrect || !variable2->isCorrect) 
	{
        return &SymbolTable::stupidVarStruct;
    }

	// Get the comparison operator
	char op = ctx->EQ->getText()[0];

	// Apply the operators
	switch (op) 
	{
		case '=':
		{
			// Add instruction for equality comparison (==)
			cfg.getCurrentBB()->addInstruction(IRInstr::cmp_eq, {variable1->variableName, variable2->variableName, tmp->variableName}, symbolTable);
			break;
		}

		case '!':
		{
			// Add instruction for inequality comparison (!=)
			cfg.getCurrentBB()->addInstruction(IRInstr::cmp_neq, {variable1->variableName, variable2->variableName, tmp->variableName}, symbolTable);
			break;
		}
	}
		
	// Return the temporary variable holding the result of the comparison
	return tmp;
}

// Visit a comparison expression for less than or greater than
antlrcpp::Any CodeGenVisitor::visitCmpLessOrGreaterExpr(ifccParser::CmpLessOrGreaterExprContext *ctx)
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable* symbolTable = symbolTablesStack.top();

	// Fetch sub-expressions
	varStruct * variable1 = visit(ctx->expr(0));
	varStruct * variable2 = visit(ctx->expr(1));

	// Create a temporary variable to store the result of the comparison
	varStruct * tmp = createTmpVariable(ctx);

	// Check for errors
    if (variable1->variableType == "void" || variable2->variableType == "void") 
    {
        // Signal an error if attempting to perform operations on void
        string message =  "Cannot perform operations on void";
        errorHandler.signal(ERROR, message, ctx->getStart()->getLine());

        // Return a dummy variable indicating an error
        return &SymbolTable::stupidVarStruct;
    }

	// Check if either sub-expression is marked as incorrect due to previous errors
    if (!variable1->isCorrect || !variable2->isCorrect)
	{
        return &SymbolTable::stupidVarStruct;
    }
	
	// Get the operator ('<' for less than, '>' for greater than)
	char op = ctx->CMP->getText()[0];

	// Apply the operators
	switch (op) 
	{
		case '<':
		{
			// Add instruction for less than comparison
			cfg.getCurrentBB()->addInstruction(IRInstr::cmp_lt, {variable1->variableName, variable2->variableName, tmp->variableName}, symbolTable);
			break;
		}

		case '>':
		{
			// Add instruction for greater than comparison
			cfg.getCurrentBB()->addInstruction(IRInstr::cmp_gt, {variable1->variableName, variable2->variableName, tmp->variableName}, symbolTable);
			break;
		}
	}
	
	// Return the temporary variable holding the result of the comparison
	return tmp;
}

// Visit a comparison expression for less or equal than, or greater or equal than
antlrcpp::Any CodeGenVisitor::visitCmpEqualityLessGreaterExpr(ifccParser::CmpEqualityLessGreaterExprContext *ctx)
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable* symbolTable = symbolTablesStack.top();

	// Fetch sub-expressions
	varStruct* variable1 = visit(ctx->expr(0));
	varStruct* variable2 = visit(ctx->expr(1));

	// Create a temporary variable to store the result of the comparison
	varStruct* tmp = createTmpVariable(ctx);

	// Check for errors
    if (variable1->variableType == "void" || variable2->variableType == "void") 
    {
        // Signal an error if attempting to perform operations on void
        string message =  "Cannot perform operations on void";
        errorHandler.signal(ERROR, message, ctx->getStart()->getLine());

        // Return a dummy variable indicating an error
        return &SymbolTable::stupidVarStruct;
    }

    // Check if either sub-expression is marked as incorrect due to previous errors
    if (!variable1->isCorrect || !variable2->isCorrect) 
	{
        return &SymbolTable::stupidVarStruct;
    }

	// Get the operator
	char op = ctx->EQLG->getText()[0];

	// Apply the operators
	switch (op) 
	{
		case '<':
		{
			// Add IR instruction for comparison: variable1 <= variable2
			cfg.getCurrentBB()->addInstruction(IRInstr::cmp_eqlt, {variable1->variableName, variable2->variableName, tmp->variableName}, symbolTable);
			break;
		}

		case '>':
		{
			// Add IR instruction for comparison: variable1 >= variable2
			cfg.getCurrentBB()->addInstruction(IRInstr::cmp_eqgt, {variable1->variableName, variable2->variableName, tmp->variableName}, symbolTable);
			break;
		}
	}
	
	// Return the temporary variable holding the result of the comparison
	return tmp;
}

// Visit an AND expression
antlrcpp::Any CodeGenVisitor::visitAndExpr(ifccParser::AndExprContext *ctx)
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable* symbolTable = symbolTablesStack.top();

	// Fetch sub-expressions
	varStruct* variable1 = visit(ctx->expr(0));
	varStruct* variable2 = visit(ctx->expr(1));

	// Create a temporary variable to store the result of the logical AND operation
	varStruct* tmp = createTmpVariable(ctx);

	// Check for errors related to void type
    if (variable1->variableType == "void" || variable2->variableType == "void") 
    {
        // Signal an error if attempting to perform operations on void
        string message =  "Cannot perform operations on void";
        errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
        
        // Return a dummy variable indicating an error
        return &SymbolTable::stupidVarStruct;
    }

    // Check if either sub-expression is marked as incorrect due to previous errors
    if (!variable1->isCorrect || !variable2->isCorrect) 
    {
        return &SymbolTable::stupidVarStruct;
    }

	// Apply the operator and generate the corresponding IR instruction
	cfg.getCurrentBB()->addInstruction(IRInstr::op_and, {variable1->variableName, variable2->variableName, tmp->variableName}, symbolTable);
	
	 // Return the temporary variable holding the result of the logical AND operation
    return tmp;
}

// Visit an OR expression
antlrcpp::Any CodeGenVisitor::visitOrExpr(ifccParser::OrExprContext *ctx)
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable* symbolTable = symbolTablesStack.top();

	// Fetch sub-expressions
	varStruct* variable1 = visit(ctx->expr(0));
	varStruct* variable2 = visit(ctx->expr(1));

	// Create a temporary variable to store the result of the logical OR operation
	varStruct* tmp = createTmpVariable(ctx);

	// Check for errors
	if (variable1->variableType == "void" || variable2->variableType == "void") 
	{
        // Signal an error if attempting to perform operations on void
        string message =  "Cannot perform operations on void";
        errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
        
        // Return a dummy variable indicating an error
        return &SymbolTable::stupidVarStruct;
    }

	// Check if either sub-expression is marked as incorrect due to previous errors
    if(!variable1->isCorrect || !variable2->isCorrect) 
	{
        return &SymbolTable::stupidVarStruct;
    }
	
	// Apply the operator and generate the corresponding IR instruction
    cfg.getCurrentBB()->addInstruction(IRInstr::op_or, {variable1->variableName, variable2->variableName, tmp->variableName}, symbolTable);
    
    // Return the temporary variable holding the result of the logical OR operation
    return tmp;
}

// Visit an XOR expression
antlrcpp::Any CodeGenVisitor::visitXorExpr(ifccParser::XorExprContext *ctx)
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable* symbolTable = symbolTablesStack.top();

	// Fetch sub-expressions
	varStruct* variable1 = visit(ctx->expr(0));
	varStruct* variable2 = visit(ctx->expr(1));

	// Create a temporary variable to store the result of the logical OR operation
	varStruct* tmp = createTmpVariable(ctx);

	// Check for errors
	if (variable1->variableType == "void" || variable2->variableType == "void") 
    {
        // Signal an error if attempting to perform operations on void
        string message =  "Cannot perform operations on void";
        errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
        
        // Return a dummy variable indicating an error
        return &SymbolTable::stupidVarStruct;
    }

	// Check if either sub-expression is marked as incorrect due to previous errors
    if(!variable1->isCorrect || !variable2->isCorrect) 
	{
        return &SymbolTable::stupidVarStruct;
    }
	
	// Apply the operator and generate the corresponding IR instruction
	cfg.getCurrentBB()->addInstruction(IRInstr::op_xor, {variable1->variableName, variable2->variableName, tmp->variableName}, symbolTable);
	
	// Return the temporary variable holding the result of the logical XOR operation
	return tmp;
}

// Visit an if statement
antlrcpp::Any CodeGenVisitor::visitIfStatement(ifccParser::IfStatementContext *ctx) 
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable* symbolTable = symbolTablesStack.top();

	// Fetch the boolean expression of the if statement
	varStruct* testVar = visit(ctx->exprInstruction(0));

	// Check whether there is an else statment
	bool hasElseStatment = ctx->elseStatement(); 

	// Basic block for the test
	BasicBlock * testBB = cfg.getCurrentBB();

	// Stores the name of the boolean test variable within the basic block for the test
	testBB->setTestVariableName(testVar->variableName);

	// Create a 'then' basic block
	BasicBlock * thenBB = cfg.createBB();
		
	// Create a basic block for the code following the if/else statement
	BasicBlock* endIfBB = cfg.createBB();

	// Set its exit pointers to the ones of the parent basic block
	endIfBB->setExitTrue(testBB->getExitTrue());
	endIfBB->setExitFalse(testBB->getExitFalse());
	
	// Set the parent's true exit pointer to the 'then' basic block
	testBB->setExitTrue(thenBB);
	
	if (hasElseStatment)  // If there's both a 'then' and an 'else' statement
	{
		// Create an 'else' basic block
		BasicBlock* elseBB = cfg.createBB();

		// Set the parent's false exit pointer to it
		testBB->setExitFalse(elseBB);

		// Set the 'else's basic block true exit pointer to the following basic block
        elseBB->setExitTrue(endIfBB);
        elseBB->setExitFalse(nullptr);
				
		// Write jump instructions
        testBB->addInstruction(IRInstr::conditional_jump, {testBB->getTestVariableName(), testBB->getExitFalse()->getLabel(), testBB->getExitTrue()->getLabel()}, symbolTable);
    
        // Visit else body
        cfg.setCurrentBB(elseBB);
        visit(ctx->elseStatement());
	}
	else  // If there's only a 'then' statement
	{
		// Set the parent's false exit pointer to the following basic block
        testBB->setExitFalse(endIfBB);

        // Write jump instructions
        testBB->addInstruction(IRInstr::conditional_jump, {testBB->getTestVariableName(), testBB->getExitFalse()->getLabel(), testBB->getExitTrue()->getLabel()}, symbolTable);
    }

	// Set the 'then's basic block true exit pointer to the following basic block
    thenBB->setExitTrue(endIfBB);
    thenBB->setExitFalse(nullptr);
	
	// Visit then body or expression
	cfg.setCurrentBB(thenBB);

	if (ctx->body()) 
	{
		visit(ctx->beginBlock());
		visit(ctx->body());

		// Write instruction to jump back to the following block
		visit(ctx->endBlock());
	} 
	else if (ctx->exprInstruction(1))
	{
		visit(ctx->exprInstruction(1));

		// Write instruction to jump back to the following block
		thenBB->addInstruction(IRInstr::absolute_jump, {thenBB->getExitTrue()->getLabel()}, symbolTable);
	} 
	else if (ctx->returnStatement())
	{
		visit(ctx->returnStatement());

		// Write instruction to jump back to the following block
		thenBB->addInstruction(IRInstr::absolute_jump, {thenBB->getExitTrue()->getLabel()}, symbolTable);
	}
	
	// Set the next current basic block
	cfg.setCurrentBB(endIfBB);

	return 0;
}

// Visit an else statement
antlrcpp::Any CodeGenVisitor::visitElseStatement(ifccParser::ElseStatementContext *ctx)
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable * symbolTable = symbolTablesStack.top();

	// Basic block for the test
	BasicBlock * elseBB = cfg.getCurrentBB();

	if (ctx->body()) 
	{
		// Visit the begin block and body of the else statement
		visit(ctx->beginBlock());
		visit(ctx->body());

		// Write instruction to jump back to the following block after executing the else statement
        visit(ctx->endBlock());
	} 
	else if (ctx->exprInstruction())
	{
		visit(ctx->exprInstruction());

		// Write instruction to jump back to the following block after executing the else statement
        elseBB->addInstruction(IRInstr::absolute_jump, {elseBB->getExitTrue()->getLabel()}, symbolTable);
    }
	 else if (ctx->returnStatement())
    {
        // Visit the return statement in the else statement
        visit(ctx->returnStatement());

        // Write instruction to jump back to the following block after executing the else statement
        elseBB->addInstruction(IRInstr::absolute_jump, {elseBB->getExitTrue()->getLabel()}, symbolTable);
    }

	return 0;
}

// Visit a while statement
antlrcpp::Any CodeGenVisitor::visitWhileStatement(ifccParser::WhileStatementContext *ctx) 
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable* symbolTable = symbolTablesStack.top();

	// Basic block before the while expression
	BasicBlock* beforeWhileBB = cfg.getCurrentBB();

	// Create a basic block that will contain the condition
	BasicBlock* testBB = cfg.createBB();

	// Set current basic block to the block for the condition
	cfg.setCurrentBB(testBB);

	// Fetch the condition of the while loop and store the boolean test variable name
    varStruct* testVar = visit(ctx->exprInstruction(0));
    testBB->setTestVariableName(testVar->variableName);

	// Create a basic block that will contain the body of the while loop
	BasicBlock* bodyBB = cfg.createBB();
	
	// Create a basic block that will contain the code after the while loop
	BasicBlock* afterWhileBB = cfg.createBB();

	// Set the exit pointers of the afterWhileBB to the ones of the parent BB
	afterWhileBB->setExitTrue(beforeWhileBB->getExitTrue());
	afterWhileBB->setExitFalse(beforeWhileBB->getExitFalse());
	
	// Set beforeWhileBB exit to testBB
	beforeWhileBB->setExitTrue(testBB);
	beforeWhileBB->setExitFalse(nullptr);
	
	// Set the true exit pointer of the test block to the body block
	testBB->setExitTrue(bodyBB);

	// Set the false exit pointer of the test block to the block after the while
	testBB->setExitFalse(afterWhileBB);

	// Set the true exit pointer of the body block to the test block
	bodyBB->setExitTrue(testBB);
	bodyBB->setExitFalse(nullptr);

	// Visit body of the while loop
	cfg.setCurrentBB(bodyBB);

	if (ctx->body()) 
	{
		// Visit the begin block, body, and end block of the while loop
		visit(ctx->beginBlock());
		visit(ctx->body());
		visit(ctx->endBlock());
	} 
	else if (ctx->exprInstruction(1)) 
	{
		// Visit the expression instruction inside the body of the while loop
		visit(ctx->exprInstruction(1));
	} 
	else if (ctx->returnStatement()) 
	{
		// Visit the return statement inside the body of the while loop
		visit(ctx->returnStatement());
	} 
	
	// Write jump instructions for the while loop
	beforeWhileBB->addInstruction(IRInstr::absolute_jump, {beforeWhileBB->getExitTrue()->getLabel()}, symbolTable);
	testBB->addInstruction(IRInstr::conditional_jump, {testBB->getTestVariableName(), testBB->getExitFalse()->getLabel(), testBB->getExitTrue()->getLabel()}, symbolTable);
	bodyBB->addInstruction(IRInstr::absolute_jump, {bodyBB->getExitTrue()->getLabel()}, symbolTable);

	// Set the next current BB to the block after the while loop
	cfg.setCurrentBB(afterWhileBB);

	return 0;
}

// Visit an assignment operation with arithmetic operator.
antlrcpp::Any CodeGenVisitor::visitPmmdEqual(ifccParser::PmmdEqualContext *ctx)
{
	// Fetch the expression on the right-hand side
    varStruct* rightExpr = visit(ctx->expr());

    // Fetch the variable (on the left-hand side)
    string leftExpr = ctx->VAR()->getText();

	// Fetch the operator
    string op = ctx->OPPMMD->getText();

    // Get the symbol table from the top of the symbol tables stack
    SymbolTable* symbolTable = symbolTablesStack.top();

	// Generate instructions based on the operator
    if (op == "+=") 
	{
        cfg.getCurrentBB()->addInstruction(IRInstr::op_plus_equal, {leftExpr, rightExpr->variableName}, symbolTable);
    } 
	else if (op == "-=") 
	{
        cfg.getCurrentBB()->addInstruction(IRInstr::op_sub_equal, {leftExpr, rightExpr->variableName}, symbolTable);
    } 
	else if (op == "*=") 
	{
        cfg.getCurrentBB()->addInstruction(IRInstr::op_mult_equal, {leftExpr, rightExpr->variableName}, symbolTable);
    } 
	else 
	{
        cfg.getCurrentBB()->addInstruction(IRInstr::op_div_equal, {leftExpr, rightExpr->variableName}, symbolTable);
    }

	// Mark the temporary variable as used
	symbolTable->getVariable(leftExpr)->isUsed = true;

	// Return the variable structure representing the left-hand side variable
    return symbolTable->getVariable(leftExpr);
}

// Visit a return statement with an expression
antlrcpp::Any CodeGenVisitor::visitExpReturnStatement(ifccParser::ExpReturnStatementContext * ctx) 
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable * symbolTable = symbolTablesStack.top();

	// Set the flag indicating that a return statement with an expression is encountered
    symbolTable->setReturned(true);

	// Save the current stack pointer
	int currStackPointer = symbolTable->getStackPointer();

	// Compute the expression
	varStruct * result = visit(ctx->exprInstruction());
	
	// Retrieve the function return type
	string funcParent = ctx->parent->parent->getText();
	string returnType = funcParent.substr(0, 4); // if it's void, it's the 4th first letter

	// Verify if returnType == void
	if (returnType == "void" && result->variableType != "void")
    {
		// Generate a warning message for empty return return value in a void function
		string message =  "'return' with a value, in function returning void '" + currentFunction + "'";
		errorHandler.signal(WARNING, message, ctx->getStart()->getLine());
	}

	// Check for errors
    if (result->variableType == "void") 
    {
        // Signal an error if attempting to return a void expression
        string message =  "Cannot perform operations on void";
        errorHandler.signal(ERROR, message, ctx->getStart()->getLine());
        
        // Return a dummy variable indicating an error
        return &SymbolTable::stupidVarStruct;
    }

	if (!result->isCorrect) 
	{
		// If the expression evaluation is incorrect, add a return instruction without value
		cfg.getCurrentBB()->addInstruction(IRInstr::ret, {}, symbolTable);
        return 1;
    }

	// Reset the stack pointer and temp variable counter after evaluating the expression
    symbolTable->setStackPointer(currStackPointer);
    
    // Add actual return instructions with the result of the expression
    cfg.getCurrentBB()->addInstruction(IRInstr::ret, {result->variableName}, symbolTable);

    return 0;
}

// Visit an empty return statement
antlrcpp::Any CodeGenVisitor::visitEmptyReturnStatement(ifccParser::EmptyReturnStatementContext *ctx) 
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable * sT = symbolTablesStack.top();

	// Set the returned flag to true
	sT->setReturned(true);

	// Check for warnings if the function has a non-void return type
	funcStruct * function = globalSymbolTable->getFunction(currentFunction);
	
    if (function->returnType != "void") 
    {
		// Generate a warning message for empty return in a non-void function
		string message =  "Use of empty 'return;' in non-void function '" + currentFunction + "'";
		errorHandler.signal(WARNING, message, ctx->getStart()->getLine());
	}

	char* wsl_env = getenv("WSLENV");

    if (wsl_env != NULL)  // WSL Case
	{
		// Add actual return instructions, specifying the appropriate return value
		// If the current function is 'main', return 41 (EXIT_SUCCESS); otherwise, return 0
		cfg.getCurrentBB()->addInstruction(IRInstr::ret, {(currentFunction == "main") ? "$41" : "$0"}, sT);
    } 
	else // Linux Case
	{
        // Add actual return instructions, specifying the appropriate return value
		// If the current function is 'main', return 37 (EXIT_SUCCESS); otherwise, return 0
		cfg.getCurrentBB()->addInstruction(IRInstr::ret, {(currentFunction == "main") ? "$37" : "$0"}, sT);
    }
	
	return 0;
}

// Handle the generation of default return instructions.
void CodeGenVisitor::returnDefault(antlr4::ParserRuleContext *ctx) 
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable * symbolTable = symbolTablesStack.top();

	// Set the returned flag to true
	symbolTable->setReturned(true);

	// Retrieve function information
	funcStruct * function = globalSymbolTable->getFunction(currentFunction);
	
	// Check for warnings if the function has a non-void return type
    if (function->returnType != "void") 
    {
		// Generate a warning message for missing return in a non-void function
		string message =  "No 'return' found in non-void function '" + currentFunction + "'";
		errorHandler.signal(WARNING, message, ctx->getStart()->getLine());
	}

	// Determine if the default return value should be 41 or 37 (EXIT_SUCCESS) based on the main function
    bool returnExitSuccess = currentFunction == "main" && function->returnType == "void";

	char* wsl_env = getenv("WSLENV");

    if (wsl_env != NULL)  // WSL Case
	{
		// Add actual return instructions, specifying the appropriate return value
    	cfg.getCurrentBB()->addInstruction(IRInstr::ret, {(returnExitSuccess) ? "$41" : "$0"}, symbolTable);
    } 
	else // Linux Case
	{
        // Add actual return instructions, specifying the appropriate return value
    	cfg.getCurrentBB()->addInstruction(IRInstr::ret, {(returnExitSuccess) ? "$37" : "$0"}, symbolTable);
    }
}

// Create a temporary variable. 
varStruct* CodeGenVisitor::createTmpVariable(antlr4::ParserRuleContext * ctx, string variableType) 
{
	// Get the symbol table from the top of the symbol tables stack
	SymbolTable * symbolTable = symbolTablesStack.top();
	
	// Increment the temporary variable counter and generate a unique variable name
	tmpVariableCounter++;
	string newVariable = "!tmp" + to_string(tmpVariableCounter);
	string newVariableType = variableType;
	
	// Add the new temporary variable to the symbol table
    symbolTable->addVariable(newVariable, newVariableType, ctx->getStart()->getLine());
	
	// Mark the temporary variable as used
	symbolTable->getVariable(newVariable)->isUsed = true;

	// Return a pointer to the created temporary variable
	return symbolTable->getVariable(newVariable);
}

// Getter for the global symbol table
SymbolTable* CodeGenVisitor::getGlobalSymbolTable() 
{
	return this->globalSymbolTable;
}

// Add the 'putchar' function symbol to the global symbol table
void CodeGenVisitor::addSymbolPutchar() 
{
    globalSymbolTable->addFunction("putchar", "int", 1, {"int"}, {"c"}, 0);
}

// Add the 'getchar' function symbol to the global symbol table
void CodeGenVisitor::addSymbolGetchar() 
{
    globalSymbolTable->addFunction("getchar", "int", -1, {}, {}, 0);
}
