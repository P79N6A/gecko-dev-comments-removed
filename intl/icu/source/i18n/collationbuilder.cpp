












#ifdef DEBUG_COLLATION_BUILDER
#include <stdio.h>
#endif

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/caniter.h"
#include "unicode/normalizer2.h"
#include "unicode/tblcoll.h"
#include "unicode/parseerr.h"
#include "unicode/uchar.h"
#include "unicode/ucol.h"
#include "unicode/unistr.h"
#include "unicode/usetiter.h"
#include "unicode/utf16.h"
#include "unicode/uversion.h"
#include "cmemory.h"
#include "collation.h"
#include "collationbuilder.h"
#include "collationdata.h"
#include "collationdatabuilder.h"
#include "collationfastlatin.h"
#include "collationroot.h"
#include "collationrootelements.h"
#include "collationruleparser.h"
#include "collationsettings.h"
#include "collationtailoring.h"
#include "collationweights.h"
#include "normalizer2impl.h"
#include "uassert.h"
#include "ucol_imp.h"
#include "utf16collationiterator.h"

U_NAMESPACE_BEGIN

namespace {

class BundleImporter : public CollationRuleParser::Importer {
public:
    BundleImporter() {}
    virtual ~BundleImporter();
    virtual void getRules(
            const char *localeID, const char *collationType,
            UnicodeString &rules,
            const char *&errorReason, UErrorCode &errorCode);
};

BundleImporter::~BundleImporter() {}

void
BundleImporter::getRules(
        const char *localeID, const char *collationType,
        UnicodeString &rules,
        const char *& , UErrorCode &errorCode) {
    CollationLoader::loadRules(localeID, collationType, rules, errorCode);
}

}  









RuleBasedCollator::RuleBasedCollator()
        : data(NULL),
          settings(NULL),
          tailoring(NULL),
          cacheEntry(NULL),
          validLocale(""),
          explicitlySetAttributes(0),
          actualLocaleIsSameAsValid(FALSE) {
}

RuleBasedCollator::RuleBasedCollator(const UnicodeString &rules, UErrorCode &errorCode)
        : data(NULL),
          settings(NULL),
          tailoring(NULL),
          cacheEntry(NULL),
          validLocale(""),
          explicitlySetAttributes(0),
          actualLocaleIsSameAsValid(FALSE) {
    internalBuildTailoring(rules, UCOL_DEFAULT, UCOL_DEFAULT, NULL, NULL, errorCode);
}

RuleBasedCollator::RuleBasedCollator(const UnicodeString &rules, ECollationStrength strength,
                                     UErrorCode &errorCode)
        : data(NULL),
          settings(NULL),
          tailoring(NULL),
          cacheEntry(NULL),
          validLocale(""),
          explicitlySetAttributes(0),
          actualLocaleIsSameAsValid(FALSE) {
    internalBuildTailoring(rules, strength, UCOL_DEFAULT, NULL, NULL, errorCode);
}

RuleBasedCollator::RuleBasedCollator(const UnicodeString &rules,
                                     UColAttributeValue decompositionMode,
                                     UErrorCode &errorCode)
        : data(NULL),
          settings(NULL),
          tailoring(NULL),
          cacheEntry(NULL),
          validLocale(""),
          explicitlySetAttributes(0),
          actualLocaleIsSameAsValid(FALSE) {
    internalBuildTailoring(rules, UCOL_DEFAULT, decompositionMode, NULL, NULL, errorCode);
}

RuleBasedCollator::RuleBasedCollator(const UnicodeString &rules,
                                     ECollationStrength strength,
                                     UColAttributeValue decompositionMode,
                                     UErrorCode &errorCode)
        : data(NULL),
          settings(NULL),
          tailoring(NULL),
          cacheEntry(NULL),
          validLocale(""),
          explicitlySetAttributes(0),
          actualLocaleIsSameAsValid(FALSE) {
    internalBuildTailoring(rules, strength, decompositionMode, NULL, NULL, errorCode);
}

RuleBasedCollator::RuleBasedCollator(const UnicodeString &rules,
                                     UParseError &parseError, UnicodeString &reason,
                                     UErrorCode &errorCode)
        : data(NULL),
          settings(NULL),
          tailoring(NULL),
          cacheEntry(NULL),
          validLocale(""),
          explicitlySetAttributes(0),
          actualLocaleIsSameAsValid(FALSE) {
    internalBuildTailoring(rules, UCOL_DEFAULT, UCOL_DEFAULT, &parseError, &reason, errorCode);
}

void
RuleBasedCollator::internalBuildTailoring(const UnicodeString &rules,
                                          int32_t strength,
                                          UColAttributeValue decompositionMode,
                                          UParseError *outParseError, UnicodeString *outReason,
                                          UErrorCode &errorCode) {
    const CollationTailoring *base = CollationRoot::getRoot(errorCode);
    if(U_FAILURE(errorCode)) { return; }
    if(outReason != NULL) { outReason->remove(); }
    CollationBuilder builder(base, errorCode);
    UVersionInfo noVersion = { 0, 0, 0, 0 };
    BundleImporter importer;
    LocalPointer<CollationTailoring> t(builder.parseAndBuild(rules, noVersion,
                                                             &importer,
                                                             outParseError, errorCode));
    if(U_FAILURE(errorCode)) {
        const char *reason = builder.getErrorReason();
        if(reason != NULL && outReason != NULL) {
            *outReason = UnicodeString(reason, -1, US_INV);
        }
        return;
    }
    t->actualLocale.setToBogus();
    adoptTailoring(t.orphan(), errorCode);
    
    
    if(strength != UCOL_DEFAULT) {
        setAttribute(UCOL_STRENGTH, (UColAttributeValue)strength, errorCode);
    }
    if(decompositionMode != UCOL_DEFAULT) {
        setAttribute(UCOL_NORMALIZATION_MODE, decompositionMode, errorCode);
    }
}





#ifndef _MSC_VER
const int32_t CollationBuilder::HAS_BEFORE2;
const int32_t CollationBuilder::HAS_BEFORE3;
#endif

CollationBuilder::CollationBuilder(const CollationTailoring *b, UErrorCode &errorCode)
        : nfd(*Normalizer2::getNFDInstance(errorCode)),
          fcd(*Normalizer2Factory::getFCDInstance(errorCode)),
          nfcImpl(*Normalizer2Factory::getNFCImpl(errorCode)),
          base(b),
          baseData(b->data),
          rootElements(b->data->rootElements, b->data->rootElementsLength),
          variableTop(0),
          dataBuilder(new CollationDataBuilder(errorCode)), fastLatinEnabled(TRUE),
          errorReason(NULL),
          cesLength(0),
          rootPrimaryIndexes(errorCode), nodes(errorCode) {
    nfcImpl.ensureCanonIterData(errorCode);
    if(U_FAILURE(errorCode)) {
        errorReason = "CollationBuilder fields initialization failed";
        return;
    }
    if(dataBuilder == NULL) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    dataBuilder->initForTailoring(baseData, errorCode);
    if(U_FAILURE(errorCode)) {
        errorReason = "CollationBuilder initialization failed";
    }
}

CollationBuilder::~CollationBuilder() {
    delete dataBuilder;
}

