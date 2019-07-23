










































#include "pkix_pl_date.h"























PKIX_Error *
pkix_pl_Date_GetPRTime(
        PKIX_PL_Date *date,
        PRTime *pPRTime,
        void *plContext)
{
        SECStatus rv = SECFailure;

        PKIX_ENTER(DATE, "PKIX_PL_Date_GetPRTime");
        PKIX_NULLCHECK_TWO(date, pPRTime);

        PKIX_DATE_DEBUG("\t\tCalling DER_DecodeTimeChoice).\n");
        rv = DER_DecodeTimeChoice(pPRTime, &date->nssTime);

        if (rv == SECFailure) {
                PKIX_ERROR(PKIX_DERDECODETIMECHOICEFAILED);
        }

cleanup:

        PKIX_RETURN(DATE);
}





















PKIX_Error *
pkix_pl_Date_CreateFromPRTime(
        PRTime prtime,
        PKIX_PL_Date **pDate,
        void *plContext)
{
        SECItem nssTime = { 0, NULL, 0 };
        SECStatus rv = SECFailure;
        PKIX_PL_Date *date = NULL;

        PKIX_ENTER(DATE, "PKIX_PL_Date_CreateFromPRTime");
        PKIX_NULLCHECK_ONE(pDate);

        PKIX_DATE_DEBUG("\t\tCalling DER_EncodeTimeChoice).\n");
        rv = DER_EncodeTimeChoice(NULL, &nssTime, prtime);

        if (rv == SECFailure) {
                PKIX_ERROR(PKIX_DERENCODETIMECHOICEFAILED);
        }

        
        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_DATE_TYPE,
                    sizeof (PKIX_PL_Date),
                    (PKIX_PL_Object **)&date,
                    plContext),
                    PKIX_COULDNOTCREATEOBJECT);

        
        date->nssTime = nssTime;

        *pDate = date;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PR_Free(nssTime.data);
        }

        PKIX_RETURN(DATE);
}






















PKIX_Error *
pkix_pl_Date_ToString_Helper(
        SECItem *nssTime,
        PKIX_PL_String **pString,
        void *plContext)
{
        char *asciiDate = NULL;

        PKIX_ENTER(DATE, "pkix_pl_Date_ToString_Helper");
        PKIX_NULLCHECK_TWO(nssTime, pString);

        switch (nssTime->type) {
        case siUTCTime:
                PKIX_PL_NSSCALLRV
                        (DATE, asciiDate, DER_UTCDayToAscii, (nssTime));
                if (!asciiDate){
                        PKIX_ERROR(PKIX_DERUTCTIMETOASCIIFAILED);
                }
                break;
        case siGeneralizedTime:
                




                PKIX_PL_NSSCALLRV
                        (DATE, asciiDate, DER_GeneralizedDayToAscii, (nssTime));
                if (!asciiDate){
                        PKIX_ERROR(PKIX_DERGENERALIZEDDAYTOASCIIFAILED);
                }
                break;
        default:
                PKIX_ERROR(PKIX_UNRECOGNIZEDTIMETYPE);
        }

        PKIX_CHECK(PKIX_PL_String_Create
                    (PKIX_ESCASCII, asciiDate, 0, pString, plContext),
                    PKIX_STRINGCREATEFAILED);

cleanup:

        PR_Free(asciiDate);

        PKIX_RETURN(DATE);
}






static PKIX_Error *
pkix_pl_Date_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_Date *date = NULL;

        PKIX_ENTER(DATE, "pkix_pl_Date_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_DATE_TYPE, plContext),
                    PKIX_OBJECTNOTDATE);

        date = (PKIX_PL_Date *)object;

        PR_Free(date->nssTime.data);

cleanup:

        PKIX_RETURN(DATE);
}





