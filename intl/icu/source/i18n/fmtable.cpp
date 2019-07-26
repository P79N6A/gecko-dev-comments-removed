














#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include <math.h>
#include "unicode/fmtable.h"
#include "unicode/ustring.h"
#include "unicode/measure.h"
#include "unicode/curramt.h"
#include "charstr.h"
#include "cmemory.h"
#include "cstring.h"
#include "decNumber.h"
#include "digitlst.h"





U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(Formattable)

#include "fmtableimp.h"












static inline UBool objectEquals(const UObject* a, const UObject* b) {
    
    return *((const Measure*) a) == *((const Measure*) b);
}


static inline UObject* objectClone(const UObject* a) {
    
    return ((const Measure*) a)->clone();
}


static inline UBool instanceOfMeasure(const UObject* a) {
    return dynamic_cast<const Measure*>(a) != NULL;
}








static Formattable* createArrayCopy(const Formattable* array, int32_t count) {
    Formattable *result = new Formattable[count];
    if (result != NULL) {
        for (int32_t i=0; i<count; ++i)
            result[i] = array[i]; 
    }
    return result;
}






static void setError(UErrorCode& ec, UErrorCode err) {
    if (U_SUCCESS(ec)) {
        ec = err;
    }
}





void  Formattable::init() {
    fValue.fInt64 = 0;
    fType = kLong;
    fDecimalStr = NULL;
    fDecimalNum = NULL;
    fBogus.setToBogus(); 
}





Formattable::Formattable() {
    init();
}




Formattable::Formattable(UDate date, ISDATE )
{
    init();
    fType = kDate;
    fValue.fDate = date;
}




Formattable::Formattable(double value)
{
    init();
    fType = kDouble;
    fValue.fDouble = value;
}




Formattable::Formattable(int32_t value)
{
    init();
    fValue.fInt64 = value;
}




Formattable::Formattable(int64_t value)
{
    init();
    fType = kInt64;
    fValue.fInt64 = value;
}




Formattable::Formattable(const StringPiece &number, UErrorCode &status) {
    init();
    setDecimalNumber(number, status);
}





Formattable::Formattable(const UnicodeString& stringToCopy)
{
    init();
    fType = kString;
    fValue.fString = new UnicodeString(stringToCopy);
}





Formattable::Formattable(UnicodeString* stringToAdopt)
{
    init();
    fType = kString;
    fValue.fString = stringToAdopt;
}

Formattable::Formattable(UObject* objectToAdopt)
{
    init();
    fType = kObject;
    fValue.fObject = objectToAdopt;
}



Formattable::Formattable(const Formattable* arrayToCopy, int32_t count)
    :   UObject(), fType(kArray)
{
    init();
    fType = kArray;
    fValue.fArrayAndCount.fArray = createArrayCopy(arrayToCopy, count);
    fValue.fArrayAndCount.fCount = count;
}





Formattable::Formattable(const Formattable &source)
     :  UObject(*this)
{
    init();
    *this = source;
}




Formattable&
Formattable::operator=(const Formattable& source)
{
    if (this != &source)
    {
        
        dispose();

        
        fType = source.fType;
        switch (fType)
        {
        case kArray:
            
            fValue.fArrayAndCount.fCount = source.fValue.fArrayAndCount.fCount;
            fValue.fArrayAndCount.fArray = createArrayCopy(source.fValue.fArrayAndCount.fArray,
                                                           source.fValue.fArrayAndCount.fCount);
            break;
        case kString:
            
            fValue.fString = new UnicodeString(*source.fValue.fString);
            break;
        case kDouble:
            
            fValue.fDouble = source.fValue.fDouble;
            break;
        case kLong:
        case kInt64:
            
            fValue.fInt64 = source.fValue.fInt64;
            break;
        case kDate:
            
            fValue.fDate = source.fValue.fDate;
            break;
        case kObject:
            fValue.fObject = objectClone(source.fValue.fObject);
            break;
        }

        UErrorCode status = U_ZERO_ERROR;
        if (source.fDecimalNum != NULL) {
          fDecimalNum = new DigitList(*source.fDecimalNum); 
        }
        if (source.fDecimalStr != NULL) {
            fDecimalStr = new CharString(*source.fDecimalStr, status);
            if (U_FAILURE(status)) {
                delete fDecimalStr;
                fDecimalStr = NULL;
            }
        }
    }
    return *this;
}



