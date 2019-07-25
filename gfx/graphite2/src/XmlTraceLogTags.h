

























#pragma once
#include "Main.h"
#include <graphite2/Types.h>
#include <graphite2/XmlLog.h>

namespace graphite2 {

#ifndef DISABLE_TRACING


enum XmlTraceLogElement {
    ElementTopLevel,
    ElementFace,
    ElementGlyphs,
    ElementGlyphFace,
    ElementAttr,
    ElementSilf,
    ElementSilfSub,
    ElementPass,
    ElementPseudo,
    ElementClassMap,
    ElementLookupClass,
    ElementLookup,
    ElementRange,
    ElementRuleMap,
    ElementRule,
    ElementStartState,
    ElementStateTransitions,
    ElementRow,
    ElementData,
    ElementConstraint,
    ElementConstraints,
    ElementActions,
    ElementAction,
    ElementFeatures,
    ElementFeature,
    ElementFeatureSetting,
    ElementSegment,
    ElementSlot,
    ElementText,
    ElementOpCode,
    ElementTestRule,
    ElementDoRule,
    ElementRunPass,
    ElementParams,
    ElementPush,
    ElementSubSeg,
    ElementSegCache,
    ElementSegCacheEntry,
    ElementGlyph,
    ElementPassResult,

    ElementError,
    ElementWarning,
    NumElements 
};




enum XmlTraceLogAttribute {
    AttrIndex,
    AttrVersion,
    AttrMajor,
    AttrMinor,
    AttrNum,
    AttrGlyphId,
    AttrAdvance,
    AttrAdvanceX,
    AttrAdvanceY,
    AttrAttrId,
    AttrAttrVal,
    AttrCompilerMajor,
    AttrCompilerMinor,
    AttrNumPasses,
    AttrSubPass,
    AttrPosPass,
    AttrJustPass,
    AttrBidiPass,
    AttrPreContext,
    AttrPostContext,
    AttrPseudoGlyph,
    AttrBreakWeight,
    AttrDirectionality,
    AttrNumJustLevels,
    AttrLigComp,
    AttrUserDefn,
    AttrNumLigComp,
    AttrNumCritFeatures,
    AttrNumScripts,
    AttrLBGlyph,
    AttrNumPseudo,
    AttrNumClasses,
    AttrNumLinear,
    AttrPassId,
    AttrFlags,
    AttrMaxRuleLoop,
    AttrMaxRuleContext,
    AttrMaxBackup,
    AttrNumRules,
    AttrNumRows,
    AttrNumTransition,
    AttrNumSuccess,
    AttrNumColumns,
    AttrNumRanges,
    AttrMinPrecontext,
    AttrMaxPrecontext,
    AttrFirstId,
    AttrLastId,
    AttrColId,
    AttrSuccessId,
    AttrRuleId,
    AttrContextLen,
    AttrState,
    AttrValue,
    AttrSortKey,
    AttrPrecontext,
    AttrAction,
    AttrActionCode,
    Attr0,
    Attr1,
    Attr2,
    Attr3,
    Attr4,
    Attr5,
    Attr6,
    Attr7,
    AttrLabel,
    AttrLength,
    AttrX,
    AttrY,
    AttrBefore,
    AttrAfter,
    AttrEncoding,
    AttrName,
    AttrResult,
    AttrDefault,
    AttrAccessCount,
    AttrLastAccess,
    AttrMisses,

    NumAttributes 
};

struct XmlTraceLogTag
{
public:
    XmlTraceLogTag(const char * name, uint32 flags) : mName(name), mFlags(flags) {}
    const char * mName;
    uint32 mFlags;
};

extern const XmlTraceLogTag xmlTraceLogElements[NumElements];
extern const char * xmlTraceLogAttributes[NumAttributes];

#endif 

} 