CollationTailoring *
CollationBuilder::parseAndBuild(const UnicodeString &ruleString,
                                const UVersionInfo rulesVersion,
                                CollationRuleParser::Importer *importer,
                                UParseError *outParseError,
                                UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return NULL; }
    if(baseData->rootElements == NULL) {
        errorCode = U_MISSING_RESOURCE_ERROR;
        errorReason = "missing root elements data, tailoring not supported";
        return NULL;
    }
    LocalPointer<CollationTailoring> tailoring(new CollationTailoring(base->settings));
    if(tailoring.isNull() || tailoring->isBogus()) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    CollationRuleParser parser(baseData, errorCode);
    if(U_FAILURE(errorCode)) { return NULL; }
    
    
    
    
    
    variableTop = base->settings->variableTop;
    parser.setSink(this);
    parser.setImporter(importer);
    CollationSettings &ownedSettings = *SharedObject::copyOnWrite(tailoring->settings);
    parser.parse(ruleString, ownedSettings, outParseError, errorCode);
    errorReason = parser.getErrorReason();
    if(U_FAILURE(errorCode)) { return NULL; }
    if(dataBuilder->hasMappings()) {
        makeTailoredCEs(errorCode);
        closeOverComposites(errorCode);
        finalizeCEs(errorCode);
        
        optimizeSet.add(0, 0x7f);
        optimizeSet.add(0xc0, 0xff);
        
        
        optimizeSet.remove(Hangul::HANGUL_BASE, Hangul::HANGUL_END);
        dataBuilder->optimize(optimizeSet, errorCode);
        tailoring->ensureOwnedData(errorCode);
        if(U_FAILURE(errorCode)) { return NULL; }
        if(fastLatinEnabled) { dataBuilder->enableFastLatin(); }
        dataBuilder->build(*tailoring->ownedData, errorCode);
        tailoring->builder = dataBuilder;
        dataBuilder = NULL;
    } else {
        tailoring->data = baseData;
    }
    if(U_FAILURE(errorCode)) { return NULL; }
    ownedSettings.fastLatinOptions = CollationFastLatin::getOptions(
        tailoring->data, ownedSettings,
        ownedSettings.fastLatinPrimaries, UPRV_LENGTHOF(ownedSettings.fastLatinPrimaries));
    tailoring->rules = ruleString;
    tailoring->rules.getTerminatedBuffer();  
    tailoring->setVersion(base->version, rulesVersion);
    return tailoring.orphan();
}

void
CollationBuilder::addReset(int32_t strength, const UnicodeString &str,
                           const char *&parserErrorReason, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return; }
    U_ASSERT(!str.isEmpty());
    if(str.charAt(0) == CollationRuleParser::POS_LEAD) {
        ces[0] = getSpecialResetPosition(str, parserErrorReason, errorCode);
        cesLength = 1;
        if(U_FAILURE(errorCode)) { return; }
        U_ASSERT((ces[0] & Collation::CASE_AND_QUATERNARY_MASK) == 0);
    } else {
        
        UnicodeString nfdString = nfd.normalize(str, errorCode);
        if(U_FAILURE(errorCode)) {
            parserErrorReason = "normalizing the reset position";
            return;
        }
        cesLength = dataBuilder->getCEs(nfdString, ces, 0);
        if(cesLength > Collation::MAX_EXPANSION_LENGTH) {
            errorCode = U_ILLEGAL_ARGUMENT_ERROR;
            parserErrorReason = "reset position maps to too many collation elements (more than 31)";
            return;
        }
    }
    if(strength == UCOL_IDENTICAL) { return; }  

    
    U_ASSERT(UCOL_PRIMARY <= strength && strength <= UCOL_TERTIARY);
    int32_t index = findOrInsertNodeForCEs(strength, parserErrorReason, errorCode);
    if(U_FAILURE(errorCode)) { return; }

    int64_t node = nodes.elementAti(index);
    
    
    while(strengthFromNode(node) > strength) {
        index = previousIndexFromNode(node);
        node = nodes.elementAti(index);
    }

    
    if(strengthFromNode(node) == strength && isTailoredNode(node)) {
        
        index = previousIndexFromNode(node);
    } else if(strength == UCOL_PRIMARY) {
        
        uint32_t p = weight32FromNode(node);
        if(p == 0) {
            errorCode = U_UNSUPPORTED_ERROR;
            parserErrorReason = "reset primary-before ignorable not possible";
            return;
        }
        if(p <= rootElements.getFirstPrimary()) {
            
            errorCode = U_UNSUPPORTED_ERROR;
            parserErrorReason = "reset primary-before first non-ignorable not supported";
            return;
        }
        if(p == Collation::FIRST_TRAILING_PRIMARY) {
            
            errorCode = U_UNSUPPORTED_ERROR;
            parserErrorReason = "reset primary-before [first trailing] not supported";
            return;
        }
        p = rootElements.getPrimaryBefore(p, baseData->isCompressiblePrimary(p));
        index = findOrInsertNodeForPrimary(p, errorCode);
        
        
        for(;;) {
            node = nodes.elementAti(index);
            int32_t nextIndex = nextIndexFromNode(node);
            if(nextIndex == 0) { break; }
            index = nextIndex;
        }
    } else {
        
        index = findCommonNode(index, UCOL_SECONDARY);
        if(strength >= UCOL_TERTIARY) {
            index = findCommonNode(index, UCOL_TERTIARY);
        }
        
        
        node = nodes.elementAti(index);
        if(strengthFromNode(node) == strength) {
            
            uint32_t weight16 = weight16FromNode(node);
            if(weight16 == 0) {
                errorCode = U_UNSUPPORTED_ERROR;
                if(strength == UCOL_SECONDARY) {
                    parserErrorReason = "reset secondary-before secondary ignorable not possible";
                } else {
                    parserErrorReason = "reset tertiary-before completely ignorable not possible";
                }
                return;
            }
            U_ASSERT(weight16 > Collation::BEFORE_WEIGHT16);
            
            
            
            weight16 = getWeight16Before(index, node, strength);
            
            uint32_t previousWeight16;
            int32_t previousIndex = previousIndexFromNode(node);
            for(int32_t i = previousIndex;; i = previousIndexFromNode(node)) {
                node = nodes.elementAti(i);
                int32_t previousStrength = strengthFromNode(node);
                if(previousStrength < strength) {
                    U_ASSERT(weight16 >= Collation::COMMON_WEIGHT16 || i == previousIndex);
                    
                    
                    
                    
                    previousWeight16 = Collation::COMMON_WEIGHT16;
                    break;
                } else if(previousStrength == strength && !isTailoredNode(node)) {
                    previousWeight16 = weight16FromNode(node);
                    break;
                }
                
            }
            if(previousWeight16 == weight16) {
                
                
                
                index = previousIndex;
            } else {
                
                node = nodeFromWeight16(weight16) | nodeFromStrength(strength);
                index = insertNodeBetween(previousIndex, index, node, errorCode);
            }
        } else {
            
            uint32_t weight16 = getWeight16Before(index, node, strength);
            index = findOrInsertWeakNode(index, weight16, strength, errorCode);
        }
        
        
        strength = ceStrength(ces[cesLength - 1]);
    }
    if(U_FAILURE(errorCode)) {
        parserErrorReason = "inserting reset position for &[before n]";
        return;
    }
    ces[cesLength - 1] = tempCEFromIndexAndStrength(index, strength);
}