UBool
Formattable::operator==(const Formattable& that) const
{
    int32_t i;

    if (this == &that) return TRUE;

    
    if (fType != that.fType) return FALSE;

    
    UBool equal = TRUE;
    switch (fType) {
    case kDate:
        equal = (fValue.fDate == that.fValue.fDate);
        break;
    case kDouble:
        equal = (fValue.fDouble == that.fValue.fDouble);
        break;
    case kLong:
    case kInt64:
        equal = (fValue.fInt64 == that.fValue.fInt64);
        break;
    case kString:
        equal = (*(fValue.fString) == *(that.fValue.fString));
        break;
    case kArray:
        if (fValue.fArrayAndCount.fCount != that.fValue.fArrayAndCount.fCount) {
            equal = FALSE;
            break;
        }
        
        for (i=0; i<fValue.fArrayAndCount.fCount; ++i) {
            if (fValue.fArrayAndCount.fArray[i] != that.fValue.fArrayAndCount.fArray[i]) {
                equal = FALSE;
                break;
            }
        }
        break;
    case kObject:
        if (fValue.fObject == NULL || that.fValue.fObject == NULL) {
            equal = FALSE;
        } else {
            equal = objectEquals(fValue.fObject, that.fValue.fObject);
        }
        break;
    }

    
    return equal;
}



Formattable::~Formattable()
{
    dispose();
}



void Formattable::dispose()
{
    
    switch (fType) {
    case kString:
        delete fValue.fString;
        break;
    case kArray:
        delete[] fValue.fArrayAndCount.fArray;
        break;
    case kObject:
        delete fValue.fObject;
        break;
    default:
        break;
    }

    fType = kLong;
    fValue.fInt64 = 0;

    delete fDecimalStr;
    fDecimalStr = NULL;
    
    FmtStackData *stackData = (FmtStackData*)fStackData;
    if(fDecimalNum != &(stackData->stackDecimalNum)) {
      delete fDecimalNum;
    } else {
      fDecimalNum->~DigitList(); 
    }
    fDecimalNum = NULL;
}

Formattable *
Formattable::clone() const {
    return new Formattable(*this);
}



Formattable::Type
Formattable::getType() const
{
    return fType;
}

UBool
Formattable::isNumeric() const {
    switch (fType) {
    case kDouble:
    case kLong:
    case kInt64:
        return TRUE;
    default:
        return FALSE;
    }
}


int32_t

