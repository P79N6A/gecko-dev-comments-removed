



















#include "unicode/region.h"
#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "unicode/ures.h"
#include "unicode/decimfmt.h"
#include "ucln_in.h"
#include "cstring.h"
#include "uhash.h"
#include "umutex.h"
#include "uresimp.h"
#include "region_impl.h"

#if !UCONFIG_NO_FORMATTING


U_CDECL_BEGIN

static void U_CALLCONV
deleteRegion(void *obj) {
    delete (icu::Region *)obj;
}




static UBool U_CALLCONV region_cleanup(void)
{
    icu::Region::cleanupRegionData();

    return TRUE;
}

U_CDECL_END

U_NAMESPACE_BEGIN

static UMutex gRegionDataLock = U_MUTEX_INITIALIZER;
static UBool regionDataIsLoaded = false;
static UVector* availableRegions[URGN_LIMIT];

static UHashtable *regionAliases;
static UHashtable *regionIDMap;
static UHashtable *numericCodeMap;

static const UChar UNKNOWN_REGION_ID [] = { 0x5A, 0x5A, 0 };  
static const UChar OUTLYING_OCEANIA_REGION_ID [] = { 0x51, 0x4F, 0 };  
static const UChar WORLD_ID [] = { 0x30, 0x30, 0x31, 0 };  

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RegionNameEnumeration)