uint32_t
CollationBuilder::getWeight16Before(int32_t index, int64_t node, int32_t level) {
    U_ASSERT(strengthFromNode(node) < level || !isTailoredNode(node));
    
    
    uint32_t t;
    if(strengthFromNode(node) == UCOL_TERTIARY) {
        t = weight16FromNode(node);
    } else {
        t = Collation::COMMON_WEIGHT16;  
    }
    while(strengthFromNode(node) > UCOL_SECONDARY) {
        index = previousIndexFromNode(node);
        node = nodes.elementAti(index);
    }
    if(isTailoredNode(node)) {
        return Collation::BEFORE_WEIGHT16;
    }
    uint32_t s;
    if(strengthFromNode(node) == UCOL_SECONDARY) {
        s = weight16FromNode(node);
    } else {
        s = Collation::COMMON_WEIGHT16;  
    }
    while(strengthFromNode(node) > UCOL_PRIMARY) {
        index = previousIndexFromNode(node);
        node = nodes.elementAti(index);
    }
    if(isTailoredNode(node)) {
        return Collation::BEFORE_WEIGHT16;
    }
    
    uint32_t p = weight32FromNode(node);
    uint32_t weight16;
    if(level == UCOL_SECONDARY) {
        weight16 = rootElements.getSecondaryBefore(p, s);
    } else {
        weight16 = rootElements.getTertiaryBefore(p, s, t);
        U_ASSERT((weight16 & ~Collation::ONLY_TERTIARY_MASK) == 0);
    }
    return weight16;
}

int64_t
CollationBuilder::getSpecialResetPosition(const UnicodeString &str,
                                          const char *&parserErrorReason, UErrorCode &errorCode) {
    U_ASSERT(str.length() == 2);
    int64_t ce;
    int32_t strength = UCOL_PRIMARY;
    UBool isBoundary = FALSE;
    UChar32 pos = str.charAt(1) - CollationRuleParser::POS_BASE;
    U_ASSERT(0 <= pos && pos <= CollationRuleParser::LAST_TRAILING);
    switch(pos) {
    case CollationRuleParser::FIRST_TERTIARY_IGNORABLE:
        
        
        return 0;
    case CollationRuleParser::LAST_TERTIARY_IGNORABLE:
        return 0;
    case CollationRuleParser::FIRST_SECONDARY_IGNORABLE: {
        
        int32_t index = findOrInsertNodeForRootCE(0, UCOL_TERTIARY, errorCode);
        if(U_FAILURE(errorCode)) { return 0; }
        int64_t node = nodes.elementAti(index);
        if((index = nextIndexFromNode(node)) != 0) {
            node = nodes.elementAti(index);
            U_ASSERT(strengthFromNode(node) <= UCOL_TERTIARY);
            if(isTailoredNode(node) && strengthFromNode(node) == UCOL_TERTIARY) {
                return tempCEFromIndexAndStrength(index, UCOL_TERTIARY);
            }
        }
        return rootElements.getFirstTertiaryCE();
        
    }
    case CollationRuleParser::LAST_SECONDARY_IGNORABLE:
        ce = rootElements.getLastTertiaryCE();
        strength = UCOL_TERTIARY;
        break;
    case CollationRuleParser::FIRST_PRIMARY_IGNORABLE: {
        
        int32_t index = findOrInsertNodeForRootCE(0, UCOL_SECONDARY, errorCode);
        if(U_FAILURE(errorCode)) { return 0; }
        int64_t node = nodes.elementAti(index);
        while((index = nextIndexFromNode(node)) != 0) {
            node = nodes.elementAti(index);
            strength = strengthFromNode(node);
            if(strength < UCOL_SECONDARY) { break; }
            if(strength == UCOL_SECONDARY) {
                if(isTailoredNode(node)) {
                    if(nodeHasBefore3(node)) {
                        index = nextIndexFromNode(nodes.elementAti(nextIndexFromNode(node)));
                        U_ASSERT(isTailoredNode(nodes.elementAti(index)));
                    }
                    return tempCEFromIndexAndStrength(index, UCOL_SECONDARY);
                } else {
                    break;
                }
            }
        }
        ce = rootElements.getFirstSecondaryCE();
        strength = UCOL_SECONDARY;
        break;
    }
    case CollationRuleParser::LAST_PRIMARY_IGNORABLE:
        ce = rootElements.getLastSecondaryCE();
        strength = UCOL_SECONDARY;
        break;
    case CollationRuleParser::FIRST_VARIABLE:
        ce = rootElements.getFirstPrimaryCE();
        isBoundary = TRUE;  
        break;
    case CollationRuleParser::LAST_VARIABLE:
        ce = rootElements.lastCEWithPrimaryBefore(variableTop + 1);
        break;
    case CollationRuleParser::FIRST_REGULAR:
        ce = rootElements.firstCEWithPrimaryAtLeast(variableTop + 1);
        isBoundary = TRUE;  
        break;
    case CollationRuleParser::LAST_REGULAR:
        
        
        
        ce = rootElements.firstCEWithPrimaryAtLeast(
            baseData->getFirstPrimaryForGroup(USCRIPT_HAN));
        break;
    case CollationRuleParser::FIRST_IMPLICIT:
        ce = baseData->getSingleCE(0x4e00, errorCode);
        break;
    case CollationRuleParser::LAST_IMPLICIT:
        
        errorCode = U_UNSUPPORTED_ERROR;
        parserErrorReason = "reset to [last implicit] not supported";
        return 0;
    case CollationRuleParser::FIRST_TRAILING:
        ce = Collation::makeCE(Collation::FIRST_TRAILING_PRIMARY);
        isBoundary = TRUE;  
        break;
    case CollationRuleParser::LAST_TRAILING:
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        parserErrorReason = "LDML forbids tailoring to U+FFFF";
        return 0;
    default:
        U_ASSERT(FALSE);
        return 0;
    }

    int32_t index = findOrInsertNodeForRootCE(ce, strength, errorCode);
    if(U_FAILURE(errorCode)) { return 0; }
    int64_t node = nodes.elementAti(index);
    if((pos & 1) == 0) {
        
        if(!nodeHasAnyBefore(node) && isBoundary) {
            
            
            
            
            if((index = nextIndexFromNode(node)) != 0) {
                
                
                
                node = nodes.elementAti(index);
                U_ASSERT(isTailoredNode(node));
                ce = tempCEFromIndexAndStrength(index, strength);
            } else {
                U_ASSERT(strength == UCOL_PRIMARY);
                uint32_t p = (uint32_t)(ce >> 32);
                int32_t pIndex = rootElements.findPrimary(p);
                UBool isCompressible = baseData->isCompressiblePrimary(p);
                p = rootElements.getPrimaryAfter(p, pIndex, isCompressible);
                ce = Collation::makeCE(p);
                index = findOrInsertNodeForRootCE(ce, UCOL_PRIMARY, errorCode);
                if(U_FAILURE(errorCode)) { return 0; }
                node = nodes.elementAti(index);
            }
        }
        if(nodeHasAnyBefore(node)) {
            
            if(nodeHasBefore2(node)) {
                index = nextIndexFromNode(nodes.elementAti(nextIndexFromNode(node)));
                node = nodes.elementAti(index);
            }
            if(nodeHasBefore3(node)) {
                index = nextIndexFromNode(nodes.elementAti(nextIndexFromNode(node)));
            }
            U_ASSERT(isTailoredNode(nodes.elementAti(index)));
            ce = tempCEFromIndexAndStrength(index, strength);
        }
    } else {
        
        
        
        for(;;) {
            int32_t nextIndex = nextIndexFromNode(node);
            if(nextIndex == 0) { break; }
            int64_t nextNode = nodes.elementAti(nextIndex);
            if(strengthFromNode(nextNode) < strength) { break; }
            index = nextIndex;
            node = nextNode;
        }
        
        
        
        if(isTailoredNode(node)) {
            ce = tempCEFromIndexAndStrength(index, strength);
        }
    }
    return ce;
}

