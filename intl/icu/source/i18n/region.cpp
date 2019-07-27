



















#include "unicode/region.h"
#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "unicode/ures.h"
#include "unicode/decimfmt.h"
#include "ucln_in.h"
#include "cstring.h"
#include "mutex.h"
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

static UInitOnce gRegionDataInitOnce = U_INITONCE_INITIALIZER;
static UVector* availableRegions[URGN_LIMIT];

static UHashtable *regionAliases = NULL;
static UHashtable *regionIDMap = NULL;
static UHashtable *numericCodeMap = NULL;

static const UChar UNKNOWN_REGION_ID [] = { 0x5A, 0x5A, 0 };  
static const UChar OUTLYING_OCEANIA_REGION_ID [] = { 0x51, 0x4F, 0 };  
static const UChar WORLD_ID [] = { 0x30, 0x30, 0x31, 0 };  

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RegionNameEnumeration)









void Region::loadRegionData(UErrorCode &status) {
    
    LocalUHashtablePointer newRegionIDMap(uhash_open(uhash_hashUnicodeString, uhash_compareUnicodeString, NULL, &status));
    LocalUHashtablePointer newNumericCodeMap(uhash_open(uhash_hashLong,uhash_compareLong,NULL,&status));
    LocalUHashtablePointer newRegionAliases(uhash_open(uhash_hashUnicodeString,uhash_compareUnicodeString,NULL,&status));

    LocalPointer<DecimalFormat> df(new DecimalFormat(status), status);

    LocalPointer<UVector> continents(new UVector(uprv_deleteUObject, uhash_compareUnicodeString, status), status);
    LocalPointer<UVector> groupings(new UVector(uprv_deleteUObject, uhash_compareUnicodeString, status), status);

    LocalUResourceBundlePointer metadata(ures_openDirect(NULL,"metadata",&status));
    LocalUResourceBundlePointer regionCodes(ures_getByKey(metadata.getAlias(),"regionCodes",NULL,&status));
    LocalUResourceBundlePointer metadataAlias(ures_getByKey(metadata.getAlias(),"alias",NULL,&status));
    LocalUResourceBundlePointer territoryAlias(ures_getByKey(metadataAlias.getAlias(),"territory",NULL,&status));

    LocalUResourceBundlePointer supplementalData(ures_openDirect(NULL,"supplementalData",&status));
    LocalUResourceBundlePointer codeMappings(ures_getByKey(supplementalData.getAlias(),"codeMappings",NULL,&status));

    LocalUResourceBundlePointer territoryContainment(ures_getByKey(supplementalData.getAlias(),"territoryContainment",NULL,&status));
    LocalUResourceBundlePointer worldContainment(ures_getByKey(territoryContainment.getAlias(),"001",NULL,&status));
    LocalUResourceBundlePointer groupingContainment(ures_getByKey(territoryContainment.getAlias(),"grouping",NULL,&status));

    if (U_FAILURE(status)) {
        return;
    }

    
    df->setParseIntegerOnly(TRUE);
    uhash_setValueDeleter(newRegionIDMap.getAlias(), deleteRegion);  
    uhash_setKeyDeleter(newRegionAliases.getAlias(), uprv_deleteUObject); 

    while ( ures_hasNext(worldContainment.getAlias()) ) {
        UnicodeString *continentName = new UnicodeString(ures_getNextUnicodeString(worldContainment.getAlias(),NULL,&status));
        continents->addElement(continentName,status);
    }

    while ( ures_hasNext(groupingContainment.getAlias()) ) {
        UnicodeString *groupingName = new UnicodeString(ures_getNextUnicodeString(groupingContainment.getAlias(),NULL,&status));
        groupings->addElement(groupingName,status);
    }

    while ( ures_hasNext(regionCodes.getAlias()) ) {
        UnicodeString regionID = ures_getNextUnicodeString(regionCodes.getAlias(), NULL, &status);
        LocalPointer<Region> r(new Region(), status);
        if ( U_FAILURE(status) ) {
           return;
        }
        r->idStr = regionID;
        r->idStr.extract(0,r->idStr.length(),r->id,sizeof(r->id),US_INV);
        r->type = URGN_TERRITORY; 

        Formattable result;
        UErrorCode ps = U_ZERO_ERROR;
        df->parse(r->idStr,result,ps);
        if ( U_SUCCESS(ps) ) {
            r->code = result.getLong(); 
            uhash_iput(newNumericCodeMap.getAlias(),r->code,(void *)(r.getAlias()),&status);
            r->type = URGN_SUBCONTINENT;
        } else {
            r->code = -1;
        }
        void* idStrAlias = (void*)&(r->idStr); 
        uhash_put(newRegionIDMap.getAlias(),idStrAlias,(void *)(r.orphan()),&status); 
    }


    
    while ( ures_hasNext(territoryAlias.getAlias()) ) {
        LocalUResourceBundlePointer res(ures_getNextResource(territoryAlias.getAlias(),NULL,&status));
        const char *aliasFrom = ures_getKey(res.getAlias());
        LocalPointer<UnicodeString> aliasFromStr(new UnicodeString(aliasFrom, -1, US_INV), status);
        UnicodeString aliasTo = ures_getUnicodeStringByKey(res.getAlias(),"replacement",&status);
        res.adoptInstead(NULL);

        const Region *aliasToRegion = (Region *) uhash_get(newRegionIDMap.getAlias(),&aliasTo);
        Region *aliasFromRegion = (Region *)uhash_get(newRegionIDMap.getAlias(),aliasFromStr.getAlias());

        if ( aliasToRegion != NULL && aliasFromRegion == NULL ) { 
            uhash_put(newRegionAliases.getAlias(),(void *)aliasFromStr.orphan(), (void *)aliasToRegion,&status);
        } else {
            if ( aliasFromRegion == NULL ) { 
                LocalPointer<Region> newRgn(new Region, status); 
                if ( U_SUCCESS(status) ) {
                    aliasFromRegion = newRgn.orphan();
                } else {
                    return; 
                }
                aliasFromRegion->idStr.setTo(*aliasFromStr);
                aliasFromRegion->idStr.extract(0,aliasFromRegion->idStr.length(),aliasFromRegion->id,sizeof(aliasFromRegion->id),US_INV);
                uhash_put(newRegionIDMap.getAlias(),(void *)&(aliasFromRegion->idStr),(void *)aliasFromRegion,&status);
                Formattable result;
                UErrorCode ps = U_ZERO_ERROR;
                df->parse(aliasFromRegion->idStr,result,ps);
                if ( U_SUCCESS(ps) ) {
                    aliasFromRegion->code = result.getLong(); 
                    uhash_iput(newNumericCodeMap.getAlias(),aliasFromRegion->code,(void *)aliasFromRegion,&status);
                } else {
                    aliasFromRegion->code = -1;
                }
                aliasFromRegion->type = URGN_DEPRECATED;
            } else {
                aliasFromRegion->type = URGN_DEPRECATED;
            }

            {
                LocalPointer<UVector> newPreferredValues(new UVector(uprv_deleteUObject, uhash_compareUnicodeString, status), status);
                aliasFromRegion->preferredValues = newPreferredValues.orphan();
            }
            if( U_FAILURE(status)) {
                return;
            }
            UnicodeString currentRegion;
            
            for (int32_t i = 0 ; i < aliasTo.length() ; i++ ) {
                if ( aliasTo.charAt(i) != 0x0020 ) {
                    currentRegion.append(aliasTo.charAt(i));
                }
                if ( aliasTo.charAt(i) == 0x0020 || i+1 == aliasTo.length() ) {
                    Region *target = (Region *)uhash_get(newRegionIDMap.getAlias(),(void *)&currentRegion);
                    if (target) {
                        LocalPointer<UnicodeString> preferredValue(new UnicodeString(target->idStr), status);
                        aliasFromRegion->preferredValues->addElement((void *)preferredValue.orphan(),status);  
                    }
                    currentRegion.remove();
                }
            }
        }
    }

    
    while ( ures_hasNext(codeMappings.getAlias()) ) {
        UResourceBundle *mapping = ures_getNextResource(codeMappings.getAlias(),NULL,&status);
        if ( ures_getType(mapping) == URES_ARRAY && ures_getSize(mapping) == 3) {
            UnicodeString codeMappingID = ures_getUnicodeStringByIndex(mapping,0,&status);
            UnicodeString codeMappingNumber = ures_getUnicodeStringByIndex(mapping,1,&status);
            UnicodeString codeMapping3Letter = ures_getUnicodeStringByIndex(mapping,2,&status);

            Region *r = (Region *)uhash_get(newRegionIDMap.getAlias(),(void *)&codeMappingID);
            if ( r ) {
                Formattable result;
                UErrorCode ps = U_ZERO_ERROR;
                df->parse(codeMappingNumber,result,ps);
                if ( U_SUCCESS(ps) ) {
                    r->code = result.getLong(); 
                    uhash_iput(newNumericCodeMap.getAlias(),r->code,(void *)r,&status);
                }
                LocalPointer<UnicodeString> code3(new UnicodeString(codeMapping3Letter), status);
                uhash_put(newRegionAliases.getAlias(),(void *)code3.orphan(), (void *)r,&status);
            }
        }
        ures_close(mapping);
    }

    
    Region *r;
    UnicodeString WORLD_ID_STRING(WORLD_ID);
    r = (Region *) uhash_get(newRegionIDMap.getAlias(),(void *)&WORLD_ID_STRING);
    if ( r ) {
        r->type = URGN_WORLD;
    }

    UnicodeString UNKNOWN_REGION_ID_STRING(UNKNOWN_REGION_ID);
    r = (Region *) uhash_get(newRegionIDMap.getAlias(),(void *)&UNKNOWN_REGION_ID_STRING);
    if ( r ) {
        r->type = URGN_UNKNOWN;
    }

    for ( int32_t i = 0 ; i < continents->size() ; i++ ) {
        r = (Region *) uhash_get(newRegionIDMap.getAlias(),(void *)continents->elementAt(i));
        if ( r ) {
            r->type = URGN_CONTINENT;
        }
    }

    for ( int32_t i = 0 ; i < groupings->size() ; i++ ) {
        r = (Region *) uhash_get(newRegionIDMap.getAlias(),(void *)groupings->elementAt(i));
        if ( r ) {
            r->type = URGN_GROUPING;
        }
    }

    
    

    UnicodeString OUTLYING_OCEANIA_REGION_ID_STRING(OUTLYING_OCEANIA_REGION_ID);
    r = (Region *) uhash_get(newRegionIDMap.getAlias(),(void *)&OUTLYING_OCEANIA_REGION_ID_STRING);
    if ( r ) {
        r->type = URGN_SUBCONTINENT;
    }

    
    while ( ures_hasNext(territoryContainment.getAlias()) ) {
        LocalUResourceBundlePointer mapping(ures_getNextResource(territoryContainment.getAlias(),NULL,&status));
        if( U_FAILURE(status) ) {
            return;  
        }
        const char *parent = ures_getKey(mapping.getAlias());
        if (uprv_strcmp(parent, "containedGroupings") == 0 || uprv_strcmp(parent, "deprecated") == 0) {
            continue; 
            
        }
        UnicodeString parentStr = UnicodeString(parent, -1 , US_INV);
        Region *parentRegion = (Region *) uhash_get(newRegionIDMap.getAlias(),(void *)&parentStr);

        for ( int j = 0 ; j < ures_getSize(mapping.getAlias()); j++ ) {
            UnicodeString child = ures_getUnicodeStringByIndex(mapping.getAlias(),j,&status);
            Region *childRegion = (Region *) uhash_get(newRegionIDMap.getAlias(),(void *)&child);
            if ( parentRegion != NULL && childRegion != NULL ) {

                
                if (parentRegion->containedRegions == NULL) {
                    parentRegion->containedRegions = new UVector(uprv_deleteUObject, uhash_compareUnicodeString, status);
                }

                LocalPointer<UnicodeString> childStr(new UnicodeString(), status);
                if( U_FAILURE(status) ) {
                    return;  
                }
                childStr->fastCopyFrom(childRegion->idStr);
                parentRegion->containedRegions->addElement((void *)childStr.orphan(),status);

                
                
                
                if ( parentRegion->type != URGN_GROUPING) {
                    childRegion->containingRegion = parentRegion;
                }
            }
        }
    }

    
    int32_t pos = UHASH_FIRST;
    while ( const UHashElement* element = uhash_nextElement(newRegionIDMap.getAlias(),&pos)) {
        Region *ar = (Region *)element->value.pointer;
        if ( availableRegions[ar->type] == NULL ) {
            LocalPointer<UVector> newAr(new UVector(uprv_deleteUObject, uhash_compareUnicodeString, status), status);
            availableRegions[ar->type] = newAr.orphan();
        }
        LocalPointer<UnicodeString> arString(new UnicodeString(ar->idStr), status);
        if( U_FAILURE(status) ) {
            return;  
        }
        availableRegions[ar->type]->addElement((void *)arString.orphan(),status);
    }
    
    ucln_i18n_registerCleanup(UCLN_I18N_REGION, region_cleanup);
    
    numericCodeMap = newNumericCodeMap.orphan();
    regionIDMap = newRegionIDMap.orphan();
    regionAliases = newRegionAliases.orphan();
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
    regionAliases = numericCodeMap = regionIDMap = NULL;

    gRegionDataInitOnce.reset();
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

    umtx_initOnce(gRegionDataInitOnce, &loadRegionData, status);
    if (U_FAILURE(status)) {
        return NULL;
    }

    if ( !region_code ) {
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
        StringEnumeration *pv = r->getPreferredValues(status);
        pv->reset(status);
        const UnicodeString *ustr = pv->snext(status);
        r = (Region *)uhash_get(regionIDMap,(void *)ustr);
        delete pv;
    }

    return r;

}





