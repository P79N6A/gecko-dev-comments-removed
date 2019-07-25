




#ifndef __nsUCConstructors_h
#define __nsUCConstructors_h

#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeDecoder.h"
#include "uconvutil.h"


NS_METHOD
CreateMultiTableDecoder(int32_t aTableCount,
                        const uRange * aRangeArray, 
                        uScanClassID * aScanClassArray,
                        uMappingTable ** aMappingTable,
                        uint32_t aMaxLengthFactor,
                        
                        nsISupports* aOuter,
                        REFNSIID aIID,
                        void** aResult);

NS_METHOD
CreateMultiTableEncoder(int32_t aTableCount,
                        uScanClassID * aScanClassArray,
                        uShiftOutTable ** aShiftOutTable,
                        uMappingTable  ** aMappingTable,
                        uint32_t aMaxLengthFactor,
                        nsISupports* aOuter,
                        REFNSIID aIID,
                        void** aResult);

NS_METHOD
CreateTableEncoder(uScanClassID aScanClass,
                   uShiftOutTable * aShiftOutTable,
                   uMappingTable  * aMappingTable,
                   uint32_t aMaxLengthFactor,
                   nsISupports* aOuter,
                   REFNSIID aIID,
                   void** aResult);

NS_METHOD
CreateMultiTableEncoder(int32_t aTableCount,
                        uScanClassID * aScanClassArray,
                        uMappingTable  ** aMappingTable,
                        uint32_t aMaxLengthFactor,
                        nsISupports* aOuter,
                        REFNSIID aIID,
                        void** aResult);

NS_METHOD
CreateTableEncoder(uScanClassID aScanClass,
                   uMappingTable  * aMappingTable,
                   uint32_t aMaxLengthFactor,
                   nsISupports* aOuter,
                   REFNSIID aIID,
                   void** aResult);

NS_METHOD
CreateTableDecoder(uScanClassID aScanClass,
                   uShiftInTable * aShiftInTable,
                   uMappingTable * aMappingTable,
                   uint32_t aMaxLengthFactor,
                   nsISupports* aOuter,
                   REFNSIID aIID,
                   void** aResult);

NS_METHOD
CreateOneByteDecoder(uMappingTable * aMappingTable,
                     nsISupports* aOuter,
                     REFNSIID aIID,
                     void** aResult);

                   
#endif
