grammar ifcc;

axiom : prog EOF;

variableType:   TINT 
				| TCHAR 
				;
			
beginBlock : '{' ;
endBlock : '}' ;

prog : (funcDeclare)* mainDeclare (funcDeclare)*;

funcDeclare : FTYPE=('void'|'int'|'char') VAR '(' ( (variableType VAR (',' variableType VAR)*)? | (TVOID)? ) ')' beginBlock body endBlock (';')? ;

mainDeclareHeader:  FTYPE=('void'|'int') 'main' '(' (TVOID)? ')' 	# mainDeclareHeaderWithReturn
					| 'main' '(' (TVOID)? ')' 						# mainDeclareHeaderNoReturn
					;

mainDeclare : mainDeclareHeader beginBlock body endBlock (';')? ;

body :	declareStatement ';' body 
		| exprInstruction ';' body
		| returnStatement ';' body
		| ifStatement body
		| whileStatement body
		| beginBlock body endBlock (';')? body
		|
		;

declareStatement :	varDeclare 
		            | varDeclareAndAffect
			     	;
				 
varDeclare : variableType VAR (',' VAR)* ;

varDeclareAndAffect : variableType VAR '=' exprInstruction ;

exprInstruction : 	affect
					| expr
					;
				
affect :    VAR '=' exprInstruction						# affectExpr
            | VAR OPPMMD=('+='|'-='|'*='|'/=') expr   	# pmmdEqual
	      	;

expr :	'(' exprInstruction ')' 		 	# parenthesesExpr
	    | UNARY=('-'|'!') expr 		   		# unaryExpr
	    | expr OP1=('*'|'/'|'%') expr 		# mulDivModExpr	
	    | expr OP2=('+'|'-') expr 			# addSubExpr
	    | expr CMP=('<' | '>') expr			# cmpLessOrGreaterExpr
	    | expr EQ=('=='|'!=') expr			# cmpEqualityExpr
	    | expr EQLG=('<='|'>=') expr		# cmpEqualityLessGreaterExpr
	    | expr '&' expr						# andExpr
	    | expr '^' expr						# xorExpr
	    | expr '|' expr						# orExpr
	    | VAR '(' (expr (',' expr)*)? ')'	# funcExpr
	    | CONST 							# constExpr 
	    | VAR								# varExpr
	    ;

ifStatement :  	'if' '(' exprInstruction ')' beginBlock body endBlock (elseStatement)?
				| 'if' '(' exprInstruction ')' exprInstruction ';' (elseStatement)?
				| 'if' '(' exprInstruction ')' returnStatement ';' (elseStatement)?
				;

elseStatement : 'else' beginBlock body endBlock
		      	|'else' exprInstruction ';'
				|'else' returnStatement ';'
				;

whileStatement :  	'while' '(' exprInstruction ')' beginBlock body endBlock
					|'while' '(' exprInstruction ')' exprInstruction ';'
					|'while' '(' exprInstruction ')' returnStatement ';'
					;

returnStatement : 	RETURN exprInstruction	# expReturnStatement
					| RETURN 				# emptyReturnStatement
					;


RETURN : 'return' ;
CONST : NUMBER | CHAR ;
NUMBER : [0-9]+ ;
CHAR : '\'' . '\'' ;
TINT : 'int' ;
TCHAR : 'char' ;
TVOID: 'void' ;
VAR : [a-zA-Z_][a-zA-Z0-9_]* ;
MULTICOMMENT : '/*' .*? '*/' -> skip ;
SINGLECOMMENT : '//' .*? '\n' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;			 
WS : [ \t\r\n] -> channel(HIDDEN) ;