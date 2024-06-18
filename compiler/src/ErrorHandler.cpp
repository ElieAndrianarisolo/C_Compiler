/*************************************************************************
                          PLD Compilateur: ErrorHandler
                          ---------------------------
    start   : 26/03/2024
    authors : ANDRIANARISOLO Elie, FARHAT Widad,
			  SARR Seynabou, TONG An Jun
*************************************************************************/

//---- Implementation of class <ErrorHandler> (file ErrorHandler.cpp) -----/

#include "ErrorHandler.h"

using namespace std;

string ErrorHandler::errorValues[2] = {"ERROR", "WARNING"};

// Static initialization of error type strings
void ErrorHandler::signal(int severity, string message, int lineNumber) 
{
	// Update error or warning flag based on severity
	switch(severity) 
	{
		case WARNING:
		{
			warning = true; 
			break;
		}

		case ERROR: 
		{
			error = true; 
			break;
		}
	}

	// Generate and print error message
	generateErrorMessage(severity, message, lineNumber);
}

// Generate error message based on severity
void ErrorHandler::generateErrorMessage(int severity, string message, int lineNumber) 
{
	// Output error type
    cerr << errorValues[severity];

	// Output line number if provided
	if (lineNumber >= 0) 
	{
		cerr << " at line " << lineNumber;
	}

	// Output error message
	cerr << " : " << message << "." << endl;
}

// Check if there are any errors
bool ErrorHandler::hasError() 
{
	return error;
}

// Check if there are any warnings
bool ErrorHandler::hasWarning() 
{
	return warning;
}