void Region::loadRegionData() {

    if (regionDataIsLoaded) {
        return;
    }

    umtx_lock(&gRegionDataLock);

    if (regionDataIsLoaded) { 
        umtx_unlock(&gRegionDataLock);
        return;
    }


    UErrorCode status = U_ZERO_ERROR;

    UResourceBundle* regionCodes = NULL;
    UResourceBundle* territoryAlias = NULL;
    UResourceBundle* codeMappings = NULL;
    UResourceBundle* worldContainment = NULL;
    UResourceBundle* territoryContainment = NULL;
    UResourceBundle* groupingContainment = NULL;

    DecimalFormat *df = new DecimalFormat(status);
    if (U_FAILURE(status)) {
        umtx_unlock(&gRegionDataLock);
        return;
    }
    df->setParseIntegerOnly(TRUE);

    regionIDMap = uhash_open(uhash_hashUnicodeString,uhash_compareUnicodeString,NULL,&status);
    uhash_setValueDeleter(regionIDMap, deleteRegion);

    numericCodeMap = uhash_open(uhash_hashLong,uhash_compareLong,NULL,&status);

    regionAliases = uhash_open(uhash_hashUnicodeString,uhash_compareUnicodeString,NULL,&status);
    uhash_setKeyDeleter(regionAliases,uprv_deleteUObject);

    UResourceBundle *rb = ures_openDirect(NULL,"metadata",&status);
    regionCodes = ures_getByKey(rb,"regionCodes",NULL,&status);
    territoryAlias = ures_getByKey(rb,"territoryAlias",NULL,&status);

    UResourceBundle *rb2 = ures_openDirect(NULL,"supplementalData",&status);
    codeMappings = ures_getByKey(rb2,"codeMappings",NULL,&status);

    territoryContainment = ures_getByKey(rb2,"territoryContainment",NULL,&status);
    worldContainment = ures_getByKey(territoryContainment,"001",NULL,&status);
    groupingContainment = ures_getByKey(territoryContainment,"grouping",NULL,&status);

    UVector *continents = new UVector(uprv_deleteUObject, uhash_compareUnicodeString, status);

    while ( ures_hasNext(worldContainment) ) {
        UnicodeString *continentName = new UnicodeString(ures_getNextUnicodeString(worldContainment,NULL,&status));
        continents->addElement(continentName,status);
    }

    UVector *groupings = new UVector(uprv_deleteUObject, uhash_compareUnicodeString, status);
    while ( ures_hasNext(groupingContainment) ) {
        UnicodeString *groupingName = new UnicodeString(ures_getNextUnicodeString(groupingContainment,NULL,&status));
        groupings->addElement(groupingName,status);
    }

    while ( ures_hasNext(regionCodes) ) {
        UnicodeString regionID = ures_getNextUnicodeString(regionCodes,NULL,&status);
        Region *r = new Region();
        r->idStr = regionID;
        r->idStr.extract(0,r->idStr.length(),r->id,sizeof(r->id),US_INV);
        r->type = URGN_TERRITORY; 

        uhash_put(regionIDMap,(void *)&(r->idStr),(void *)r,&status);
        Formattable result;
        UErrorCode ps = U_ZERO_ERROR;
        df->parse(r->idStr,result,ps);
        if ( U_SUCCESS(ps) ) {
            r->code = result.getLong(); 
            uhash_iput(numericCodeMap,r->code,(void *)r,&status);
            r->type = URGN_SUBCONTINENT;
        } else {
            r->code = -1;
        }
    }


    
    while ( ures_hasNext(territoryAlias) ) {
        UResourceBundle *res = ures_getNextResource(territoryAlias,NULL,&status);
        const char *aliasFrom = ures_getKey(res);
        UnicodeString* aliasFromStr = new UnicodeString(aliasFrom, -1, US_INV);
        UnicodeString aliasTo = ures_getUnicodeString(res,&status);
        ures_close(res);

        Region *aliasToRegion = (Region *) uhash_get(regionIDMap,&aliasTo);
        Region *aliasFromRegion = (Region *)uhash_get(regionIDMap,aliasFromStr);

        if ( aliasToRegion != NULL && aliasFromRegion == NULL ) { 
            uhash_put(regionAliases,(void *)aliasFromStr, (void *)aliasToRegion,&status);
        } else {
            if ( aliasFromRegion == NULL ) { 
                aliasFromRegion = new Region();
                aliasFromRegion->idStr.setTo(*aliasFromStr);
                aliasFromRegion->idStr.extract(0,aliasFromRegion->idStr.length(),aliasFromRegion->id,sizeof(aliasFromRegion->id),US_INV);
                uhash_put(regionIDMap,(void *)&(aliasFromRegion->idStr),(void *)aliasFromRegion,&status);
                Formattable result;
                UErrorCode ps = U_ZERO_ERROR;
                df->parse(aliasFromRegion->idStr,result,ps);
                if ( U_SUCCESS(ps) ) {
                    aliasFromRegion->code = result.getLong(); 
                    uhash_iput(numericCodeMap,aliasFromRegion->code,(void *)aliasFromRegion,&status);
                } else {
                    aliasFromRegion->code = -1;
                }
                aliasFromRegion->type = URGN_DEPRECATED;
            } else {
                aliasFromRegion->type = URGN_DEPRECATED;
            }
            delete aliasFromStr;

            aliasFromRegion->preferredValues = new UVector(uprv_deleteUObject, uhash_compareUnicodeString, status);
            UnicodeString currentRegion;
            currentRegion.remove();
            for (int32_t i = 0 ; i < aliasTo.length() ; i++ ) {
                if ( aliasTo.charAt(i) != 0x0020 ) {
                    currentRegion.append(aliasTo.charAt(i));
                }
                if ( aliasTo.charAt(i) == 0x0020 || i+1 == aliasTo.length() ) {
                    Region *target = (Region *)uhash_get(regionIDMap,(void *)&currentRegion);
                    if (target) {
                        UnicodeString *preferredValue = new UnicodeString(target->idStr);
                        aliasFromRegion->preferredValues->addElement((void *)preferredValue,status);
                    }
                    currentRegion.remove();
                }
            }
        }
    }

    
    while ( ures_hasNext(codeMappings) ) {
        UResourceBundle *mapping = ures_getNextResource(codeMappings,NULL,&status);
        if ( ures_getType(mapping) == URES_ARRAY && ures_getSize(mapping) == 3) {
            UnicodeString codeMappingID = ures_getUnicodeStringByIndex(mapping,0,&status);
            UnicodeString codeMappingNumber = ures_getUnicodeStringByIndex(mapping,1,&status);
            UnicodeString codeMapping3Letter = ures_getUnicodeStringByIndex(mapping,2,&status);

            Region *r = (Region *)uhash_get(regionIDMap,(void *)&codeMappingID);
            if ( r ) {
                Formattable result;
                UErrorCode ps = U_ZERO_ERROR;
                df->parse(codeMappingNumber,result,ps);
                if ( U_SUCCESS(ps) ) {
                    r->code = result.getLong(); 
                    uhash_iput(numericCodeMap,r->code,(void *)r,&status);
                }
                UnicodeString *code3 = new UnicodeString(codeMapping3Letter);
                uhash_put(regionAliases,(void *)code3, (void *)r,&status);
            }
        }
        ures_close(mapping);
    }

    
    Region *r;
	UnicodeString WORLD_ID_STRING(WORLD_ID);
    r = (Region *) uhash_get(regionIDMap,(void *)&WORLD_ID_STRING);
    if ( r ) {
        r->type = URGN_WORLD;
    }

	UnicodeString UNKNOWN_REGION_ID_STRING(UNKNOWN_REGION_ID);
    r = (Region *) uhash_get(regionIDMap,(void *)&UNKNOWN_REGION_ID_STRING);
    if ( r ) {
        r->type = URGN_UNKNOWN;
    }

    for ( int32_t i = 0 ; i < continents->size() ; i++ ) {
        r = (Region *) uhash_get(regionIDMap,(void *)continents->elementAt(i));
        if ( r ) {
            r->type = URGN_CONTINENT;
        }
    }
    delete continents;

    for ( int32_t i = 0 ; i < groupings->size() ; i++ ) {
        r = (Region *) uhash_get(regionIDMap,(void *)groupings->elementAt(i));
        if ( r ) {
            r->type = URGN_GROUPING;
        }
    }
    delete groupings;

    
    

	UnicodeString OUTLYING_OCEANIA_REGION_ID_STRING(OUTLYING_OCEANIA_REGION_ID);
    r = (Region *) uhash_get(regionIDMap,(void *)&OUTLYING_OCEANIA_REGION_ID_STRING);
    if ( r ) {
        r->type = URGN_SUBCONTINENT;
    }

    
    while ( ures_hasNext(territoryContainment) ) {
        UResourceBundle *mapping = ures_getNextResource(territoryContainment,NULL,&status);
        const char *parent = ures_getKey(mapping);
        UnicodeString parentStr = UnicodeString(parent, -1 , US_INV);
        Region *parentRegion = (Region *) uhash_get(regionIDMap,(void *)&parentStr);

        for ( int j = 0 ; j < ures_getSize(mapping); j++ ) {
            UnicodeString child = ures_getUnicodeStringByIndex(mapping,j,&status);
            Region *childRegion = (Region *) uhash_get(regionIDMap,(void *)&child);
            if ( parentRegion != NULL && childRegion != NULL ) {

                
                if (parentRegion->containedRegions == NULL) {
                    parentRegion->containedRegions = new UVector(uprv_deleteUObject, uhash_compareUnicodeString, status);
                }

                UnicodeString *childStr = new UnicodeString();
                childStr->fastCopyFrom(childRegion->idStr);
                parentRegion->containedRegions->addElement((void *)childStr,status);

                
                
                
                if ( parentRegion->type != URGN_GROUPING) {
                    childRegion->containingRegion = parentRegion;
                }
            }
        }
        ures_close(mapping);
    }

    
    int32_t pos = -1;
    while ( const UHashElement* element = uhash_nextElement(regionIDMap,&pos)) {
        Region *ar = (Region *)element->value.pointer;
        if ( availableRegions[ar->type] == NULL ) {
            availableRegions[ar->type] = new UVector(uprv_deleteUObject, uhash_compareUnicodeString, status);
        }
        UnicodeString *arString = new UnicodeString(ar->idStr);
        availableRegions[ar->type]->addElement((void *)arString,status);
    }

    ures_close(territoryContainment);
    ures_close(worldContainment);
    ures_close(groupingContainment);

    ures_close(codeMappings);
    ures_close(rb2);
    ures_close(territoryAlias);
    ures_close(regionCodes);
    ures_close(rb);

    delete df;

    ucln_i18n_registerCleanup(UCLN_I18N_REGION, region_cleanup);

    regionDataIsLoaded = true;
    umtx_unlock(&gRegionDataLock);

}