static PKIX_Error *
pkix_pl_Date_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_String *dateString = NULL;
        PKIX_PL_Date *date = NULL;
        SECItem *nssTime = NULL;
        char *asciiDate = NULL;

        PKIX_ENTER(DATE, "pkix_pl_Date_toString");
        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType(object, PKIX_DATE_TYPE, plContext),
                    PKIX_OBJECTNOTDATE);

        date = (PKIX_PL_Date *)object;

        nssTime = &date->nssTime;

        PKIX_CHECK(pkix_pl_Date_ToString_Helper
                    (nssTime, &dateString, plContext),
                    PKIX_DATETOSTRINGHELPERFAILED);

        *pString = dateString;

cleanup:

        PKIX_FREE(asciiDate);

        PKIX_RETURN(DATE);
}





static PKIX_Error *
pkix_pl_Date_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PL_Date *date = NULL;
        SECItem *nssTime = NULL;
        PKIX_UInt32 dateHash;

        PKIX_ENTER(DATE, "pkix_pl_Date_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_DATE_TYPE, plContext),
                    PKIX_OBJECTNOTDATE);

        date = (PKIX_PL_Date *)object;

        nssTime = &date->nssTime;

        PKIX_CHECK
                (pkix_hash
                ((const unsigned char *)nssTime->data,
                nssTime->len,
                &dateHash,
                plContext),
                PKIX_HASHFAILED);

        *pHashcode = dateHash;

cleanup:

        PKIX_RETURN(DATE);

}





static PKIX_Error *
pkix_pl_Date_Comparator(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Int32 *pResult,
        void *plContext)
{
        SECItem *firstTime = NULL;
        SECItem *secondTime = NULL;
        SECComparison cmpResult;

        PKIX_ENTER(DATE, "pkix_pl_Date_Comparator");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        PKIX_CHECK(pkix_CheckTypes
                    (firstObject, secondObject, PKIX_DATE_TYPE, plContext),
                    PKIX_ARGUMENTSNOTDATES);

        firstTime = &((PKIX_PL_Date *)firstObject)->nssTime;
        secondTime = &((PKIX_PL_Date *)secondObject)->nssTime;

        PKIX_DATE_DEBUG("\t\tCalling SECITEM_CompareItem).\n");
        cmpResult = SECITEM_CompareItem(firstTime, secondTime);

        *pResult = cmpResult;

cleanup:

        PKIX_RETURN(DATE);
}





static PKIX_Error *
pkix_pl_Date_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext)
{

        

        SECItem *firstTime = NULL;
        SECItem *secondTime = NULL;
        PKIX_UInt32 secondType;
        SECComparison cmpResult;

        PKIX_ENTER(DATE, "pkix_pl_Date_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        
        PKIX_CHECK(pkix_CheckType(firstObject, PKIX_DATE_TYPE, plContext),
                PKIX_FIRSTOBJECTNOTDATE);

        



        if (firstObject == secondObject){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        



        *pResult = PKIX_FALSE;
        PKIX_CHECK(PKIX_PL_Object_GetType
                    (secondObject, &secondType, plContext),
                    PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);
        if (secondType != PKIX_DATE_TYPE) goto cleanup;

        firstTime = &((PKIX_PL_Date *)firstObject)->nssTime;
        secondTime = &((PKIX_PL_Date *)secondObject)->nssTime;

        PKIX_DATE_DEBUG("\t\tCalling SECITEM_CompareItem).\n");
        cmpResult = SECITEM_CompareItem(firstTime, secondTime);

        *pResult = (cmpResult == SECEqual);

cleanup:

        PKIX_RETURN(DATE);
}












PKIX_Error *
pkix_pl_Date_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(DATE, "pkix_pl_Date_RegisterSelf");

        entry.description = "Date";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_Date);
        entry.destructor = pkix_pl_Date_Destroy;
        entry.equalsFunction = pkix_pl_Date_Equals;
        entry.hashcodeFunction = pkix_pl_Date_Hashcode;
        entry.toStringFunction = pkix_pl_Date_ToString;
        entry.comparator = pkix_pl_Date_Comparator;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_DATE_TYPE] = entry;

        PKIX_RETURN(DATE);
}