const Region* U_EXPORT2
Region::getInstance (int32_t code, UErrorCode &status) {

    umtx_initOnce(gRegionDataInitOnce, &loadRegionData, status);
    if (U_FAILURE(status)) {
        return NULL;
    }

    Region *r = (Region *)uhash_iget(numericCodeMap,code);

    if ( !r ) { 
        UErrorCode fs = U_ZERO_ERROR;
        UnicodeString pat = UNICODE_STRING_SIMPLE("00#");
        LocalPointer<DecimalFormat> df(new DecimalFormat(pat,fs), status);
        if( U_FAILURE(status) ) {
            return NULL;
        }
        UnicodeString id;
        id.remove();
        FieldPosition posIter;
        df->format(code,id, posIter, status);
        r = (Region *)uhash_get(regionAliases,&id);
    }

    if( U_FAILURE(status) ) {
        return NULL;
    }

    if ( !r ) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if ( r->type == URGN_DEPRECATED && r->preferredValues->size() == 1) {
        StringEnumeration *pv = r->getPreferredValues(status);
        pv->reset(status);
        const UnicodeString *ustr = pv->snext(status);
        r = (Region *)uhash_get(regionIDMap,(void *)ustr);
        delete pv;
    }

    return r;
}





StringEnumeration* U_EXPORT2
Region::getAvailable(URegionType type, UErrorCode &status) {
    umtx_initOnce(gRegionDataInitOnce, &loadRegionData, status); 
    if (U_FAILURE(status)) {
        return NULL;
    }
    return new RegionNameEnumeration(availableRegions[type],status);
}






