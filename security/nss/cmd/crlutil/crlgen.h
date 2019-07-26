




#ifndef _CRLGEN_H_
#define _CRLGEN_H_

#include "prio.h"
#include "prprf.h"
#include "plhash.h"
#include "seccomon.h"
#include "certt.h"
#include "secoidt.h"


#define CRLGEN_UNKNOWN_CONTEXT                   0
#define CRLGEN_ISSUER_CONTEXT                    1
#define CRLGEN_UPDATE_CONTEXT                    2
#define CRLGEN_NEXT_UPDATE_CONTEXT               3
#define CRLGEN_ADD_EXTENSION_CONTEXT             4
#define CRLGEN_ADD_CERT_CONTEXT                  6
#define CRLGEN_CHANGE_RANGE_CONTEXT              7
#define CRLGEN_RM_CERT_CONTEXT                   8

#define CRLGEN_TYPE_DATE                         0
#define CRLGEN_TYPE_ZDATE                        1
#define CRLGEN_TYPE_DIGIT                        2
#define CRLGEN_TYPE_DIGIT_RANGE                  3
#define CRLGEN_TYPE_OID                          4
#define CRLGEN_TYPE_STRING                       5
#define CRLGEN_TYPE_ID                           6


typedef struct CRLGENGeneratorDataStr          CRLGENGeneratorData;
typedef struct CRLGENEntryDataStr              CRLGENEntryData;
typedef struct CRLGENExtensionEntryStr         CRLGENExtensionEntry;
typedef struct CRLGENCertEntrySrt              CRLGENCertEntry;
typedef struct CRLGENCrlFieldStr               CRLGENCrlField;
typedef struct CRLGENEntriesSortedDataStr      CRLGENEntriesSortedData;





extern SECStatus CRLGEN_ExtHandleInit(CRLGENGeneratorData *crlGenData);


extern SECStatus CRLGEN_CommitExtensionsAndEntries(CRLGENGeneratorData *crlGenData);


extern SECStatus CRLGEN_StartCrlGen(CRLGENGeneratorData *crlGenData);


extern void CRLGEN_FinalizeCrlGeneration(CRLGENGeneratorData *crlGenData);



extern CRLGENGeneratorData* CRLGEN_InitCrlGeneration(CERTSignedCrl *newCrl,
                                                     PRFileDesc *src);





extern void CRLGEN_InitCrlGenParserLock();
extern void CRLGEN_DestroyCrlGenParserLock();





typedef SECStatus updateCrlFn_t(CRLGENGeneratorData *crlGenData, void *str);
typedef SECStatus setNextDataFn_t(CRLGENGeneratorData *crlGenData, void *str,
                                  void *data, unsigned short dtype);
typedef SECStatus createNewLangStructFn_t(CRLGENGeneratorData *crlGenData,
                                          void *str, unsigned i);


extern void      crlgen_setFailure(CRLGENGeneratorData *str, char *);




extern SECStatus crlgen_setNextData(CRLGENGeneratorData *str, void *data,
                             unsigned short dtype);



extern SECStatus crlgen_updateCrl(CRLGENGeneratorData *str);



extern SECStatus crlgen_createNewLangStruct(CRLGENGeneratorData *str,
                                            unsigned structType);









struct CRLGENExtensionEntryStr {
    char **extData;
    int    nextUpdatedData;
    updateCrlFn_t    *updateCrlFn;
    setNextDataFn_t  *setNextDataFn;
};






struct CRLGENCertEntrySrt {
    char *certId;
    char *revocationTime;
    updateCrlFn_t   *updateCrlFn;
    setNextDataFn_t *setNextDataFn;
};





struct CRLGENCrlFieldStr {
    char *value;
    updateCrlFn_t   *updateCrlFn;
    setNextDataFn_t *setNextDataFn;
};







struct CRLGENEntryDataStr {
    SECItem *certId;
    void *extHandle;
    CERTCrlEntry *entry;
};
























 
struct CRLGENGeneratorDataStr {
    unsigned short contextId;
    CRLGENCrlField       *crlField;
    CRLGENCertEntry      *certEntry;
    CRLGENExtensionEntry *extensionEntry;	
    PRUint64 rangeFrom;
    PRUint64 rangeTo;
    CERTSignedCrl *signCrl;
    void *crlExtHandle;
    PLHashTable *entryDataHashTable;
    
    PRFileDesc *src;
    int parsedLineNum;
};


#endif 