PKIX_Error *
PKIX_PL_Date_Create_UTCTime(
        PKIX_PL_String *stringRep,
        PKIX_PL_Date **pDate,
        void *plContext)
{
        PKIX_PL_Date *date = NULL;
        char *asciiString = NULL;
        PKIX_UInt32 escAsciiLength;
        SECItem nssTime;
        SECStatus rv;
        PRTime time;

        PKIX_ENTER(DATE, "PKIX_PL_Date_Create_UTCTime");
        PKIX_NULLCHECK_ONE(pDate);

        if (stringRep == NULL){
                PKIX_DATE_DEBUG("\t\tCalling PR_Now).\n");
                time = PR_Now();
        } else {
                
                PKIX_CHECK(PKIX_PL_String_GetEncoded
                            (stringRep,
                            PKIX_ESCASCII,
                            (void **)&asciiString,
                            &escAsciiLength,
                            plContext),
                            PKIX_STRINGGETENCODEDFAILED);

                PKIX_DATE_DEBUG("\t\tCalling DER_AsciiToTime).\n");
                
                rv = DER_AsciiToTime(&time, asciiString);
                if (rv != SECSuccess){
                        PKIX_ERROR(PKIX_DERASCIITOTIMEFAILED);
                }
        }

        PKIX_DATE_DEBUG("\t\tCalling DER_TimeToUTCTime).\n");
        rv = DER_TimeToUTCTime(&nssTime, time);
        if (rv != SECSuccess){
                PKIX_ERROR(PKIX_DERTIMETOUTCTIMEFAILED);
        }

        
        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_DATE_TYPE,
                    sizeof (PKIX_PL_Date),
                    (PKIX_PL_Object **)&date,
                    plContext),
                    PKIX_COULDNOTCREATEOBJECT);

        
        date->nssTime = nssTime;

        *pDate = date;

cleanup:

        PKIX_FREE(asciiString);

        if (PKIX_ERROR_RECEIVED){
                PR_Free(nssTime.data);
        }

        PKIX_RETURN(DATE);
}





PKIX_Error *
PKIX_PL_Date_Create_CurrentOffBySeconds(
        PKIX_Int32 secondsOffset,
        PKIX_PL_Date **pDate,
        void *plContext)
{
        PKIX_PL_Date *date = NULL;
        SECItem nssTime;
        SECStatus rv;
        PRTime time;

        PKIX_ENTER(DATE, "PKIX_PL_Date_Create_CurrentOffBySeconds");
        PKIX_NULLCHECK_ONE(pDate);

        PKIX_DATE_DEBUG("\t\tCalling PR_Now).\n");
        time = PR_Now();

        time += (secondsOffset * PR_USEC_PER_SEC);

        PKIX_DATE_DEBUG("\t\tCalling DER_TimeToUTCTime).\n");
        rv = DER_TimeToUTCTime(&nssTime, time);
        if (rv != SECSuccess){
                PKIX_ERROR(PKIX_DERTIMETOUTCTIMEFAILED);
        }

        
        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_DATE_TYPE,
                    sizeof (PKIX_PL_Date),
                    (PKIX_PL_Object **)&date,
                    plContext),
                    PKIX_COULDNOTCREATEOBJECT);

        
        date->nssTime = nssTime;

        *pDate = date;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PR_Free(nssTime.data);
        }

        PKIX_RETURN(DATE);
}

PKIX_Error *
PKIX_PL_Date_CreateFromPRTime(
        PRTime prtime,
        PKIX_PL_Date **pDate,
        void *plContext)
{
    PKIX_ENTER(DATE, "PKIX_PL_Date_CreateFromPRTime");
    PKIX_CHECK(
        pkix_pl_Date_CreateFromPRTime(prtime, pDate, plContext),
        PKIX_DATECREATEFROMPRTIMEFAILED);

cleanup:
    PKIX_RETURN(DATE);
}
