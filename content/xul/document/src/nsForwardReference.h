





































#ifndef nsForwardReference_h__
#define nsForwardReference_h__

class nsForwardReference
{
protected:
    nsForwardReference() {}

public:
    virtual ~nsForwardReference() {}

    


    enum Phase {
        
        eStart,

        

        eConstruction,

        

        eHookup,

        
        eDone
    };

    





    static const Phase kPasses[];

    






    virtual Phase GetPhase() = 0;

    


    enum Result {
        
        eResolve_Succeeded,

        
        eResolve_Later,

        
        eResolve_Error
    };

    





    virtual Result Resolve() = 0;
};

#endif 
