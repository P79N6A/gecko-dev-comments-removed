









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "unicode/resbund.h"
#include "unicode/uniset.h"
#include "unicode/uscript.h"
#include "rbt.h"
#include "cpdtrans.h"
#include "nultrans.h"
#include "transreg.h"
#include "rbt_data.h"
#include "rbt_pars.h"
#include "tridpars.h"
#include "charstr.h"
#include "uassert.h"
#include "locutil.h"








#ifdef DEBUG_MEM
#include <stdio.h>
#endif


static const UChar LOCALE_SEP  = 95; 




static const UChar ANY[] = { 65, 110, 121, 0 }; 


#define NO_VARIANT UnicodeString()






U_NAMESPACE_BEGIN





TransliteratorAlias::TransliteratorAlias(const UnicodeString& theAliasID,
                                         const UnicodeSet* cpdFilter) :
    ID(),
    aliasesOrRules(theAliasID),
    transes(0),
    compoundFilter(cpdFilter),
    direction(UTRANS_FORWARD),
    type(TransliteratorAlias::SIMPLE) {
}

TransliteratorAlias::TransliteratorAlias(const UnicodeString& theID,
                                         const UnicodeString& idBlocks,
                                         UVector* adoptedTransliterators,
                                         const UnicodeSet* cpdFilter) :
    ID(theID),
    aliasesOrRules(idBlocks),
    transes(adoptedTransliterators),
    compoundFilter(cpdFilter),
    direction(UTRANS_FORWARD),
    type(TransliteratorAlias::COMPOUND) {
}

TransliteratorAlias::TransliteratorAlias(const UnicodeString& theID,
                                         const UnicodeString& rules,
                                         UTransDirection dir) :
    ID(theID),
    aliasesOrRules(rules),
    transes(0),
    compoundFilter(0),
    direction(dir),
    type(TransliteratorAlias::RULES) {
}

TransliteratorAlias::~TransliteratorAlias() {
    delete transes;
}


Transliterator* TransliteratorAlias::create(UParseError& pe,
                                            UErrorCode& ec) {
    if (U_FAILURE(ec)) {
        return 0;
    }
    Transliterator *t = NULL;
    switch (type) {
    case SIMPLE:
        t = Transliterator::createInstance(aliasesOrRules, UTRANS_FORWARD, pe, ec);
        if(U_FAILURE(ec)){
            return 0;
        }
        if (compoundFilter != 0)
            t->adoptFilter((UnicodeSet*)compoundFilter->clone());
        break;
    case COMPOUND:
        {
            
            
            
            
            
            int32_t anonymousRBTs = transes->size();
            int32_t transCount = anonymousRBTs * 2 + 1;
            if (!aliasesOrRules.isEmpty() && aliasesOrRules[0] == (UChar)(0xffff))
                --transCount;
            if (aliasesOrRules.length() >= 2 && aliasesOrRules[aliasesOrRules.length() - 1] == (UChar)(0xffff))
                --transCount;
            UnicodeString noIDBlock((UChar)(0xffff));
            noIDBlock += ((UChar)(0xffff));
            int32_t pos = aliasesOrRules.indexOf(noIDBlock);
            while (pos >= 0) {
                --transCount;
                pos = aliasesOrRules.indexOf(noIDBlock, pos + 1);
            }

            UVector transliterators(ec);
            UnicodeString idBlock;
            int32_t blockSeparatorPos = aliasesOrRules.indexOf((UChar)(0xffff));
            while (blockSeparatorPos >= 0) {
                aliasesOrRules.extract(0, blockSeparatorPos, idBlock);
                aliasesOrRules.remove(0, blockSeparatorPos + 1);
                if (!idBlock.isEmpty())
                    transliterators.addElement(Transliterator::createInstance(idBlock, UTRANS_FORWARD, pe, ec), ec);
                if (!transes->isEmpty())
                    transliterators.addElement(transes->orphanElementAt(0), ec);
                blockSeparatorPos = aliasesOrRules.indexOf((UChar)(0xffff));
            }
            if (!aliasesOrRules.isEmpty())
                transliterators.addElement(Transliterator::createInstance(aliasesOrRules, UTRANS_FORWARD, pe, ec), ec);
            while (!transes->isEmpty())
                transliterators.addElement(transes->orphanElementAt(0), ec);

            if (U_SUCCESS(ec)) {
                t = new CompoundTransliterator(ID, transliterators,
                    (compoundFilter ? (UnicodeSet*)(compoundFilter->clone()) : 0),
                    anonymousRBTs, pe, ec);
                if (t == 0) {
                    ec = U_MEMORY_ALLOCATION_ERROR;
                    return 0;
                }
            } else {
                for (int32_t i = 0; i < transliterators.size(); i++)
                    delete (Transliterator*)(transliterators.elementAt(i));
            }
        }
        break;
    case RULES:
        U_ASSERT(FALSE); 
        break;
    }
    return t;
}

