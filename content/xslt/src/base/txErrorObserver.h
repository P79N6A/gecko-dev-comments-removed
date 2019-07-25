





































#ifndef MITRE_ERROROBSERVER_H
#define MITRE_ERROROBSERVER_H

#include "txCore.h"




class ErrorObserver {

public:

    


    virtual ~ErrorObserver() {};

    


    virtual void receiveError(const nsAString& errorMessage, nsresult aRes) = 0;

    



    void receiveError(const nsAString& errorMessage)
    {
        receiveError(errorMessage, NS_ERROR_FAILURE);
    }

        

}; 

#endif
