





































#ifndef _PK11_INIT_H_
#define _PK11_INIT_H_ 1




struct PK11PreSlotInfoStr {
    CK_SLOT_ID slotID;  	
    unsigned long defaultFlags; 

    int askpw;			
    long timeout;		
    char hasRootCerts;		
    char hasRootTrust;		
};

#endif 