UBool TransliteratorAlias::isRuleBased() const {
    return type == RULES;
}

void TransliteratorAlias::parse(TransliteratorParser& parser,
                                UParseError& pe, UErrorCode& ec) const {
    U_ASSERT(type == RULES);
    if (U_FAILURE(ec)) {
        return;
    }

    parser.parse(aliasesOrRules, direction, pe, ec);
}




















class TransliteratorSpec : public UMemory {
 public:
    TransliteratorSpec(const UnicodeString& spec);
    ~TransliteratorSpec();

    const UnicodeString& get() const;
    UBool hasFallback() const;
    const UnicodeString& next();
    void reset();

    UBool isLocale() const;
    ResourceBundle& getBundle() const;

    operator const UnicodeString&() const { return get(); }
    const UnicodeString& getTop() const { return top; }

 private:
    void setupNext();

    UnicodeString top;
    UnicodeString spec;
    UnicodeString nextSpec;
    UnicodeString scriptName;
    UBool isSpecLocale; 
    UBool isNextLocale; 
    ResourceBundle* res;

    TransliteratorSpec(const TransliteratorSpec &other); 
    TransliteratorSpec &operator=(const TransliteratorSpec &other); 
};

TransliteratorSpec::TransliteratorSpec(const UnicodeString& theSpec)
: top(theSpec),
  res(0)
{
    UErrorCode status = U_ZERO_ERROR;
    Locale topLoc("");
    LocaleUtility::initLocaleFromName(theSpec, topLoc);
    if (!topLoc.isBogus()) {
        res = new ResourceBundle(U_ICUDATA_TRANSLIT, topLoc, status);
        
        if (res == 0) {
            return;
        }
        if (U_FAILURE(status) || status == U_USING_DEFAULT_WARNING) {
            delete res;
            res = 0;
        }
    }

    
    status = U_ZERO_ERROR;
    static const int32_t capacity = 10;
    UScriptCode script[capacity]={USCRIPT_INVALID_CODE};
    int32_t num = uscript_getCode(CharString().appendInvariantChars(theSpec, status).data(),
                                  script, capacity, &status);
    if (num > 0 && script[0] != USCRIPT_INVALID_CODE) {
        scriptName = UnicodeString(uscript_getName(script[0]), -1, US_INV);
    }

    
    if (res != 0) {
        
        UnicodeString locStr;
        LocaleUtility::initNameFromLocale(topLoc, locStr);
        if (!locStr.isBogus()) {
            top = locStr;
        }
    } else if (scriptName.length() != 0) {
        
        top = scriptName;
    }

    
    reset();
}

TransliteratorSpec::~TransliteratorSpec() {
    delete res;
}

UBool TransliteratorSpec::hasFallback() const {
    return nextSpec.length() != 0;
}

void TransliteratorSpec::reset() {
    if (spec != top) {
        spec = top;
        isSpecLocale = (res != 0);
        setupNext();
    }
}

void TransliteratorSpec::setupNext() {
    isNextLocale = FALSE;
    if (isSpecLocale) {
        nextSpec = spec;
        int32_t i = nextSpec.lastIndexOf(LOCALE_SEP);
        
        
        if (i > 0) {
            nextSpec.truncate(i);
            isNextLocale = TRUE;
        } else {
            nextSpec = scriptName; 
        }
    } else {
        
        nextSpec.truncate(0);
    }
}





const UnicodeString& TransliteratorSpec::next() {
    spec = nextSpec;
    isSpecLocale = isNextLocale;
    setupNext();
    return spec;
}

const UnicodeString& TransliteratorSpec::get() const {
    return spec;
}

UBool TransliteratorSpec::isLocale() const {
    return isSpecLocale;
}

