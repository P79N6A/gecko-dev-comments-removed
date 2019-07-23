








































#if defined(_RCNETDB_H)
#else
#define _RCNETDB_H

#include "rclock.h"
#include "rcbase.h"

#include <prnetdb.h>

class PR_IMPLEMENT(RCNetAddr): public RCBase
{
public:
    typedef enum {
        any = PR_IpAddrAny,             
        loopback = PR_IpAddrLoopback    
    } HostValue;

    RCNetAddr();                        
    RCNetAddr(const RCNetAddr&);        
    RCNetAddr(HostValue, PRUint16 port);
    RCNetAddr(const RCNetAddr&, PRUint16 port);
                                        

    virtual ~RCNetAddr();

    void operator=(const RCNetAddr&);

    virtual PRBool operator==(const RCNetAddr&) const;
                                        
    virtual PRBool EqualHost(const RCNetAddr&) const;
                                        


public:

    void operator=(const PRNetAddr*);   
    operator const PRNetAddr*() const;  
    virtual PRStatus FromString(const char* string);
                                        
    virtual PRStatus ToString(char *string, PRSize size) const;
                                        

private:

    PRNetAddr address;

};  











class RCHostLookup: public RCBase
{
public:
    virtual ~RCHostLookup();

    RCHostLookup();

    virtual PRStatus ByName(const char* name);
    virtual PRStatus ByAddress(const RCNetAddr&);

    virtual const RCNetAddr* operator[](PRUintn);

private:
    RCLock ml;
    PRIntn max_index;
    RCNetAddr* address;

    RCHostLookup(const RCHostLookup&);
    RCHostLookup& operator=(const RCHostLookup&);
};

inline RCNetAddr::RCNetAddr(): RCBase() { }
inline RCNetAddr::operator const PRNetAddr*() const { return &address; }


#endif 