void
CollationBuilder::addRelation(int32_t strength, const UnicodeString &prefix,
                              const UnicodeString &str, const UnicodeString &extension,
                              const char *&parserErrorReason, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return; }
    UnicodeString nfdPrefix;
    if(!prefix.isEmpty()) {
        nfd.normalize(prefix, nfdPrefix, errorCode);
        if(U_FAILURE(errorCode)) {
            parserErrorReason = "normalizing the relation prefix";
            return;
        }
    }
    UnicodeString nfdString = nfd.normalize(str, errorCode);
    if(U_FAILURE(errorCode)) {
        parserErrorReason = "normalizing the relation string";
        return;
    }

    
    
    
    int32_t nfdLength = nfdString.length();
    if(nfdLength >= 2) {
        UChar c = nfdString.charAt(0);
        if(Hangul::isJamoL(c) || Hangul::isJamoV(c)) {
            
            
            errorCode = U_UNSUPPORTED_ERROR;
            parserErrorReason = "contractions starting with conjoining Jamo L or V not supported";
            return;
        }
        c = nfdString.charAt(nfdLength - 1);
        if(Hangul::isJamoL(c) ||
                (Hangul::isJamoV(c) && Hangul::isJamoL(nfdString.charAt(nfdLength - 2)))) {
            
            
            
            errorCode = U_UNSUPPORTED_ERROR;
            parserErrorReason = "contractions ending with conjoining Jamo L or L+V not supported";
            return;
        }
        
    }
    
    
    
    
    

    if(strength != UCOL_IDENTICAL) {
        
        int32_t index = findOrInsertNodeForCEs(strength, parserErrorReason, errorCode);
        U_ASSERT(cesLength > 0);
        int64_t ce = ces[cesLength - 1];
        if(strength == UCOL_PRIMARY && !isTempCE(ce) && (uint32_t)(ce >> 32) == 0) {
            
            errorCode = U_UNSUPPORTED_ERROR;
            parserErrorReason = "tailoring primary after ignorables not supported";
            return;
        }
        if(strength == UCOL_QUATERNARY && ce == 0) {
            
            
            errorCode = U_UNSUPPORTED_ERROR;
            parserErrorReason = "tailoring quaternary after tertiary ignorables not supported";
            return;
        }
        
        index = insertTailoredNodeAfter(index, strength, errorCode);
        if(U_FAILURE(errorCode)) {
            parserErrorReason = "modifying collation elements";
            return;
        }
        
        
        int32_t tempStrength = ceStrength(ce);
        if(strength < tempStrength) { tempStrength = strength; }
        ces[cesLength - 1] = tempCEFromIndexAndStrength(index, tempStrength);
    }

    setCaseBits(nfdString, parserErrorReason, errorCode);
    if(U_FAILURE(errorCode)) { return; }

    int32_t cesLengthBeforeExtension = cesLength;
    if(!extension.isEmpty()) {
        UnicodeString nfdExtension = nfd.normalize(extension, errorCode);
        if(U_FAILURE(errorCode)) {
            parserErrorReason = "normalizing the relation extension";
            return;
        }
        cesLength = dataBuilder->getCEs(nfdExtension, ces, cesLength);
        if(cesLength > Collation::MAX_EXPANSION_LENGTH) {
            errorCode = U_ILLEGAL_ARGUMENT_ERROR;
            parserErrorReason =
                "extension string adds too many collation elements (more than 31 total)";
            return;
        }
    }
    uint32_t ce32 = Collation::UNASSIGNED_CE32;
    if((prefix != nfdPrefix || str != nfdString) &&
            !ignorePrefix(prefix, errorCode) && !ignoreString(str, errorCode)) {
        
        
        
        ce32 = addIfDifferent(prefix, str, ces, cesLength, ce32, errorCode);
    }
    addWithClosure(nfdPrefix, nfdString, ces, cesLength, ce32, errorCode);
    if(U_FAILURE(errorCode)) {
        parserErrorReason = "writing collation elements";
        return;
    }
    cesLength = cesLengthBeforeExtension;
}

int32_t
CollationBuilder::findOrInsertNodeForCEs(int32_t strength, const char *&parserErrorReason,
                                         UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return 0; }
    U_ASSERT(UCOL_PRIMARY <= strength && strength <= UCOL_QUATERNARY);

    
    
    int64_t ce;
    for(;; --cesLength) {
        if(cesLength == 0) {
            ce = ces[0] = 0;
            cesLength = 1;
            break;
        } else {
            ce = ces[cesLength - 1];
        }
        if(ceStrength(ce) <= strength) { break; }
    }

    if(isTempCE(ce)) {
        
        
        return indexFromTempCE(ce);
    }

    
    if((uint8_t)(ce >> 56) == Collation::UNASSIGNED_IMPLICIT_BYTE) {
        errorCode = U_UNSUPPORTED_ERROR;
        parserErrorReason = "tailoring relative to an unassigned code point not supported";
        return 0;
    }
    return findOrInsertNodeForRootCE(ce, strength, errorCode);
}

int32_t
CollationBuilder::findOrInsertNodeForRootCE(int64_t ce, int32_t strength, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return 0; }
    U_ASSERT((uint8_t)(ce >> 56) != Collation::UNASSIGNED_IMPLICIT_BYTE);

    
    
    
    U_ASSERT((ce & 0xc0) == 0);
    int32_t index = findOrInsertNodeForPrimary((uint32_t)(ce >> 32), errorCode);
    if(strength >= UCOL_SECONDARY) {
        uint32_t lower32 = (uint32_t)ce;
        index = findOrInsertWeakNode(index, lower32 >> 16, UCOL_SECONDARY, errorCode);
        if(strength >= UCOL_TERTIARY) {
            index = findOrInsertWeakNode(index, lower32 & Collation::ONLY_TERTIARY_MASK,
                                         UCOL_TERTIARY, errorCode);
        }
    }
    return index;
}

namespace {








int32_t
binarySearchForRootPrimaryNode(const int32_t *rootPrimaryIndexes, int32_t length,
                               const int64_t *nodes, uint32_t p) {
    if(length == 0) { return ~0; }
    int32_t start = 0;
    int32_t limit = length;
    for (;;) {
        int32_t i = (start + limit) / 2;
        int64_t node = nodes[rootPrimaryIndexes[i]];
        uint32_t nodePrimary = (uint32_t)(node >> 32);  
        if (p == nodePrimary) {
            return i;
        } else if (p < nodePrimary) {
            if (i == start) {
                return ~start;  
            }
            limit = i;
        } else {
            if (i == start) {
                return ~(start + 1);  
            }
            start = i;
        }
    }
}

}  

int32_t
CollationBuilder::findOrInsertNodeForPrimary(uint32_t p, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return 0; }

    int32_t rootIndex = binarySearchForRootPrimaryNode(
        rootPrimaryIndexes.getBuffer(), rootPrimaryIndexes.size(), nodes.getBuffer(), p);
    if(rootIndex >= 0) {
        return rootPrimaryIndexes.elementAti(rootIndex);
    } else {
        
        int32_t index = nodes.size();
        nodes.addElement(nodeFromWeight32(p), errorCode);
        rootPrimaryIndexes.insertElementAt(index, ~rootIndex, errorCode);
        return index;
    }
}

