











#ifndef __SCRIPTSET_H__
#define __SCRIPTSET_H__

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/uscript.h"

#include "uelement.h"

U_NAMESPACE_BEGIN











class U_I18N_API ScriptSet: public UMemory {
  public:
    ScriptSet();
    ScriptSet(const ScriptSet &other);
    ~ScriptSet();

    UBool operator == (const ScriptSet &other) const;
    ScriptSet & operator = (const ScriptSet &other);

    UBool      test(UScriptCode script, UErrorCode &status) const;
    ScriptSet &Union(const ScriptSet &other);
    ScriptSet &set(UScriptCode script, UErrorCode &status);
    ScriptSet &reset(UScriptCode script, UErrorCode &status);
    ScriptSet &intersect(const ScriptSet &other);
    ScriptSet &intersect(UScriptCode script, UErrorCode &status);
    UBool      intersects(const ScriptSet &other) const;  
    UBool      contains(const ScriptSet &other) const;    

    ScriptSet &setAll();
    ScriptSet &resetAll();
    int32_t countMembers() const;
    int32_t hashCode() const;
    int32_t nextSetBit(int32_t script) const;

    UnicodeString &displayScripts(UnicodeString &dest) const; 
    ScriptSet & parseScripts(const UnicodeString &scriptsString, UErrorCode &status);  

  private:
    uint32_t  bits[6];
};

U_NAMESPACE_END

U_CAPI UBool U_EXPORT2
uhash_compareScriptSet(const UElement key1, const UElement key2);

U_CAPI int32_t U_EXPORT2
uhash_hashScriptSet(const UElement key);

U_CAPI void U_EXPORT2
uhash_deleteScriptSet(void *obj);

#endif 