const Region*
Region::getContainingRegion() const {
    UErrorCode status = U_ZERO_ERROR;
    umtx_initOnce(gRegionDataInitOnce, &loadRegionData, status);
    return containingRegion;
}








const Region*
Region::getContainingRegion(URegionType type) const {
    UErrorCode status = U_ZERO_ERROR;
    umtx_initOnce(gRegionDataInitOnce, &loadRegionData, status);
    if ( containingRegion == NULL ) {
        return NULL;
    }

    return ( containingRegion->type == type )? containingRegion: containingRegion->getContainingRegion(type);
}









StringEnumeration*
Region::getContainedRegions(UErrorCode &status) const {
    umtx_initOnce(gRegionDataInitOnce, &loadRegionData, status); 
    if (U_FAILURE(status)) {
        return NULL;
    }
    return new RegionNameEnumeration(containedRegions,status);
}







StringEnumeration*
Region::getContainedRegions( URegionType type, UErrorCode &status ) const {
    umtx_initOnce(gRegionDataInitOnce, &loadRegionData, status); 
    if (U_FAILURE(status)) {
        return NULL;
    }

    UVector *result = new UVector(NULL, uhash_compareChars, status);

    StringEnumeration *cr = getContainedRegions(status);

    for ( int32_t i = 0 ; i < cr->count(status) ; i++ ) {
        const char *id = cr->next(NULL,status);
        const Region *r = Region::getInstance(id,status);
        if ( r->getType() == type ) {
            result->addElement((void *)&r->idStr,status);
        } else {
            StringEnumeration *children = r->getContainedRegions(type, status);
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
    UErrorCode status = U_ZERO_ERROR;
    umtx_initOnce(gRegionDataInitOnce, &loadRegionData, status);

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
Region::getPreferredValues(UErrorCode &status) const {
    umtx_initOnce(gRegionDataInitOnce, &loadRegionData, status); 
    if (U_FAILURE(status) ||  type != URGN_DEPRECATED) {
        return NULL;
    }
    return new RegionNameEnumeration(preferredValues,status);
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


