/*************************************************************************
                          PLD Compilateur: ErrorHandler
                          ---------------------------
    start   : 26/03/2024
    authors : ANDRIANARISOLO Elie, FARHAT Widad,
			  SARR Seynabou, TONG An Jun
*************************************************************************/

//------ Interface of class <ErrorHandler> (file ErrorHandler.h) -------/
#pragma once

//--------------------------------------------------- Called interfaces
#include <iostream>
#include <string>

using namespace std;

//------------------------------------------------------------------ Types
enum errorType {ERROR, WARNING}; // Enumeration for error types

//------------------------------------------------------------------------
//
// Goal of class <ErrorHandler> :
//
// The goal of this class is to handle errors and warnings
//
//------------------------------------------------------------------------

class ErrorHandler 
{
    public:

        // Signal an error or warning
        void signal(int severity, string message, int lineNumber);
        
        // Check if there are any errors
        bool hasError();
        
        // Check if there are any warnings
        bool hasWarning();
        
    protected:

        // Static array to hold error type strings
        static string errorValues[2];
        
        // Flags to indicate presence of error or warning
        bool error = false;
        bool warning = false;
        
        // Generate error message based on severity
        void generateErrorMessage(int severity, string message, int lineNumber); 
};