int32_t
CollationBuilder::findOrInsertWeakNode(int32_t index, uint32_t weight16, int32_t level, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return 0; }
    U_ASSERT(0 <= index && index < nodes.size());
    U_ASSERT(UCOL_SECONDARY <= level && level <= UCOL_TERTIARY);

    if(weight16 == Collation::COMMON_WEIGHT16) {
        return findCommonNode(index, level);
    }

    
    
    int64_t node = nodes.elementAti(index);
    U_ASSERT(strengthFromNode(node) < level);  
    if(weight16 != 0 && weight16 < Collation::COMMON_WEIGHT16) {
        int32_t hasThisLevelBefore = level == UCOL_SECONDARY ? HAS_BEFORE2 : HAS_BEFORE3;
        if((node & hasThisLevelBefore) == 0) {
            
            int64_t commonNode =
                nodeFromWeight16(Collation::COMMON_WEIGHT16) | nodeFromStrength(level);
            if(level == UCOL_SECONDARY) {
                
                
                commonNode |= node & HAS_BEFORE3;
                node &= ~(int64_t)HAS_BEFORE3;
            }
            nodes.setElementAt(node | hasThisLevelBefore, index);
            
            int32_t nextIndex = nextIndexFromNode(node);
            node = nodeFromWeight16(weight16) | nodeFromStrength(level);
            index = insertNodeBetween(index, nextIndex, node, errorCode);
            
            insertNodeBetween(index, nextIndex, commonNode, errorCode);
            
            return index;
        }
    }

    
    
    
    
    int32_t nextIndex;
    while((nextIndex = nextIndexFromNode(node)) != 0) {
        node = nodes.elementAti(nextIndex);
        int32_t nextStrength = strengthFromNode(node);
        if(nextStrength <= level) {
            
            if(nextStrength < level) { break; }
            
            if(!isTailoredNode(node)) {
                uint32_t nextWeight16 = weight16FromNode(node);
                if(nextWeight16 == weight16) {
                    
                    return nextIndex;
                }
                
                if(nextWeight16 > weight16) { break; }
            }
        }
        
        index = nextIndex;
    }
    node = nodeFromWeight16(weight16) | nodeFromStrength(level);
    return insertNodeBetween(index, nextIndex, node, errorCode);
}

int32_t
CollationBuilder::insertTailoredNodeAfter(int32_t index, int32_t strength, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return 0; }
    U_ASSERT(0 <= index && index < nodes.size());
    if(strength >= UCOL_SECONDARY) {
        index = findCommonNode(index, UCOL_SECONDARY);
        if(strength >= UCOL_TERTIARY) {
            index = findCommonNode(index, UCOL_TERTIARY);
        }
    }
    
    
    int64_t node = nodes.elementAti(index);
    int32_t nextIndex;
    while((nextIndex = nextIndexFromNode(node)) != 0) {
        node = nodes.elementAti(nextIndex);
        if(strengthFromNode(node) <= strength) { break; }
        
        index = nextIndex;
    }
    node = IS_TAILORED | nodeFromStrength(strength);
    return insertNodeBetween(index, nextIndex, node, errorCode);
}

int32_t
CollationBuilder::insertNodeBetween(int32_t index, int32_t nextIndex, int64_t node,
                                    UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return 0; }
    U_ASSERT(previousIndexFromNode(node) == 0);
    U_ASSERT(nextIndexFromNode(node) == 0);
    U_ASSERT(nextIndexFromNode(nodes.elementAti(index)) == nextIndex);
    
    int32_t newIndex = nodes.size();
    node |= nodeFromPreviousIndex(index) | nodeFromNextIndex(nextIndex);
    nodes.addElement(node, errorCode);
    if(U_FAILURE(errorCode)) { return 0; }
    
    node = nodes.elementAti(index);
    nodes.setElementAt(changeNodeNextIndex(node, newIndex), index);
    
    if(nextIndex != 0) {
        node = nodes.elementAti(nextIndex);
        nodes.setElementAt(changeNodePreviousIndex(node, newIndex), nextIndex);
    }
    return newIndex;
}

int32_t
CollationBuilder::findCommonNode(int32_t index, int32_t strength) const {
    U_ASSERT(UCOL_SECONDARY <= strength && strength <= UCOL_TERTIARY);
    int64_t node = nodes.elementAti(index);
    if(strengthFromNode(node) >= strength) {
        
        return index;
    }
    if(strength == UCOL_SECONDARY ? !nodeHasBefore2(node) : !nodeHasBefore3(node)) {
        
        return index;
    }
    index = nextIndexFromNode(node);
    node = nodes.elementAti(index);
    U_ASSERT(!isTailoredNode(node) && strengthFromNode(node) == strength &&
            weight16FromNode(node) < Collation::COMMON_WEIGHT16);
    
    do {
        index = nextIndexFromNode(node);
        node = nodes.elementAti(index);
        U_ASSERT(strengthFromNode(node) >= strength);
    } while(isTailoredNode(node) || strengthFromNode(node) > strength ||
            weight16FromNode(node) < Collation::COMMON_WEIGHT16);
    U_ASSERT(weight16FromNode(node) == Collation::COMMON_WEIGHT16);
    return index;
}

void
CollationBuilder::setCaseBits(const UnicodeString &nfdString,
                              const char *&parserErrorReason, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return; }
    int32_t numTailoredPrimaries = 0;
    for(int32_t i = 0; i < cesLength; ++i) {
        if(ceStrength(ces[i]) == UCOL_PRIMARY) { ++numTailoredPrimaries; }
    }
    
    
    
    U_ASSERT(numTailoredPrimaries <= 31);

    int64_t cases = 0;
    if(numTailoredPrimaries > 0) {
        const UChar *s = nfdString.getBuffer();
        UTF16CollationIterator baseCEs(baseData, FALSE, s, s, s + nfdString.length());
        int32_t baseCEsLength = baseCEs.fetchCEs(errorCode) - 1;
        if(U_FAILURE(errorCode)) {
            parserErrorReason = "fetching root CEs for tailored string";
            return;
        }
        U_ASSERT(baseCEsLength >= 0 && baseCEs.getCE(baseCEsLength) == Collation::NO_CE);

        uint32_t lastCase = 0;
        int32_t numBasePrimaries = 0;
        for(int32_t i = 0; i < baseCEsLength; ++i) {
            int64_t ce = baseCEs.getCE(i);
            if((ce >> 32) != 0) {
                ++numBasePrimaries;
                uint32_t c = ((uint32_t)ce >> 14) & 3;
                U_ASSERT(c == 0 || c == 2);  
                if(numBasePrimaries < numTailoredPrimaries) {
                    cases |= (int64_t)c << ((numBasePrimaries - 1) * 2);
                } else if(numBasePrimaries == numTailoredPrimaries) {
                    lastCase = c;
                } else if(c != lastCase) {
                    
                    
                    lastCase = 1;
                    
                    break;
                }
            }
        }
        if(numBasePrimaries >= numTailoredPrimaries) {
            cases |= (int64_t)lastCase << ((numTailoredPrimaries - 1) * 2);
        }
    }

    for(int32_t i = 0; i < cesLength; ++i) {
        int64_t ce = ces[i] & INT64_C(0xffffffffffff3fff);  
        int32_t strength = ceStrength(ce);
        if(strength == UCOL_PRIMARY) {
            ce |= (cases & 3) << 14;
            cases >>= 2;
        } else if(strength == UCOL_TERTIARY) {
            
            
            ce |= 0x8000;
        }
        
        
        
        
        
        ces[i] = ce;
    }
}

void
CollationBuilder::suppressContractions(const UnicodeSet &set, const char *&parserErrorReason,
                                       UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return; }
    dataBuilder->suppressContractions(set, errorCode);
    if(U_FAILURE(errorCode)) {
        parserErrorReason = "application of [suppressContractions [set]] failed";
    }
}