Formattable::getLong(UErrorCode& status) const
{
    if (U_FAILURE(status)) {
        return 0;
    }
        
    switch (fType) {
    case Formattable::kLong: 
        return (int32_t)fValue.fInt64;
    case Formattable::kInt64: 
        if (fValue.fInt64 > INT32_MAX) {
            status = U_INVALID_FORMAT_ERROR;
            return INT32_MAX;
        } else if (fValue.fInt64 < INT32_MIN) {
            status = U_INVALID_FORMAT_ERROR;
            return INT32_MIN;
        } else {
            return (int32_t)fValue.fInt64;
        }
    case Formattable::kDouble:
        if (fValue.fDouble > INT32_MAX) {
            status = U_INVALID_FORMAT_ERROR;
            return INT32_MAX;
        } else if (fValue.fDouble < INT32_MIN) {
            status = U_INVALID_FORMAT_ERROR;
            return INT32_MIN;
        } else {
            return (int32_t)fValue.fDouble; 
        }
    case Formattable::kObject:
        if (fValue.fObject == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        
        if (instanceOfMeasure(fValue.fObject)) {
            return ((const Measure*) fValue.fObject)->
                getNumber().getLong(status);
        }
    default: 
        status = U_INVALID_FORMAT_ERROR;
        return 0;
    }
}






static const int64_t U_DOUBLE_MAX_EXACT_INT = 9007199254740992LL;

int64_t
Formattable::getInt64(UErrorCode& status) const
{
    if (U_FAILURE(status)) {
        return 0;
    }
        
    switch (fType) {
    case Formattable::kLong: 
    case Formattable::kInt64: 
        return fValue.fInt64;
    case Formattable::kDouble:
        if (fValue.fDouble > (double)U_INT64_MAX) {
            status = U_INVALID_FORMAT_ERROR;
            return U_INT64_MAX;
        } else if (fValue.fDouble < (double)U_INT64_MIN) {
            status = U_INVALID_FORMAT_ERROR;
            return U_INT64_MIN;
        } else if (fabs(fValue.fDouble) > U_DOUBLE_MAX_EXACT_INT && fDecimalNum != NULL) {
            int64_t val = fDecimalNum->getInt64();
            if (val != 0) {
                return val;
            } else {
                status = U_INVALID_FORMAT_ERROR;
                return fValue.fDouble > 0 ? U_INT64_MAX : U_INT64_MIN;
            }
        } else {
            return (int64_t)fValue.fDouble;
        } 
    case Formattable::kObject:
        if (fValue.fObject == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        if (instanceOfMeasure(fValue.fObject)) {
            return ((const Measure*) fValue.fObject)->
                getNumber().getInt64(status);
        }
    default: 
        status = U_INVALID_FORMAT_ERROR;
        return 0;
    }
}


double
Formattable::getDouble(UErrorCode& status) const
{
    if (U_FAILURE(status)) {
        return 0;
    }
        
    switch (fType) {
    case Formattable::kLong: 
    case Formattable::kInt64: 
        return (double)fValue.fInt64;
    case Formattable::kDouble:
        return fValue.fDouble;
    case Formattable::kObject:
        if (fValue.fObject == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        
        if (instanceOfMeasure(fValue.fObject)) {
            return ((const Measure*) fValue.fObject)->
                getNumber().getDouble(status);
        }
    default: 
        status = U_INVALID_FORMAT_ERROR;
        return 0;
    }
}

const UObject*
Formattable::getObject() const {
    return (fType == kObject) ? fValue.fObject : NULL;
}




void
Formattable::setDouble(double d)
{
    dispose();
    fType = kDouble;
    fValue.fDouble = d;
}




void
Formattable::setLong(int32_t l)
{
    dispose();
    fType = kLong;
    fValue.fInt64 = l;
}




void
Formattable::setInt64(int64_t ll)
{
    dispose();
    fType = kInt64;
    fValue.fInt64 = ll;
}




void
Formattable::setDate(UDate d)
{
    dispose();
    fType = kDate;
    fValue.fDate = d;
}




void
Formattable::setString(const UnicodeString& stringToCopy)
{
    dispose();
    fType = kString;
    fValue.fString = new UnicodeString(stringToCopy);
}




void
Formattable::setArray(const Formattable* array, int32_t count)
{
    dispose();
    fType = kArray;
    fValue.fArrayAndCount.fArray = createArrayCopy(array, count);
    fValue.fArrayAndCount.fCount = count;
}




void
Formattable::adoptString(UnicodeString* stringToAdopt)
{
    dispose();
    fType = kString;
    fValue.fString = stringToAdopt;
}




void
Formattable::adoptArray(Formattable* array, int32_t count)
{
    dispose();
    fType = kArray;
    fValue.fArrayAndCount.fArray = array;
    fValue.fArrayAndCount.fCount = count;
}

void
Formattable::adoptObject(UObject* objectToAdopt) {
    dispose();
    fType = kObject;
    fValue.fObject = objectToAdopt;
}


UnicodeString& 
Formattable::getString(UnicodeString& result, UErrorCode& status) const 
{
    if (fType != kString) {
        setError(status, U_INVALID_FORMAT_ERROR);
        result.setToBogus();
    } else {
        if (fValue.fString == NULL) {
            setError(status, U_MEMORY_ALLOCATION_ERROR);
        } else {
            result = *fValue.fString;
        }
    }
    return result;
}


const UnicodeString& 
Formattable::getString(UErrorCode& status) const 
{
    if (fType != kString) {
        setError(status, U_INVALID_FORMAT_ERROR);
        return *getBogus();
    }
    if (fValue.fString == NULL) {
        setError(status, U_MEMORY_ALLOCATION_ERROR);
        return *getBogus();
    }
    return *fValue.fString;
}


UnicodeString& 
Formattable::getString(UErrorCode& status) 
{
    if (fType != kString) {
        setError(status, U_INVALID_FORMAT_ERROR);
        return *getBogus();
    }
    if (fValue.fString == NULL) {
    	setError(status, U_MEMORY_ALLOCATION_ERROR);
    	return *getBogus();
    }
    return *fValue.fString;
}


const Formattable* 
Formattable::getArray(int32_t& count, UErrorCode& status) const 
{
    if (fType != kArray) {
        setError(status, U_INVALID_FORMAT_ERROR);
        count = 0;
        return NULL;
    }
    count = fValue.fArrayAndCount.fCount; 
    return fValue.fArrayAndCount.fArray;
}




UnicodeString*
Formattable::getBogus() const 
{
    return (UnicodeString*)&fBogus; 
}



StringPiece Formattable::getDecimalNumber(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return "";
    }
    if (fDecimalStr != NULL) {
        return fDecimalStr->toStringPiece();
    }

    if (fDecimalNum == NULL) {
        
        
        
        
        
      fDecimalNum = new DigitList; 
        if (fDecimalNum == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return "";
        }

        switch (fType) {
        case kDouble:
            fDecimalNum->set(this->getDouble());
            break;
        case kLong:
            fDecimalNum->set(this->getLong());
            break;
        case kInt64:
            fDecimalNum->set(this->getInt64());
            break;
        default:
            
            status = U_INVALID_STATE_ERROR;
            return "";
        }
    }

    fDecimalStr = new CharString;
    if (fDecimalStr == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return "";
    }
    fDecimalNum->getDecimal(*fDecimalStr, status);

    return fDecimalStr->toStringPiece();
}


DigitList *
Formattable::getInternalDigitList() {
  FmtStackData *stackData = (FmtStackData*)fStackData;
  if(fDecimalNum != &(stackData->stackDecimalNum)) {
    delete fDecimalNum;
    fDecimalNum = new (&(stackData->stackDecimalNum), kOnStack) DigitList();
  } else {
    fDecimalNum->clear();
  }
  return fDecimalNum;
}


void
Formattable::adoptDigitList(DigitList *dl) {
  if(fDecimalNum==dl) {
    fDecimalNum = NULL; 
  }
  dispose();

  fDecimalNum = dl;

  if(dl==NULL) { 
    return;
  }

    
    

    if (fDecimalNum->fitsIntoLong(FALSE)) {
        fType = kLong;
        fValue.fInt64 = fDecimalNum->getLong();
    } else if (fDecimalNum->fitsIntoInt64(FALSE)) {
        fType = kInt64;
        fValue.fInt64 = fDecimalNum->getInt64();
    } else {
        fType = kDouble;
        fValue.fDouble = fDecimalNum->getDouble();
    }
}



void
Formattable::setDecimalNumber(const StringPiece &numberString, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    dispose();

    
    
    
    
    DigitList *dnum = new DigitList(); 
    if (dnum == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    dnum->set(CharString(numberString, status).toStringPiece(), status);
    if (U_FAILURE(status)) {
        delete dnum;
        return;   
    }
    adoptDigitList(dnum);

    
    
}

#if 0



#ifdef _DEBUG

#include <iostream>
using namespace std;

#include "unicode/datefmt.h"
#include "unistrm.h"

class FormattableStreamer  {
public:
    static void streamOut(ostream& stream, const Formattable& obj);

private:
    FormattableStreamer() {} 
};




void
FormattableStreamer::streamOut(ostream& stream, const Formattable& obj)
{
    static DateFormat *defDateFormat = 0;

    UnicodeString buffer;
    switch(obj.getType()) {
        case Formattable::kDate : 
            
            
            if (defDateFormat == 0) {
                defDateFormat = DateFormat::createInstance();
            }
            defDateFormat->format(obj.getDate(), buffer);
            stream << buffer;
            break;
        case Formattable::kDouble :
            
            stream << obj.getDouble() << 'D';
            break;
        case Formattable::kLong :
            
            stream << obj.getLong() << 'L';
            break;
        case Formattable::kString:
            
            
            stream << '"' << obj.getString(buffer) << '"';
            break;
        case Formattable::kArray:
            int32_t i, count;
            const Formattable* array;
            array = obj.getArray(count);
            stream << '[';
            
            for (i=0; i<count; ++i) {
                FormattableStreamer::streamOut(stream, array[i]);
                stream << ( (i==(count-1)) ? "" : ", " );
            }
            stream << ']';
            break;
        default:
            
            stream << "INVALID_Formattable";
    }
    stream.flush();
}
#endif

#endif

U_NAMESPACE_END

#endif 


