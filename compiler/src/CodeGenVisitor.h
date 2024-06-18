/*************************************************************************
                          PLD Compilateur: CodeGenVisitor
                          ---------------------------
    start   : 26/03/2024
    authors : ANDRIANARISOLO Elie, FARHAT Widad,
			  SARR Seynabou, TONG An Jun
*************************************************************************/

//------ Interface of class <CodeGenVisitor> (file CodeGenVisitor.h) -------/
#pragma once

//--------------------------------------------------- Called interfaces
#include "antlr4-runtime.h"
#include "../generated/ifccBaseVisitor.h"
#include <utility>
#include <vector>
#include <algorithm>
#include "IR/CFG.h"
#include "SymbolTable.h"

//------------------------------------------------------------------------
//
// Goal of class <CodeGenVisitor> :
//
// The goal of this class is to define the visitors for the different
// syntax element.
//
//------------------------------------------------------------------------

class  CodeGenVisitor : public ifccBaseVisitor 
{
        public:
                
                // Constructor
                CodeGenVisitor(ErrorHandler& eH, CFG& cfg);
		
                // Destructor
                ~CodeGenVisitor();
                
                // Linearising functions
	        antlrcpp::Any visitProg(ifccParser::ProgContext *ctx);
		antlrcpp::Any visitMainDeclare(ifccParser::MainDeclareContext *ctx);
		antlrcpp::Any visitMainDeclareHeaderWithReturn(ifccParser::MainDeclareHeaderWithReturnContext *ctx);
		antlrcpp::Any visitMainDeclareHeaderNoReturn(ifccParser::MainDeclareHeaderNoReturnContext *ctx);
                antlrcpp::Any visitFuncDeclare(ifccParser::FuncDeclareContext *ctx); 
		antlrcpp::Any visitFuncDeclareHeader(ifccParser::FuncDeclareContext *ctx);
                antlrcpp::Any visitFuncDeclareBody(ifccParser::FuncDeclareContext *ctx);
                antlrcpp::Any visitBeginBlock(ifccParser::BeginBlockContext *ctx);
                antlrcpp::Any visitEndBlock(ifccParser::EndBlockContext *ctx);
                antlrcpp::Any visitEmptyReturnStatement(ifccParser::EmptyReturnStatementContext *ctx);
                antlrcpp::Any visitExpReturnStatement(ifccParser::ExpReturnStatementContext *ctx);
                antlrcpp::Any visitConstExpr(ifccParser::ConstExprContext *ctx);
                antlrcpp::Any visitVarDeclare(ifccParser::VarDeclareContext *ctx);
                antlrcpp::Any visitVarDeclareAndAffect(ifccParser::VarDeclareAndAffectContext *ctx);
                antlrcpp::Any visitAffectExpr(ifccParser::AffectExprContext *ctx);
                antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx);
                antlrcpp::Any visitFuncExpr(ifccParser::FuncExprContext *ctx);
                antlrcpp::Any visitUnaryExpr(ifccParser::UnaryExprContext *ctx);
                antlrcpp::Any visitAddSubExpr(ifccParser::AddSubExprContext *ctx);
                antlrcpp::Any visitMulDivModExpr(ifccParser::MulDivModExprContext *ctx);
                antlrcpp::Any visitParenthesesExpr(ifccParser::ParenthesesExprContext *ctx);
                antlrcpp::Any visitCmpEqualityExpr(ifccParser::CmpEqualityExprContext *ctx);
                antlrcpp::Any visitCmpLessOrGreaterExpr(ifccParser::CmpLessOrGreaterExprContext *ctx);
                antlrcpp::Any visitCmpEqualityLessGreaterExpr(ifccParser::CmpEqualityLessGreaterExprContext *ctx);
                antlrcpp::Any visitAndExpr(ifccParser::AndExprContext *ctx);
                antlrcpp::Any visitOrExpr(ifccParser::OrExprContext *ctx);
                antlrcpp::Any visitXorExpr(ifccParser::XorExprContext *ctx);
                antlrcpp::Any visitIfStatement(ifccParser::IfStatementContext *ctx);
		antlrcpp::Any visitElseStatement(ifccParser::ElseStatementContext *ctx);
		antlrcpp::Any visitWhileStatement(ifccParser::WhileStatementContext *ctx);
                antlrcpp::Any visitPmmdEqual(ifccParser::PmmdEqualContext *ctx);

                // Getter for the global symbol table
                SymbolTable* getGlobalSymbolTable();

        protected:
                
                // Method for handling default return behavior
                void returnDefault(antlr4::ParserRuleContext *ctx);
                
                // Method for creating temporary variables
                varStruct* createTmpVariable(antlr4::ParserRuleContext *ctx, string varType="int");

                ErrorHandler& errorHandler;             // Reference to the error handler
                CFG& cfg;                               // Reference to the control flow graph
                int tmpVariableCounter = 0;             // Counter for temporary variables
                stack<SymbolTable*> symbolTablesStack;  // Stack to manage symbol tables during code generation
                stack<SymbolTable*> symbolTableGarbage; // Stack to hold symbol tables that can be deleted
                string currentFunction = "";            // Name of the current function being processed
                SymbolTable* globalSymbolTable;         // Pointer to the global symbol table

	private:

                // Add the 'putchar' function symbol to the global symbol table
		void addSymbolPutchar();
		
                // Add the 'getchar' function symbol to the global symbol table
                void addSymbolGetchar();
};