void
CollationBuilder::optimize(const UnicodeSet &set, const char *& ,
                           UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return; }
    optimizeSet.addAll(set);
}

uint32_t
CollationBuilder::addWithClosure(const UnicodeString &nfdPrefix, const UnicodeString &nfdString,
                                 const int64_t newCEs[], int32_t newCEsLength, uint32_t ce32,
                                 UErrorCode &errorCode) {
    
    ce32 = addIfDifferent(nfdPrefix, nfdString, newCEs, newCEsLength, ce32, errorCode);
    ce32 = addOnlyClosure(nfdPrefix, nfdString, newCEs, newCEsLength, ce32, errorCode);
    addTailComposites(nfdPrefix, nfdString, errorCode);
    return ce32;
}

uint32_t
CollationBuilder::addOnlyClosure(const UnicodeString &nfdPrefix, const UnicodeString &nfdString,
                                 const int64_t newCEs[], int32_t newCEsLength, uint32_t ce32,
                                 UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return ce32; }

    
    if(nfdPrefix.isEmpty()) {
        CanonicalIterator stringIter(nfdString, errorCode);
        if(U_FAILURE(errorCode)) { return ce32; }
        UnicodeString prefix;
        for(;;) {
            UnicodeString str = stringIter.next();
            if(str.isBogus()) { break; }
            if(ignoreString(str, errorCode) || str == nfdString) { continue; }
            ce32 = addIfDifferent(prefix, str, newCEs, newCEsLength, ce32, errorCode);
            if(U_FAILURE(errorCode)) { return ce32; }
        }
    } else {
        CanonicalIterator prefixIter(nfdPrefix, errorCode);
        CanonicalIterator stringIter(nfdString, errorCode);
        if(U_FAILURE(errorCode)) { return ce32; }
        for(;;) {
            UnicodeString prefix = prefixIter.next();
            if(prefix.isBogus()) { break; }
            if(ignorePrefix(prefix, errorCode)) { continue; }
            UBool samePrefix = prefix == nfdPrefix;
            for(;;) {
                UnicodeString str = stringIter.next();
                if(str.isBogus()) { break; }
                if(ignoreString(str, errorCode) || (samePrefix && str == nfdString)) { continue; }
                ce32 = addIfDifferent(prefix, str, newCEs, newCEsLength, ce32, errorCode);
                if(U_FAILURE(errorCode)) { return ce32; }
            }
            stringIter.reset();
        }
    }
    return ce32;
}

void
CollationBuilder::addTailComposites(const UnicodeString &nfdPrefix, const UnicodeString &nfdString,
                                    UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return; }

    
    UChar32 lastStarter;
    int32_t indexAfterLastStarter = nfdString.length();
    for(;;) {
        if(indexAfterLastStarter == 0) { return; }  
        lastStarter = nfdString.char32At(indexAfterLastStarter - 1);
        if(nfd.getCombiningClass(lastStarter) == 0) { break; }
        indexAfterLastStarter -= U16_LENGTH(lastStarter);
    }
    
    if(Hangul::isJamoL(lastStarter)) { return; }

    
    
    
    UnicodeSet composites;
    if(!nfcImpl.getCanonStartSet(lastStarter, composites)) { return; }

    UnicodeString decomp;
    UnicodeString newNFDString, newString;
    int64_t newCEs[Collation::MAX_EXPANSION_LENGTH];
    UnicodeSetIterator iter(composites);
    while(iter.next()) {
        U_ASSERT(!iter.isString());
        UChar32 composite = iter.getCodepoint();
        nfd.getDecomposition(composite, decomp);
        if(!mergeCompositeIntoString(nfdString, indexAfterLastStarter, composite, decomp,
                                     newNFDString, newString, errorCode)) {
            continue;
        }
        int32_t newCEsLength = dataBuilder->getCEs(nfdPrefix, newNFDString, newCEs, 0);
        if(newCEsLength > Collation::MAX_EXPANSION_LENGTH) {
            
            continue;
        }
        
        
        
        
        
        
        
        
        
        
        

        
        
        
        uint32_t ce32 = addIfDifferent(nfdPrefix, newString,
                                       newCEs, newCEsLength, Collation::UNASSIGNED_CE32, errorCode);
        if(ce32 != Collation::UNASSIGNED_CE32) {
            
            addOnlyClosure(nfdPrefix, newNFDString, newCEs, newCEsLength, ce32, errorCode);
        }
    }
}

UBool
CollationBuilder::mergeCompositeIntoString(const UnicodeString &nfdString,
                                           int32_t indexAfterLastStarter,
                                           UChar32 composite, const UnicodeString &decomp,
                                           UnicodeString &newNFDString, UnicodeString &newString,
                                           UErrorCode &errorCode) const {
    if(U_FAILURE(errorCode)) { return FALSE; }
    U_ASSERT(nfdString.char32At(indexAfterLastStarter - 1) == decomp.char32At(0));
    int32_t lastStarterLength = decomp.moveIndex32(0, 1);
    if(lastStarterLength == decomp.length()) {
        
        
        return FALSE;
    }
    if(nfdString.compare(indexAfterLastStarter, 0x7fffffff,
                         decomp, lastStarterLength, 0x7fffffff) == 0) {
        
        return FALSE;
    }

    
    
    
    newNFDString.setTo(nfdString, 0, indexAfterLastStarter);
    newString.setTo(nfdString, 0, indexAfterLastStarter - lastStarterLength).append(composite);

    
    
    int32_t sourceIndex = indexAfterLastStarter;
    int32_t decompIndex = lastStarterLength;
    
    
    
    UChar32 sourceChar = U_SENTINEL;
    
    
    uint8_t sourceCC = 0;
    uint8_t decompCC = 0;
    for(;;) {
        if(sourceChar < 0) {
            if(sourceIndex >= nfdString.length()) { break; }
            sourceChar = nfdString.char32At(sourceIndex);
            sourceCC = nfd.getCombiningClass(sourceChar);
            U_ASSERT(sourceCC != 0);
        }
        
        if(decompIndex >= decomp.length()) { break; }
        UChar32 decompChar = decomp.char32At(decompIndex);
        decompCC = nfd.getCombiningClass(decompChar);
        
        if(decompCC == 0) {
            
            
            
            return FALSE;
        } else if(sourceCC < decompCC) {
            
            return FALSE;
        } else if(decompCC < sourceCC) {
            newNFDString.append(decompChar);
            decompIndex += U16_LENGTH(decompChar);
        } else if(decompChar != sourceChar) {
            
            return FALSE;
        } else {  
            newNFDString.append(decompChar);
            decompIndex += U16_LENGTH(decompChar);
            sourceIndex += U16_LENGTH(decompChar);
            sourceChar = U_SENTINEL;
        }
    }
    
    if(sourceChar >= 0) {  
        if(sourceCC < decompCC) {
            
            return FALSE;
        }
        newNFDString.append(nfdString, sourceIndex, 0x7fffffff);
        newString.append(nfdString, sourceIndex, 0x7fffffff);
    } else if(decompIndex < decomp.length()) {  
        newNFDString.append(decomp, decompIndex, 0x7fffffff);
    }
    U_ASSERT(nfd.isNormalized(newNFDString, errorCode));
    U_ASSERT(fcd.isNormalized(newString, errorCode));
    U_ASSERT(nfd.normalize(newString, errorCode) == newNFDString);  
    return TRUE;
}

