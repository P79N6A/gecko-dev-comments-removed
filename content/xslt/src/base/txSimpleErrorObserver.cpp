





































#include "txErrorObserver.h"
#include "nsString.h"





SimpleErrorObserver::SimpleErrorObserver() {
#ifdef TX_EXE
    errStream = &cout;
#endif
    hideWarnings = MB_FALSE;
} 





SimpleErrorObserver::SimpleErrorObserver(ostream& errStream) {
    this->errStream = &errStream;
    hideWarnings = MB_FALSE;
} 




void SimpleErrorObserver::receiveError(const nsAString& errorMessage,
                                       nsresult aRes)
{
#ifdef TX_EXE
    if (NS_FAILED(aRes)) {
        *errStream << "error: ";
    }

    *errStream << NS_LossyConvertUTF16toASCII(errorMessage).get() << endl;
    errStream->flush();
#endif
}

void SimpleErrorObserver::supressWarnings(MBool supress) {
    this->hideWarnings = supress;
} 
