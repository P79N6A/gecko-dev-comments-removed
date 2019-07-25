

























#include "Main.h"
#include "XmlTraceLogTags.h"


using namespace graphite2;

#ifndef DISABLE_TRACING



const XmlTraceLogTag graphite2::xmlTraceLogElements[NumElements] = {
    XmlTraceLogTag("Graphite2Log", GRLOG_ALL),
    XmlTraceLogTag("Face", GRLOG_FACE | GRLOG_PASS),
    XmlTraceLogTag("Glyphs", GRLOG_FACE),
    XmlTraceLogTag("GlyphFace", GRLOG_FACE),
    XmlTraceLogTag("Attr", GRLOG_FACE),
    XmlTraceLogTag("Silf", GRLOG_FACE | GRLOG_PASS),
    XmlTraceLogTag("SilfSub", GRLOG_FACE | GRLOG_PASS),
    XmlTraceLogTag("Pass", GRLOG_FACE | GRLOG_PASS),
    XmlTraceLogTag("Pseudo", GRLOG_FACE | GRLOG_PASS),
    XmlTraceLogTag("ClassMap", GRLOG_FACE | GRLOG_PASS),
    XmlTraceLogTag("LookupClass", GRLOG_FACE | GRLOG_PASS),
    XmlTraceLogTag("Lookup", GRLOG_FACE | GRLOG_PASS),
    XmlTraceLogTag("Range", GRLOG_PASS),
    XmlTraceLogTag("RuleMap", GRLOG_PASS),
    XmlTraceLogTag("Rule", GRLOG_PASS),
    XmlTraceLogTag("StartState", GRLOG_PASS),
    XmlTraceLogTag("StateTransitions", GRLOG_PASS),
    XmlTraceLogTag("TR", GRLOG_PASS),
    XmlTraceLogTag("TD", GRLOG_PASS),
    XmlTraceLogTag("Constraint", GRLOG_PASS),
    XmlTraceLogTag("Constraints", GRLOG_PASS),
    XmlTraceLogTag("Actions", GRLOG_PASS),
    XmlTraceLogTag("Action", GRLOG_PASS),
    XmlTraceLogTag("Features", GRLOG_PASS),
    XmlTraceLogTag("Feature", GRLOG_PASS),
    XmlTraceLogTag("FeatureSetting", GRLOG_PASS),
    XmlTraceLogTag("Segment", GRLOG_SEGMENT),
    XmlTraceLogTag("Slot", GRLOG_SEGMENT),
    XmlTraceLogTag("Text", GRLOG_SEGMENT),
    XmlTraceLogTag("OpCode", GRLOG_OPCODE),
    XmlTraceLogTag("TestRule", GRLOG_OPCODE),
    XmlTraceLogTag("DoRule", GRLOG_OPCODE),
    XmlTraceLogTag("RunPass", GRLOG_OPCODE),
    XmlTraceLogTag("Params", GRLOG_OPCODE),
    XmlTraceLogTag("Push", GRLOG_OPCODE),
    XmlTraceLogTag("SubSeg", GRLOG_SEGMENT),
    XmlTraceLogTag("SegCache", GRLOG_CACHE),
    XmlTraceLogTag("SegCacheEntry", GRLOG_CACHE),
    XmlTraceLogTag("Glyph", GRLOG_CACHE),
    XmlTraceLogTag("PassResult", GRLOG_OPCODE),

    XmlTraceLogTag("Error", GRLOG_ALL),
    XmlTraceLogTag("Warning", GRLOG_ALL)
    
};




const char * graphite2::xmlTraceLogAttributes[NumAttributes] = {
    "index",
    "version",
    "major",
    "minor",
    "num",
    "glyphId",
    "advance",
    "advanceX",
    "advanceY",
    "attrId",
    "attrVal",
    "compilerMajor",
    "compilerMinor",
    "numPasses",
    "subPass",
    "posPass",
    "justPass",
    "bidiPass",
    "preContext",
    "postContext",
    "pseudoGlyph",
    "breakWeight",
    "directionality",
    "numJustLevels",
    "numLigCompAttr",
    "numUserDefinedAttr",
    "maxNumLigComp",
    "numCriticalFeatures",
    "numScripts",
    "lineBreakglyph",
    "numPseudo",
    "numClasses",
    "numLinear",
    "passId",
    "flags",
    "maxRuleLoop",
    "maxRuleContext",
    "maxBackup",
    "numRules",
    "numRows",
    "numTransitionStates",
    "numSuccessStates",
    "numColumns",
    "numRanges",
    "minPrecontext",
    "maxPrecontext",
    "firstId",
    "lastId",
    "colId",
    "successId",
    "ruleId",
    "contextLength",
    "state",
    "value",
    "sortKey",
    "precontext",
    "action",
    "actionCode",
    "arg1",
    "arg2",
    "arg3",
    "arg4",
    "arg5",
    "arg6",
    "arg7",
    "arg8",
    "label",
    "length",
    "x",
    "y",
    "before",
    "after",
    "encoding",
    "name",
    "result",
    "default",
    "accessCount",
    "lastAccess",
    "misses"
};

#endif
