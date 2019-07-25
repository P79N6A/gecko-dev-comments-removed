





































#ifndef MITRE_ERROROBSERVER_H
#define MITRE_ERROROBSERVER_H

#include "txCore.h"
#ifdef TX_EXE
#include <iostream.h>
#endif




class ErrorObserver {

public:

    


    virtual ~ErrorObserver() {};

    


    virtual void receiveError(const nsAString& errorMessage, nsresult aRes) = 0;

    



    void receiveError(const nsAString& errorMessage)
    {
        receiveError(errorMessage, NS_ERROR_FAILURE);
    }

        

}; 

#ifdef TX_EXE



class SimpleErrorObserver : public ErrorObserver {

public:

    



    SimpleErrorObserver();

    



    SimpleErrorObserver(ostream& errStream);

    


    void receiveError(const nsAString& errorMessage, nsresult aRes);

    virtual void suppressWarnings(MBool suppress);

private:

    ostream* errStream;
    MBool hideWarnings;
}; 
#endif

#endif