UBool
CollationBuilder::ignorePrefix(const UnicodeString &s, UErrorCode &errorCode) const {
    
    return !isFCD(s, errorCode);
}

UBool
CollationBuilder::ignoreString(const UnicodeString &s, UErrorCode &errorCode) const {
    
    
    return !isFCD(s, errorCode) || Hangul::isHangul(s.charAt(0));
}

UBool
CollationBuilder::isFCD(const UnicodeString &s, UErrorCode &errorCode) const {
    return U_SUCCESS(errorCode) && fcd.isNormalized(s, errorCode);
}

void
CollationBuilder::closeOverComposites(UErrorCode &errorCode) {
    UnicodeSet composites(UNICODE_STRING_SIMPLE("[:NFD_QC=N:]"), errorCode);  
    if(U_FAILURE(errorCode)) { return; }
    
    composites.remove(Hangul::HANGUL_BASE, Hangul::HANGUL_END);
    UnicodeString prefix;  
    UnicodeString nfdString;
    UnicodeSetIterator iter(composites);
    while(iter.next()) {
        U_ASSERT(!iter.isString());
        nfd.getDecomposition(iter.getCodepoint(), nfdString);
        cesLength = dataBuilder->getCEs(nfdString, ces, 0);
        if(cesLength > Collation::MAX_EXPANSION_LENGTH) {
            
            
            
            continue;
        }
        const UnicodeString &composite(iter.getString());
        addIfDifferent(prefix, composite, ces, cesLength, Collation::UNASSIGNED_CE32, errorCode);
    }
}

uint32_t
CollationBuilder::addIfDifferent(const UnicodeString &prefix, const UnicodeString &str,
                                 const int64_t newCEs[], int32_t newCEsLength, uint32_t ce32,
                                 UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return ce32; }
    int64_t oldCEs[Collation::MAX_EXPANSION_LENGTH];
    int32_t oldCEsLength = dataBuilder->getCEs(prefix, str, oldCEs, 0);
    if(!sameCEs(newCEs, newCEsLength, oldCEs, oldCEsLength)) {
        if(ce32 == Collation::UNASSIGNED_CE32) {
            ce32 = dataBuilder->encodeCEs(newCEs, newCEsLength, errorCode);
        }
        dataBuilder->addCE32(prefix, str, ce32, errorCode);
    }
    return ce32;
}

UBool
CollationBuilder::sameCEs(const int64_t ces1[], int32_t ces1Length,
                          const int64_t ces2[], int32_t ces2Length) {
    if(ces1Length != ces2Length) {
        return FALSE;
    }
    U_ASSERT(ces1Length <= Collation::MAX_EXPANSION_LENGTH);
    for(int32_t i = 0; i < ces1Length; ++i) {
        if(ces1[i] != ces2[i]) { return FALSE; }
    }
    return TRUE;
}

#ifdef DEBUG_COLLATION_BUILDER

uint32_t
alignWeightRight(uint32_t w) {
    if(w != 0) {
        while((w & 0xff) == 0) { w >>= 8; }
    }
    return w;
}

#endif

void
CollationBuilder::makeTailoredCEs(UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return; }

    CollationWeights primaries, secondaries, tertiaries;
    int64_t *nodesArray = nodes.getBuffer();
#ifdef DEBUG_COLLATION_BUILDER
        puts("\nCollationBuilder::makeTailoredCEs()");
#endif

    for(int32_t rpi = 0; rpi < rootPrimaryIndexes.size(); ++rpi) {
        int32_t i = rootPrimaryIndexes.elementAti(rpi);
        int64_t node = nodesArray[i];
        uint32_t p = weight32FromNode(node);
        uint32_t s = p == 0 ? 0 : Collation::COMMON_WEIGHT16;
        uint32_t t = s;
        uint32_t q = 0;
        UBool pIsTailored = FALSE;
        UBool sIsTailored = FALSE;
        UBool tIsTailored = FALSE;
#ifdef DEBUG_COLLATION_BUILDER
        printf("\nprimary     %lx\n", (long)alignWeightRight(p));
#endif
        int32_t pIndex = p == 0 ? 0 : rootElements.findPrimary(p);
        int32_t nextIndex = nextIndexFromNode(node);
        while(nextIndex != 0) {
            i = nextIndex;
            node = nodesArray[i];
            nextIndex = nextIndexFromNode(node);
            int32_t strength = strengthFromNode(node);
            if(strength == UCOL_QUATERNARY) {
                U_ASSERT(isTailoredNode(node));
#ifdef DEBUG_COLLATION_BUILDER
                printf("      quat+     ");
#endif
                if(q == 3) {
                    errorCode = U_BUFFER_OVERFLOW_ERROR;
                    errorReason = "quaternary tailoring gap too small";
                    return;
                }
                ++q;
            } else {
                if(strength == UCOL_TERTIARY) {
                    if(isTailoredNode(node)) {
#ifdef DEBUG_COLLATION_BUILDER
                        printf("    ter+        ");
#endif
                        if(!tIsTailored) {
                            
                            int32_t tCount = countTailoredNodes(nodesArray, nextIndex,
                                                                UCOL_TERTIARY) + 1;
                            uint32_t tLimit;
                            if(t == 0) {
                                
                                t = rootElements.getTertiaryBoundary() - 0x100;
                                tLimit = rootElements.getFirstTertiaryCE() & Collation::ONLY_TERTIARY_MASK;
                            } else if(!pIsTailored && !sIsTailored) {
                                
                                tLimit = rootElements.getTertiaryAfter(pIndex, s, t);
                            } else if(t == Collation::BEFORE_WEIGHT16) {
                                tLimit = Collation::COMMON_WEIGHT16;
                            } else {
                                
                                U_ASSERT(t == Collation::COMMON_WEIGHT16);
                                tLimit = rootElements.getTertiaryBoundary();
                            }
                            U_ASSERT(tLimit == 0x4000 || (tLimit & ~Collation::ONLY_TERTIARY_MASK) == 0);
                            tertiaries.initForTertiary();
                            if(!tertiaries.allocWeights(t, tLimit, tCount)) {
                                errorCode = U_BUFFER_OVERFLOW_ERROR;
                                errorReason = "tertiary tailoring gap too small";
                                return;
                            }
                            tIsTailored = TRUE;
                        }
                        t = tertiaries.nextWeight();
                        U_ASSERT(t != 0xffffffff);
                    } else {
                        t = weight16FromNode(node);
                        tIsTailored = FALSE;
#ifdef DEBUG_COLLATION_BUILDER
                        printf("    ter     %lx\n", (long)alignWeightRight(t));
#endif
                    }
                } else {
                    if(strength == UCOL_SECONDARY) {
                        if(isTailoredNode(node)) {
#ifdef DEBUG_COLLATION_BUILDER
                            printf("  sec+          ");
#endif
                            if(!sIsTailored) {
                                
                                int32_t sCount = countTailoredNodes(nodesArray, nextIndex,
                                                                    UCOL_SECONDARY) + 1;
                                uint32_t sLimit;
                                if(s == 0) {
                                    
                                    s = rootElements.getSecondaryBoundary() - 0x100;
                                    sLimit = rootElements.getFirstSecondaryCE() >> 16;
                                } else if(!pIsTailored) {
                                    
                                    sLimit = rootElements.getSecondaryAfter(pIndex, s);
                                } else if(s == Collation::BEFORE_WEIGHT16) {
                                    sLimit = Collation::COMMON_WEIGHT16;
                                } else {
                                    
                                    U_ASSERT(s == Collation::COMMON_WEIGHT16);
                                    sLimit = rootElements.getSecondaryBoundary();
                                }
                                if(s == Collation::COMMON_WEIGHT16) {
                                    
                                    
                                    s = rootElements.getLastCommonSecondary();
                                }
                                secondaries.initForSecondary();
                                if(!secondaries.allocWeights(s, sLimit, sCount)) {
                                    errorCode = U_BUFFER_OVERFLOW_ERROR;
                                    errorReason = "secondary tailoring gap too small";
#ifdef DEBUG_COLLATION_BUILDER
                                    printf("!secondaries.allocWeights(%lx, %lx, sCount=%ld)\n",
                                           (long)alignWeightRight(s), (long)alignWeightRight(sLimit),
                                           (long)alignWeightRight(sCount));
#endif
                                    return;
                                }
                                sIsTailored = TRUE;
                            }
                            s = secondaries.nextWeight();
                            U_ASSERT(s != 0xffffffff);
                        } else {
                            s = weight16FromNode(node);
                            sIsTailored = FALSE;
#ifdef DEBUG_COLLATION_BUILDER
                            printf("  sec       %lx\n", (long)alignWeightRight(s));
#endif
                        }
                    } else  {
                        U_ASSERT(isTailoredNode(node));
#ifdef DEBUG_COLLATION_BUILDER
                        printf("pri+            ");
#endif
                        if(!pIsTailored) {
                            
                            int32_t pCount = countTailoredNodes(nodesArray, nextIndex,
                                                                UCOL_PRIMARY) + 1;
                            UBool isCompressible = baseData->isCompressiblePrimary(p);
                            uint32_t pLimit =
                                rootElements.getPrimaryAfter(p, pIndex, isCompressible);
                            primaries.initForPrimary(isCompressible);
                            if(!primaries.allocWeights(p, pLimit, pCount)) {
                                errorCode = U_BUFFER_OVERFLOW_ERROR;  
                                errorReason = "primary tailoring gap too small";
                                return;
                            }
                            pIsTailored = TRUE;
                        }
                        p = primaries.nextWeight();
                        U_ASSERT(p != 0xffffffff);
                        s = Collation::COMMON_WEIGHT16;
                        sIsTailored = FALSE;
                    }
                    t = s == 0 ? 0 : Collation::COMMON_WEIGHT16;
                    tIsTailored = FALSE;
                }
                q = 0;
            }
            if(isTailoredNode(node)) {
                nodesArray[i] = Collation::makeCE(p, s, t, q);
#ifdef DEBUG_COLLATION_BUILDER
                printf("%016llx\n", (long long)nodesArray[i]);
#endif
            }
        }
    }
}