ResourceBundle& TransliteratorSpec::getBundle() const {
    return *res;
}



#ifdef DEBUG_MEM


static UVector* DEBUG_entries = NULL;

static void DEBUG_setup() {
    if (DEBUG_entries == NULL) {
        UErrorCode ec = U_ZERO_ERROR;
        DEBUG_entries = new UVector(ec);
    }
}



static int DEBUG_findEntry(TransliteratorEntry* e) {
    for (int i=0; i<DEBUG_entries->size(); ++i) {
        if (e == (TransliteratorEntry*) DEBUG_entries->elementAt(i)) {
            return i;
        }
    }
    return -1;
}


static void DEBUG_newEntry(TransliteratorEntry* e) {
    DEBUG_setup();
    if (DEBUG_findEntry(e) >= 0) {
        
        printf("ERROR DEBUG_newEntry duplicate new pointer %08X\n", e);
        return;
    }
    UErrorCode ec = U_ZERO_ERROR;
    DEBUG_entries->addElement(e, ec);
}


static void DEBUG_delEntry(TransliteratorEntry* e) {
    DEBUG_setup();
    int i = DEBUG_findEntry(e);
    if (i < 0) {
        printf("ERROR DEBUG_delEntry possible double deletion %08X\n", e);
        return;
    }
    DEBUG_entries->removeElementAt(i);
}


static void DEBUG_useEntry(TransliteratorEntry* e) {
    if (e == NULL) return;
    DEBUG_setup();
    int i = DEBUG_findEntry(e);
    if (i < 0) {
        printf("ERROR DEBUG_useEntry possible dangling pointer %08X\n", e);
    }
}

#else

#define DEBUG_newEntry(x)
#define DEBUG_delEntry(x)
#define DEBUG_useEntry(x)
#endif















class TransliteratorEntry : public UMemory {
public:
    enum Type {
        RULES_FORWARD,
        RULES_REVERSE,
        LOCALE_RULES,
        PROTOTYPE,
        RBT_DATA,
        COMPOUND_RBT,
        ALIAS,
        FACTORY,
        NONE 
    } entryType;
    
    
    UnicodeString stringArg; 
    int32_t intArg; 
    UnicodeSet* compoundFilter; 
    union {
        Transliterator* prototype; 
        TransliterationRuleData* data; 
        UVector* dataVector;    
        struct {
            Transliterator::Factory function;
            Transliterator::Token   context;
        } factory; 
    } u;
    TransliteratorEntry();
    ~TransliteratorEntry();
    void adoptPrototype(Transliterator* adopted);
    void setFactory(Transliterator::Factory factory,
                    Transliterator::Token context);

private:

    TransliteratorEntry(const TransliteratorEntry &other); 
    TransliteratorEntry &operator=(const TransliteratorEntry &other); 
};

TransliteratorEntry::TransliteratorEntry() {
    u.prototype = 0;
    compoundFilter = NULL;
    entryType = NONE;
    DEBUG_newEntry(this);
}

TransliteratorEntry::~TransliteratorEntry() {
    DEBUG_delEntry(this);
    if (entryType == PROTOTYPE) {
        delete u.prototype;
    } else if (entryType == RBT_DATA) {
        
        
        
        
        delete u.data;
    } else if (entryType == COMPOUND_RBT) {
        while (u.dataVector != NULL && !u.dataVector->isEmpty())
            delete (TransliterationRuleData*)u.dataVector->orphanElementAt(0);
        delete u.dataVector;
    }
    delete compoundFilter;
}

void TransliteratorEntry::adoptPrototype(Transliterator* adopted) {
    if (entryType == PROTOTYPE) {
        delete u.prototype;
    }
    entryType = PROTOTYPE;
    u.prototype = adopted;
}

void TransliteratorEntry::setFactory(Transliterator::Factory factory,
                       Transliterator::Token context) {
    if (entryType == PROTOTYPE) {
        delete u.prototype;
    }
    entryType = FACTORY;
    u.factory.function = factory;
    u.factory.context = context;
}


U_CDECL_BEGIN
static void U_CALLCONV
deleteEntry(void* obj) {
    delete (TransliteratorEntry*) obj;
}
U_CDECL_END