void Region::cleanupRegionData() {

    for (int32_t i = 0 ; i < URGN_LIMIT ; i++ ) {
        if ( availableRegions[i] ) {
            delete availableRegions[i];
        }
    }

    if (regionAliases) {
        uhash_close(regionAliases);
    }

    if (numericCodeMap) {
        uhash_close(numericCodeMap);
    }

    if (regionIDMap) {
        uhash_close(regionIDMap);
    }
}

Region::Region ()
        : code(-1),
          type(URGN_UNKNOWN),
          containingRegion(NULL),
          containedRegions(NULL),
          preferredValues(NULL) {
    id[0] = 0;
}

Region::~Region () {
        if (containedRegions) {
            delete containedRegions;
        }
        if (preferredValues) {
            delete preferredValues;
        }
}




UBool
Region::operator==(const Region &that) const {
    return (idStr == that.idStr);
}




UBool
Region::operator!=(const Region &that) const {
        return (idStr != that.idStr);
}







const Region* U_EXPORT2
Region::getInstance(const char *region_code, UErrorCode &status) {

    if ( !region_code ) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    loadRegionData();

    if (regionIDMap == NULL) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    UnicodeString regionCodeString = UnicodeString(region_code, -1, US_INV);
    Region *r = (Region *)uhash_get(regionIDMap,(void *)&regionCodeString);

    if ( !r ) {
        r = (Region *)uhash_get(regionAliases,(void *)&regionCodeString);
    }

    if ( !r ) { 
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if ( r->type == URGN_DEPRECATED && r->preferredValues->size() == 1) {
        StringEnumeration *pv = r->getPreferredValues();
        pv->reset(status);
        const UnicodeString *ustr = pv->snext(status);
        r = (Region *)uhash_get(regionIDMap,(void *)ustr);
        delete pv;
    }

    return r;

}





const Region* U_EXPORT2
Region::getInstance (int32_t code, UErrorCode &status) {

    loadRegionData();

    if (numericCodeMap == NULL) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    Region *r = (Region *)uhash_iget(numericCodeMap,code);

    if ( !r ) { 
        UErrorCode fs = U_ZERO_ERROR;
        UnicodeString pat = UNICODE_STRING_SIMPLE("00#");
        DecimalFormat *df = new DecimalFormat(pat,fs);

        UnicodeString id;
        id.remove();
        df->format(code,id);
        delete df;
        r = (Region *)uhash_get(regionAliases,&id);
    }

    if ( !r ) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if ( r->type == URGN_DEPRECATED && r->preferredValues->size() == 1) {
        StringEnumeration *pv = r->getPreferredValues();
        pv->reset(status);
        const UnicodeString *ustr = pv->snext(status);
        r = (Region *)uhash_get(regionIDMap,(void *)ustr);
        delete pv;
    }

    return r;
}





StringEnumeration* U_EXPORT2
Region::getAvailable(URegionType type) {

    loadRegionData();
    UErrorCode status = U_ZERO_ERROR;
    return new RegionNameEnumeration(availableRegions[type],status);

    return NULL;
}






const Region*
Region::getContainingRegion() const {
    loadRegionData();
    return containingRegion;
}








const Region*
Region::getContainingRegion(URegionType type) const {
    loadRegionData();
    if ( containingRegion == NULL ) {
        return NULL;
    }

    if ( containingRegion->type == type ) {
        return containingRegion;
    } else {
        return containingRegion->getContainingRegion(type);
    }
}









StringEnumeration*
Region::getContainedRegions() const {
    loadRegionData();
    UErrorCode status = U_ZERO_ERROR;
    return new RegionNameEnumeration(containedRegions,status);
}







StringEnumeration*
Region::getContainedRegions( URegionType type ) const {
    loadRegionData();

    UErrorCode status = U_ZERO_ERROR;
    UVector *result = new UVector(NULL, uhash_compareChars, status);

    StringEnumeration *cr = getContainedRegions();

    for ( int32_t i = 0 ; i < cr->count(status) ; i++ ) {
        const char *id = cr->next(NULL,status);
        const Region *r = Region::getInstance(id,status);
        if ( r->getType() == type ) {
            result->addElement((void *)&r->idStr,status);
        } else {
            StringEnumeration *children = r->getContainedRegions(type);
            for ( int32_t j = 0 ; j < children->count(status) ; j++ ) {
                const char *id2 = children->next(NULL,status);
                const Region *r2 = Region::getInstance(id2,status);
                result->addElement((void *)&r2->idStr,status);
            }
            delete children;
        }
    }
    delete cr;
    StringEnumeration* resultEnumeration = new RegionNameEnumeration(result,status);
    delete result;
    return resultEnumeration;
}




UBool
Region::contains(const Region &other) const {
    loadRegionData();

    if (!containedRegions) {
          return FALSE;
    }
    if (containedRegions->contains((void *)&other.idStr)) {
        return TRUE;
    } else {
        for ( int32_t i = 0 ; i < containedRegions->size() ; i++ ) {
            UnicodeString *crStr = (UnicodeString *)containedRegions->elementAt(i);
            Region *cr = (Region *) uhash_get(regionIDMap,(void *)crStr);
            if ( cr && cr->contains(other) ) {
                return TRUE;
            }
        }
    }

    return FALSE;
}






StringEnumeration*
Region::getPreferredValues() const {
    loadRegionData();
    UErrorCode status = U_ZERO_ERROR;
    if ( type == URGN_DEPRECATED ) {
        return new RegionNameEnumeration(preferredValues,status);
    } else {
        return NULL;
    }
}





const char*
Region::getRegionCode() const {
    return id;
}

int32_t
Region::getNumericCode() const {
    return code;
}




URegionType
Region::getType() const {
    return type;
}

RegionNameEnumeration::RegionNameEnumeration(UVector *fNameList, UErrorCode& status) {
    pos=0;
    if (fNameList && U_SUCCESS(status)) {
        fRegionNames = new UVector(uprv_deleteUObject, uhash_compareUnicodeString, fNameList->size(),status);
        for ( int32_t i = 0 ; i < fNameList->size() ; i++ ) {
            UnicodeString* this_region_name = (UnicodeString *)fNameList->elementAt(i);
            UnicodeString* new_region_name = new UnicodeString(*this_region_name);
            fRegionNames->addElement((void *)new_region_name,status);
        }
    }
    else {
        fRegionNames = NULL;
    }
}

const UnicodeString*
RegionNameEnumeration::snext(UErrorCode& status) {
  if (U_FAILURE(status) || (fRegionNames==NULL)) {
    return NULL;
  }
  const UnicodeString* nextStr = (const UnicodeString *)fRegionNames->elementAt(pos);
  if (nextStr!=NULL) {
    pos++;
  }
  return nextStr;
}

void
RegionNameEnumeration::reset(UErrorCode& ) {
    pos=0;
}

int32_t
RegionNameEnumeration::count(UErrorCode& ) const {
    return (fRegionNames==NULL) ? 0 : fRegionNames->size();
}

RegionNameEnumeration::~RegionNameEnumeration() {
    delete fRegionNames;
}

U_NAMESPACE_END

#endif 