int32_t
CollationBuilder::countTailoredNodes(const int64_t *nodesArray, int32_t i, int32_t strength) {
    int32_t count = 0;
    for(;;) {
        if(i == 0) { break; }
        int64_t node = nodesArray[i];
        if(strengthFromNode(node) < strength) { break; }
        if(strengthFromNode(node) == strength) {
            if(isTailoredNode(node)) {
                ++count;
            } else {
                break;
            }
        }
        i = nextIndexFromNode(node);
    }
    return count;
}

class CEFinalizer : public CollationDataBuilder::CEModifier {
public:
    CEFinalizer(const int64_t *ces) : finalCEs(ces) {}
    virtual ~CEFinalizer();
    virtual int64_t modifyCE32(uint32_t ce32) const {
        U_ASSERT(!Collation::isSpecialCE32(ce32));
        if(CollationBuilder::isTempCE32(ce32)) {
            
            return finalCEs[CollationBuilder::indexFromTempCE32(ce32)] | ((ce32 & 0xc0) << 8);
        } else {
            return Collation::NO_CE;
        }
    }
    virtual int64_t modifyCE(int64_t ce) const {
        if(CollationBuilder::isTempCE(ce)) {
            
            return finalCEs[CollationBuilder::indexFromTempCE(ce)] | (ce & 0xc000);
        } else {
            return Collation::NO_CE;
        }
    }

private:
    const int64_t *finalCEs;
};

CEFinalizer::~CEFinalizer() {}

void
CollationBuilder::finalizeCEs(UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return; }
    LocalPointer<CollationDataBuilder> newBuilder(new CollationDataBuilder(errorCode), errorCode);
    if(U_FAILURE(errorCode)) {
        return;
    }
    newBuilder->initForTailoring(baseData, errorCode);
    CEFinalizer finalizer(nodes.getBuffer());
    newBuilder->copyFrom(*dataBuilder, finalizer, errorCode);
    if(U_FAILURE(errorCode)) { return; }
    delete dataBuilder;
    dataBuilder = newBuilder.orphan();
}

int32_t
CollationBuilder::ceStrength(int64_t ce) {
    return
        isTempCE(ce) ? strengthFromTempCE(ce) :
        (ce & INT64_C(0xff00000000000000)) != 0 ? UCOL_PRIMARY :
        ((uint32_t)ce & 0xff000000) != 0 ? UCOL_SECONDARY :
        ce != 0 ? UCOL_TERTIARY :
        UCOL_IDENTICAL;
}

U_NAMESPACE_END

U_NAMESPACE_USE

U_CAPI UCollator * U_EXPORT2
ucol_openRules(const UChar *rules, int32_t rulesLength,
               UColAttributeValue normalizationMode, UCollationStrength strength,
               UParseError *parseError, UErrorCode *pErrorCode) {
    if(U_FAILURE(*pErrorCode)) { return NULL; }
    if(rules == NULL && rulesLength != 0) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    RuleBasedCollator *coll = new RuleBasedCollator();
    if(coll == NULL) {
        *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    UnicodeString r((UBool)(rulesLength < 0), rules, rulesLength);
    coll->internalBuildTailoring(r, strength, normalizationMode, parseError, NULL, *pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        delete coll;
        return NULL;
    }
    return coll->toUCollator();
}

static const int32_t internalBufferSize = 512;







U_CAPI int32_t U_EXPORT2
ucol_getUnsafeSet( const UCollator *coll,
                  USet *unsafe,
                  UErrorCode *status)
{
    UChar buffer[internalBufferSize];
    int32_t len = 0;

    uset_clear(unsafe);

    
    static const UChar cccpattern[25] = { 0x5b, 0x5b, 0x3a, 0x5e, 0x74, 0x63, 0x63, 0x63, 0x3d, 0x30, 0x3a, 0x5d,
                                    0x5b, 0x3a, 0x5e, 0x6c, 0x63, 0x63, 0x63, 0x3d, 0x30, 0x3a, 0x5d, 0x5d, 0x00 };

    
    uset_applyPattern(unsafe, cccpattern, 24, USET_IGNORE_SPACE, status);

    
    
    
    uset_addRange(unsafe, 0xd800, 0xdfff);

    USet *contractions = uset_open(0,0);

    int32_t i = 0, j = 0;
    ucol_getContractionsAndExpansions(coll, contractions, NULL, FALSE, status);
    int32_t contsSize = uset_size(contractions);
    UChar32 c = 0;
    
    
    
    for(i = 0; i < contsSize; i++) {
        len = uset_getItem(contractions, i, NULL, NULL, buffer, internalBufferSize, status);
        if(len > 0) {
            j = 0;
            while(j < len) {
                U16_NEXT(buffer, j, len, c);
                if(j < len) {
                    uset_add(unsafe, c);
                }
            }
        }
    }

    uset_close(contractions);

    return uset_size(unsafe);
}

#endif  