TransliteratorRegistry::TransliteratorRegistry(UErrorCode& status) :
    registry(TRUE, status),
    specDAG(TRUE, status),
    availableIDs(status)
{
    registry.setValueDeleter(deleteEntry);
    availableIDs.setDeleter(uprv_deleteUObject);
    availableIDs.setComparer(uhash_compareCaselessUnicodeString);
    specDAG.setValueDeleter(uhash_deleteHashtable);
}

TransliteratorRegistry::~TransliteratorRegistry() {
    
}

Transliterator* TransliteratorRegistry::get(const UnicodeString& ID,
                                            TransliteratorAlias*& aliasReturn,
                                            UErrorCode& status) {
    U_ASSERT(aliasReturn == NULL);
    TransliteratorEntry *entry = find(ID);
    return (entry == 0) ? 0
        : instantiateEntry(ID, entry, aliasReturn, status);
}

Transliterator* TransliteratorRegistry::reget(const UnicodeString& ID,
                                              TransliteratorParser& parser,
                                              TransliteratorAlias*& aliasReturn,
                                              UErrorCode& status) {
    U_ASSERT(aliasReturn == NULL);
    TransliteratorEntry *entry = find(ID);

    if (entry == 0) {
        
        
        
        return 0;
    }

    
    
    
    
    

    
    
    
    
    

    if (entry->entryType == TransliteratorEntry::RULES_FORWARD ||
        entry->entryType == TransliteratorEntry::RULES_REVERSE ||
        entry->entryType == TransliteratorEntry::LOCALE_RULES) {
        
        if (parser.idBlockVector.isEmpty() && parser.dataVector.isEmpty()) {
            entry->u.data = 0;
            entry->entryType = TransliteratorEntry::ALIAS;
            entry->stringArg = UNICODE_STRING_SIMPLE("Any-NULL");
        }
        else if (parser.idBlockVector.isEmpty() && parser.dataVector.size() == 1) {
            entry->u.data = (TransliterationRuleData*)parser.dataVector.orphanElementAt(0);
            entry->entryType = TransliteratorEntry::RBT_DATA;
        }
        else if (parser.idBlockVector.size() == 1 && parser.dataVector.isEmpty()) {
            entry->stringArg = *(UnicodeString*)(parser.idBlockVector.elementAt(0));
            entry->compoundFilter = parser.orphanCompoundFilter();
            entry->entryType = TransliteratorEntry::ALIAS;
        }
        else {
            entry->entryType = TransliteratorEntry::COMPOUND_RBT;
            entry->compoundFilter = parser.orphanCompoundFilter();
            entry->u.dataVector = new UVector(status);
            entry->stringArg.remove();

            int32_t limit = parser.idBlockVector.size();
            if (parser.dataVector.size() > limit)
                limit = parser.dataVector.size();

            for (int32_t i = 0; i < limit; i++) {
                if (i < parser.idBlockVector.size()) {
                    UnicodeString* idBlock = (UnicodeString*)parser.idBlockVector.elementAt(i);
                    if (!idBlock->isEmpty())
                        entry->stringArg += *idBlock;
                }
                if (!parser.dataVector.isEmpty()) {
                    TransliterationRuleData* data = (TransliterationRuleData*)parser.dataVector.orphanElementAt(0);
                    entry->u.dataVector->addElement(data, status);
                    entry->stringArg += (UChar)0xffff;  
                }
            }
        }
    }

    Transliterator *t =
        instantiateEntry(ID, entry, aliasReturn, status);
    return t;
}

void TransliteratorRegistry::put(Transliterator* adoptedProto,
                                 UBool visible,
                                 UErrorCode& ec)
{
    TransliteratorEntry *entry = new TransliteratorEntry();
    if (entry == NULL) {
        ec = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    entry->adoptPrototype(adoptedProto);
    registerEntry(adoptedProto->getID(), entry, visible);
}

void TransliteratorRegistry::put(const UnicodeString& ID,
                                 Transliterator::Factory factory,
                                 Transliterator::Token context,
                                 UBool visible,
                                 UErrorCode& ec) {
    TransliteratorEntry *entry = new TransliteratorEntry();
    if (entry == NULL) {
        ec = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    entry->setFactory(factory, context);
    registerEntry(ID, entry, visible);
}

void TransliteratorRegistry::put(const UnicodeString& ID,
                                 const UnicodeString& resourceName,
                                 UTransDirection dir,
                                 UBool readonlyResourceAlias,
                                 UBool visible,
                                 UErrorCode& ec) {
    TransliteratorEntry *entry = new TransliteratorEntry();
    if (entry == NULL) {
        ec = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    entry->entryType = (dir == UTRANS_FORWARD) ? TransliteratorEntry::RULES_FORWARD
        : TransliteratorEntry::RULES_REVERSE;
    if (readonlyResourceAlias) {
        entry->stringArg.setTo(TRUE, resourceName.getBuffer(), -1);
    }
    else {
        entry->stringArg = resourceName;
    }
    registerEntry(ID, entry, visible);
}

void TransliteratorRegistry::put(const UnicodeString& ID,
                                 const UnicodeString& alias,
                                 UBool readonlyAliasAlias,
                                 UBool visible,
                                 UErrorCode& ) {
    TransliteratorEntry *entry = new TransliteratorEntry();
    
    if (entry != NULL) {
        entry->entryType = TransliteratorEntry::ALIAS;
        if (readonlyAliasAlias) {
            entry->stringArg.setTo(TRUE, alias.getBuffer(), -1);
        }
        else {
            entry->stringArg = alias;
        }
        registerEntry(ID, entry, visible);
    }
}

void TransliteratorRegistry::remove(const UnicodeString& ID) {
    UnicodeString source, target, variant;
    UBool sawSource;
    TransliteratorIDParser::IDtoSTV(ID, source, target, variant, sawSource);
    
    UnicodeString id;
    TransliteratorIDParser::STVtoID(source, target, variant, id);
    registry.remove(id);
    removeSTV(source, target, variant);
    availableIDs.removeElement((void*) &id);
}











int32_t TransliteratorRegistry::countAvailableIDs(void) const {
    return availableIDs.size();
}







const UnicodeString& TransliteratorRegistry::getAvailableID(int32_t index) const {
    if (index < 0 || index >= availableIDs.size()) {
        index = 0;
    }
    return *(const UnicodeString*) availableIDs[index];
}

StringEnumeration* TransliteratorRegistry::getAvailableIDs() const {
    return new Enumeration(*this);
}

int32_t TransliteratorRegistry::countAvailableSources(void) const {
    return specDAG.count();
}

UnicodeString& TransliteratorRegistry::getAvailableSource(int32_t index,
                                                          UnicodeString& result) const {
    int32_t pos = -1;
    const UHashElement *e = 0;
    while (index-- >= 0) {
        e = specDAG.nextElement(pos);
        if (e == 0) {
            break;
        }
    }
    if (e == 0) {
        result.truncate(0);
    } else {
        result = *(UnicodeString*) e->key.pointer;
    }
    return result;
}

int32_t TransliteratorRegistry::countAvailableTargets(const UnicodeString& source) const {
    Hashtable *targets = (Hashtable*) specDAG.get(source);
    return (targets == 0) ? 0 : targets->count();
}

UnicodeString& TransliteratorRegistry::getAvailableTarget(int32_t index,
                                                          const UnicodeString& source,
                                                          UnicodeString& result) const {
    Hashtable *targets = (Hashtable*) specDAG.get(source);
    if (targets == 0) {
        result.truncate(0); 
        return result;
    }
    int32_t pos = -1;
    const UHashElement *e = 0;
    while (index-- >= 0) {
        e = targets->nextElement(pos);
        if (e == 0) {
            break;
        }
    }
    if (e == 0) {
        result.truncate(0); 
    } else {
        result = *(UnicodeString*) e->key.pointer;
    }
    return result;
}

int32_t TransliteratorRegistry::countAvailableVariants(const UnicodeString& source,
                                                       const UnicodeString& target) const {
    Hashtable *targets = (Hashtable*) specDAG.get(source);
    if (targets == 0) {
        return 0;
    }
    UVector *variants = (UVector*) targets->get(target);
    
    return (variants == 0) ? 0 : variants->size();
}

UnicodeString& TransliteratorRegistry::getAvailableVariant(int32_t index,
                                                           const UnicodeString& source,
                                                           const UnicodeString& target,
                                                           UnicodeString& result) const {
    Hashtable *targets = (Hashtable*) specDAG.get(source);
    if (targets == 0) {
        result.truncate(0); 
        return result;
    }
    UVector *variants = (UVector*) targets->get(target);
    if (variants == 0) {
        result.truncate(0); 
        return result;
    }
    UnicodeString *v = (UnicodeString*) variants->elementAt(index);
    if (v == 0) {
        result.truncate(0); 
    } else {
        result = *v;
    }
    return result;
}





TransliteratorRegistry::Enumeration::Enumeration(const TransliteratorRegistry& _reg) :
    index(0), reg(_reg) {
}

TransliteratorRegistry::Enumeration::~Enumeration() {
}

int32_t TransliteratorRegistry::Enumeration::count(UErrorCode& ) const {
    return reg.availableIDs.size();
}

const UnicodeString* TransliteratorRegistry::Enumeration::snext(UErrorCode& status) {
    
    
    
    
    
    
    
    
    
    if (U_FAILURE(status)) {
        return NULL;
    }
    int32_t n = reg.availableIDs.size();
    if (index > n) {
        status = U_ENUM_OUT_OF_SYNC_ERROR;
    }
    
    if (index < n) {
        
        unistr = *(const UnicodeString*)reg.availableIDs[index++];
        return &unistr;
    } else {
        return NULL;
    }
}

void TransliteratorRegistry::Enumeration::reset(UErrorCode& ) {
    index = 0;
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(TransliteratorRegistry::Enumeration)








void TransliteratorRegistry::registerEntry(const UnicodeString& source,
                                           const UnicodeString& target,
                                           const UnicodeString& variant,
                                           TransliteratorEntry* adopted,
                                           UBool visible) {
    UnicodeString ID;
    UnicodeString s(source);
    if (s.length() == 0) {
        s.setTo(TRUE, ANY, 3);
    }
    TransliteratorIDParser::STVtoID(source, target, variant, ID);
    registerEntry(ID, s, target, variant, adopted, visible);
}




void TransliteratorRegistry::registerEntry(const UnicodeString& ID,
                                           TransliteratorEntry* adopted,
                                           UBool visible) {
    UnicodeString source, target, variant;
    UBool sawSource;
    TransliteratorIDParser::IDtoSTV(ID, source, target, variant, sawSource);
    
    UnicodeString id;
    TransliteratorIDParser::STVtoID(source, target, variant, id);
    registerEntry(id, source, target, variant, adopted, visible);
}





void TransliteratorRegistry::registerEntry(const UnicodeString& ID,
                                           const UnicodeString& source,
                                           const UnicodeString& target,
                                           const UnicodeString& variant,
                                           TransliteratorEntry* adopted,
                                           UBool visible) {
    UErrorCode status = U_ZERO_ERROR;
    registry.put(ID, adopted, status);
    if (visible) {
        registerSTV(source, target, variant);
        if (!availableIDs.contains((void*) &ID)) {
            UnicodeString *newID = (UnicodeString *)ID.clone();
            
            if (newID != NULL) {
	            
	            newID->getTerminatedBuffer();
	            availableIDs.addElement(newID, status);
            }
        }
    } else {
        removeSTV(source, target, variant);
        availableIDs.removeElement((void*) &ID);
    }
}







void TransliteratorRegistry::registerSTV(const UnicodeString& source,
                                         const UnicodeString& target,
                                         const UnicodeString& variant) {
    
    
    UErrorCode status = U_ZERO_ERROR;
    Hashtable *targets = (Hashtable*) specDAG.get(source);
    if (targets == 0) {
        targets = new Hashtable(TRUE, status);
        if (U_FAILURE(status) || targets == 0) {
            return;
        }
        targets->setValueDeleter(uprv_deleteUObject);
        specDAG.put(source, targets, status);
    }
    UVector *variants = (UVector*) targets->get(target);
    if (variants == 0) {
        variants = new UVector(uprv_deleteUObject,
                               uhash_compareCaselessUnicodeString, status);
        if (variants == 0) {
            return;
        }
        targets->put(target, variants, status);
    }
    
    
    
    if (!variants->contains((void*) &variant)) {
    	UnicodeString *tempus; 
        if (variant.length() > 0) {
        	tempus = new UnicodeString(variant);
        	if (tempus != NULL) {
        		variants->addElement(tempus, status);
        	}
        } else {
        	tempus = new UnicodeString();  
        	if (tempus != NULL) {
        		variants->insertElementAt(tempus, 0, status);
        	}
        }
    }
}




void TransliteratorRegistry::removeSTV(const UnicodeString& source,
                                       const UnicodeString& target,
                                       const UnicodeString& variant) {
    
    

    Hashtable *targets = (Hashtable*) specDAG.get(source);
    if (targets == 0) {
        return; 
    }
    UVector *variants = (UVector*) targets->get(target);
    if (variants == 0) {
        return; 
    }
    variants->removeElement((void*) &variant);
    if (variants->size() == 0) {
        targets->remove(target); 
        if (targets->count() == 0) {
            specDAG.remove(source); 
        }
    }
}







TransliteratorEntry* TransliteratorRegistry::findInDynamicStore(const TransliteratorSpec& src,
                                                  const TransliteratorSpec& trg,
                                                  const UnicodeString& variant) const {
    UnicodeString ID;
    TransliteratorIDParser::STVtoID(src, trg, variant, ID);
    TransliteratorEntry *e = (TransliteratorEntry*) registry.get(ID);
    DEBUG_useEntry(e);
    return e;
}












TransliteratorEntry* TransliteratorRegistry::findInStaticStore(const TransliteratorSpec& src,
                                                 const TransliteratorSpec& trg,
                                                 const UnicodeString& variant) {
    TransliteratorEntry* entry = 0;
    if (src.isLocale()) {
        entry = findInBundle(src, trg, variant, UTRANS_FORWARD);
    } else if (trg.isLocale()) {
        entry = findInBundle(trg, src, variant, UTRANS_REVERSE);
    }

    
    
    if (entry != 0) {
        registerEntry(src.getTop(), trg.getTop(), variant, entry, FALSE);
    }

    return entry;
}


static const UChar TRANSLITERATE_TO[] = {84,114,97,110,115,108,105,116,101,114,97,116,101,84,111,0}; 

static const UChar TRANSLITERATE_FROM[] = {84,114,97,110,115,108,105,116,101,114,97,116,101,70,114,111,109,0}; 

static const UChar TRANSLITERATE[] = {84,114,97,110,115,108,105,116,101,114,97,116,101,0}; 











TransliteratorEntry* TransliteratorRegistry::findInBundle(const TransliteratorSpec& specToOpen,
                                            const TransliteratorSpec& specToFind,
                                            const UnicodeString& variant,
                                            UTransDirection direction)
{
    UnicodeString utag;
    UnicodeString resStr;
    int32_t pass;

    for (pass=0; pass<2; ++pass) {
        utag.truncate(0);
        
        
        
        
        if (pass == 0) {
            utag.append(direction == UTRANS_FORWARD ?
                        TRANSLITERATE_TO : TRANSLITERATE_FROM, -1);
        } else {
            utag.append(TRANSLITERATE, -1);
        }
        UnicodeString s(specToFind.get());
        utag.append(s.toUpper(""));
        UErrorCode status = U_ZERO_ERROR;
        ResourceBundle subres(specToOpen.getBundle().get(
            CharString().appendInvariantChars(utag, status).data(), status));
        if (U_FAILURE(status) || status == U_USING_DEFAULT_WARNING) {
            continue;
        }

        s.truncate(0);
        if (specToOpen.get() != LocaleUtility::initNameFromLocale(subres.getLocale(), s)) {
            continue;
        }

        if (variant.length() != 0) {
            status = U_ZERO_ERROR;
            resStr = subres.getStringEx(
                CharString().appendInvariantChars(variant, status).data(), status);
            if (U_SUCCESS(status)) {
                
                break;
            }
        } else {
            
            status = U_ZERO_ERROR;
            resStr = subres.getStringEx(1, status);
            if (U_SUCCESS(status)) {
                
                break;
            }
        }
    }

    if (pass==2) {
        
        return NULL;
    }

    
    
    TransliteratorEntry *entry = new TransliteratorEntry();
    if (entry != 0) {
        
        
        
        
        
        
        int32_t dir = (pass == 0) ? UTRANS_FORWARD : direction;
        entry->entryType = TransliteratorEntry::LOCALE_RULES;
        entry->stringArg = resStr;
        entry->intArg = dir;
    }

    return entry;
}




TransliteratorEntry* TransliteratorRegistry::find(const UnicodeString& ID) {
    UnicodeString source, target, variant;
    UBool sawSource;
    TransliteratorIDParser::IDtoSTV(ID, source, target, variant, sawSource);
    return find(source, target, variant);
}






















TransliteratorEntry* TransliteratorRegistry::find(UnicodeString& source,
                                    UnicodeString& target,
                                    UnicodeString& variant) {
    
    TransliteratorSpec src(source);
    TransliteratorSpec trg(target);
    TransliteratorEntry* entry;

    
    
    
    UnicodeString ID;
    TransliteratorIDParser::STVtoID(source, target, variant, ID);
    entry = (TransliteratorEntry*) registry.get(ID);
    if (entry != 0) {
        
        
        return entry;
    }

    if (variant.length() != 0) {
        
        
        entry = findInDynamicStore(src, trg, variant);
        if (entry != 0) {
            return entry;
        }
        
        
        entry = findInStaticStore(src, trg, variant);
        if (entry != 0) {
            return entry;
        }
    }

    for (;;) {
        src.reset();
        for (;;) {
            
            entry = findInDynamicStore(src, trg, NO_VARIANT);
            if (entry != 0) {
                return entry;
            }
            
            
            entry = findInStaticStore(src, trg, NO_VARIANT);
            if (entry != 0) {
                return entry;
            }
            if (!src.hasFallback()) {
                break;
            }
            src.next();
        }
        if (!trg.hasFallback()) {
            break;
        }
        trg.next();
    }

    return 0;
}













Transliterator* TransliteratorRegistry::instantiateEntry(const UnicodeString& ID,
                                                         TransliteratorEntry *entry,
                                                         TransliteratorAlias* &aliasReturn,
                                                         UErrorCode& status) {
    Transliterator *t = 0;
    U_ASSERT(aliasReturn == 0);

    switch (entry->entryType) {
    case TransliteratorEntry::RBT_DATA:
        t = new RuleBasedTransliterator(ID, entry->u.data);
        if (t == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        return t;
    case TransliteratorEntry::PROTOTYPE:
        t = entry->u.prototype->clone();
        if (t == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        return t;
    case TransliteratorEntry::ALIAS:
        aliasReturn = new TransliteratorAlias(entry->stringArg, entry->compoundFilter);
        if (aliasReturn == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        return 0;
    case TransliteratorEntry::FACTORY:
        t = entry->u.factory.function(ID, entry->u.factory.context);
        if (t == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        return t;
    case TransliteratorEntry::COMPOUND_RBT:
        {
            UVector* rbts = new UVector(entry->u.dataVector->size(), status);
            
            if (rbts == NULL) {
            	status = U_MEMORY_ALLOCATION_ERROR;
            	return NULL;
            }
            int32_t passNumber = 1;
            for (int32_t i = 0; U_SUCCESS(status) && i < entry->u.dataVector->size(); i++) {
                
                Transliterator* t = new RuleBasedTransliterator(UnicodeString(CompoundTransliterator::PASS_STRING) + UnicodeString(passNumber++),
                    (TransliterationRuleData*)(entry->u.dataVector->elementAt(i)), FALSE);
                if (t == 0)
                    status = U_MEMORY_ALLOCATION_ERROR;
                else
                    rbts->addElement(t, status);
            }
            if (U_FAILURE(status)) {
                delete rbts;
                return 0;
            }
            aliasReturn = new TransliteratorAlias(ID, entry->stringArg, rbts, entry->compoundFilter);
        }
        if (aliasReturn == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        return 0;
    case TransliteratorEntry::LOCALE_RULES:
        aliasReturn = new TransliteratorAlias(ID, entry->stringArg,
                                              (UTransDirection) entry->intArg);
        if (aliasReturn == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        return 0;
    case TransliteratorEntry::RULES_FORWARD:
    case TransliteratorEntry::RULES_REVERSE:
        
        
        
        {
            TransliteratorParser parser(status);
            
            
            
            
            
            
            
            UnicodeString rules = entry->stringArg;
            
            
            
                
                
                
                
                
                
            
            
                
                
                
                
                
                
                aliasReturn = new TransliteratorAlias(ID, rules,
                    ((entry->entryType == TransliteratorEntry::RULES_REVERSE) ?
                     UTRANS_REVERSE : UTRANS_FORWARD));
                if (aliasReturn == 0) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                }
            
        }
        return 0;
    default:
        U_ASSERT(FALSE); 
        return 0;
    }
}
U_NAMESPACE_END

#endif 


