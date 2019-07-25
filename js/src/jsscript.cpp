










































#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsdbgapi.h"
#include "jsemit.h"
#include "jsfun.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsopcode.h"
#include "jsparse.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jstracer.h"
#if JS_HAS_XDR
#include "jsxdrapi.h"
#endif
#include "methodjit/MethodJIT.h"

#include "jsinterpinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace js::gc;

#if JS_HAS_XDR

enum ScriptBits {
    NoScriptRval,
    SavedCallerFun,
    HasSharps,
    StrictModeCode,
    UsesEval,
    UsesArguments
};

JSBool
js_XDRScript(JSXDRState *xdr, JSScript **scriptp, JSBool *hasMagic)
{
    JSContext *cx;
    JSScript *script, *oldscript;
    JSBool ok;
    jsbytecode *code;
    uint32 length, lineno, nslots, magic;
    uint32 natoms, nsrcnotes, ntrynotes, nobjects, nupvars, nregexps, nconsts, i;
    uint32 prologLength, version, encodedClosedCount;
    uint16 nClosedArgs = 0, nClosedVars = 0;
    JSPrincipals *principals;
    uint32 encodeable;
    JSBool filenameWasSaved;
    jssrcnote *notes, *sn;
    JSSecurityCallbacks *callbacks;
    uint32 scriptBits = 0;

    cx = xdr->cx;
    script = *scriptp;
    nsrcnotes = ntrynotes = natoms = nobjects = nupvars = nregexps = nconsts = 0;
    filenameWasSaved = JS_FALSE;
    notes = NULL;

    
    JS_ASSERT_IF(script, !JSScript::isValidOffset(script->globalsOffset));

    if (xdr->mode == JSXDR_ENCODE)
        magic = JSXDR_MAGIC_SCRIPT_CURRENT;
    if (!JS_XDRUint32(xdr, &magic))
        return JS_FALSE;
    if (magic != JSXDR_MAGIC_SCRIPT_CURRENT) {
        
        if (!hasMagic) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_SCRIPT_MAGIC);
            return JS_FALSE;
        }
        *hasMagic = JS_FALSE;
        return JS_TRUE;
    }
    if (hasMagic)
        *hasMagic = JS_TRUE;

    if (xdr->mode == JSXDR_ENCODE)
        length = script->length;
    if (!JS_XDRUint32(xdr, &length))
        return JS_FALSE;

    if (xdr->mode == JSXDR_ENCODE) {
        prologLength = script->main - script->code;
        JS_ASSERT(script->getVersion() != JSVERSION_UNKNOWN);
        version = (uint32)script->getVersion() | (script->nfixed << 16);
        lineno = (uint32)script->lineno;
        nslots = (uint32)script->nslots;
        nslots = (uint32)((script->staticLevel << 16) | script->nslots);
        natoms = (uint32)script->atomMap.length;

        
        notes = script->notes();
        for (sn = notes; !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn))
            continue;
        nsrcnotes = sn - notes;
        nsrcnotes++;            

        if (JSScript::isValidOffset(script->objectsOffset))
            nobjects = script->objects()->length;
        if (JSScript::isValidOffset(script->upvarsOffset))
            nupvars = script->upvars()->length;
        if (JSScript::isValidOffset(script->regexpsOffset))
            nregexps = script->regexps()->length;
        if (JSScript::isValidOffset(script->trynotesOffset))
            ntrynotes = script->trynotes()->length;
        if (JSScript::isValidOffset(script->constOffset))
            nconsts = script->consts()->length;

        nClosedArgs = script->nClosedArgs;
        nClosedVars = script->nClosedVars;
        encodedClosedCount = (nClosedArgs << 16) | nClosedVars;

        if (script->noScriptRval)
            scriptBits |= (1 << NoScriptRval);
        if (script->savedCallerFun)
            scriptBits |= (1 << SavedCallerFun);
        if (script->hasSharps)
            scriptBits |= (1 << HasSharps);
        if (script->strictModeCode)
            scriptBits |= (1 << StrictModeCode);
        if (script->usesEval)
            scriptBits |= (1 << UsesEval);
        if (script->usesArguments)
            scriptBits |= (1 << UsesArguments);
        JS_ASSERT(!script->compileAndGo);
        JS_ASSERT(!script->hasSingletons);
    }

    if (!JS_XDRUint32(xdr, &prologLength))
        return JS_FALSE;
    if (!JS_XDRUint32(xdr, &version))
        return JS_FALSE;

    



    if (!JS_XDRUint32(xdr, &natoms))
        return JS_FALSE;
    if (!JS_XDRUint32(xdr, &nsrcnotes))
        return JS_FALSE;
    if (!JS_XDRUint32(xdr, &ntrynotes))
        return JS_FALSE;
    if (!JS_XDRUint32(xdr, &nobjects))
        return JS_FALSE;
    if (!JS_XDRUint32(xdr, &nupvars))
        return JS_FALSE;
    if (!JS_XDRUint32(xdr, &nregexps))
        return JS_FALSE;
    if (!JS_XDRUint32(xdr, &nconsts))
        return JS_FALSE;
    if (!JS_XDRUint32(xdr, &encodedClosedCount))
        return JS_FALSE;
    if (!JS_XDRUint32(xdr, &scriptBits))
        return JS_FALSE;

    AutoScriptRooter tvr(cx, NULL);

    if (xdr->mode == JSXDR_DECODE) {
        nClosedArgs = encodedClosedCount >> 16;
        nClosedVars = encodedClosedCount & 0xFFFF;

        script = JSScript::NewScript(cx, length, nsrcnotes, natoms, nobjects, nupvars,
                                     nregexps, ntrynotes, nconsts, 0, nClosedArgs,
                                     nClosedVars);
        if (!script)
            return JS_FALSE;

        script->main += prologLength;
        script->setVersion(JSVersion(version & 0xffff));
        script->nfixed = uint16(version >> 16);

        
        notes = script->notes();
        *scriptp = script;
        tvr.setScript(script);

        if (scriptBits & (1 << NoScriptRval))
            script->noScriptRval = true;
        if (scriptBits & (1 << SavedCallerFun))
            script->savedCallerFun = true;
        if (scriptBits & (1 << HasSharps))
            script->hasSharps = true;
        if (scriptBits & (1 << StrictModeCode))
            script->strictModeCode = true;
        if (scriptBits & (1 << UsesEval))
            script->usesEval = true;
        if (scriptBits & (1 << UsesArguments))
            script->usesArguments = true;
    }

    



    oldscript = xdr->script;
    code = script->code;
    if (xdr->mode == JSXDR_ENCODE) {
        code = js_UntrapScriptCode(cx, script);
        if (!code)
            goto error;
    }

    xdr->script = script;
    ok = JS_XDRBytes(xdr, (char *) code, length * sizeof(jsbytecode));

    if (code != script->code)
        cx->free(code);

    if (!ok)
        goto error;

    if (!JS_XDRBytes(xdr, (char *)notes, nsrcnotes * sizeof(jssrcnote)) ||
        !JS_XDRCStringOrNull(xdr, (char **)&script->filename) ||
        !JS_XDRUint32(xdr, &lineno) ||
        !JS_XDRUint32(xdr, &nslots)) {
        goto error;
    }

    callbacks = JS_GetSecurityCallbacks(cx);
    if (xdr->mode == JSXDR_ENCODE) {
        principals = script->principals;
        encodeable = callbacks && callbacks->principalsTranscoder;
        if (!JS_XDRUint32(xdr, &encodeable))
            goto error;
        if (encodeable &&
            !callbacks->principalsTranscoder(xdr, &principals)) {
            goto error;
        }
    } else {
        if (!JS_XDRUint32(xdr, &encodeable))
            goto error;
        if (encodeable) {
            if (!(callbacks && callbacks->principalsTranscoder)) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_CANT_DECODE_PRINCIPALS);
                goto error;
            }
            if (!callbacks->principalsTranscoder(xdr, &principals))
                goto error;
            script->principals = principals;
        }
    }

    if (xdr->mode == JSXDR_DECODE) {
        const char *filename = script->filename;
        if (filename) {
            filename = js_SaveScriptFilename(cx, filename);
            if (!filename)
                goto error;
            cx->free((void *) script->filename);
            script->filename = filename;
            filenameWasSaved = JS_TRUE;
        }
        script->lineno = (uintN)lineno;
        script->nslots = (uint16)nslots;
        script->staticLevel = (uint16)(nslots >> 16);

    }

    for (i = 0; i != natoms; ++i) {
        if (!js_XDRAtom(xdr, &script->atomMap.vector[i]))
            goto error;
    }

    





    for (i = 0; i != nobjects; ++i) {
        JSObject **objp = &script->objects()->vector[i];
        uint32 isBlock;
        if (xdr->mode == JSXDR_ENCODE) {
            Class *clasp = (*objp)->getClass();
            JS_ASSERT(clasp == &js_FunctionClass ||
                      clasp == &js_BlockClass);
            isBlock = (clasp == &js_BlockClass) ? 1 : 0;
        }
        if (!JS_XDRUint32(xdr, &isBlock))
            goto error;
        if (isBlock == 0) {
            if (!js_XDRFunctionObject(xdr, objp))
                goto error;
        } else {
            JS_ASSERT(isBlock == 1);
            if (!js_XDRBlockObject(xdr, objp))
                goto error;
        }
    }
    for (i = 0; i != nupvars; ++i) {
        if (!JS_XDRUint32(xdr, reinterpret_cast<uint32 *>(&script->upvars()->vector[i])))
            goto error;
    }
    for (i = 0; i != nregexps; ++i) {
        if (!js_XDRRegExpObject(xdr, &script->regexps()->vector[i]))
            goto error;
    }
    for (i = 0; i != nClosedArgs; ++i) {
        if (!JS_XDRUint32(xdr, &script->closedSlots[i]))
            goto error;
    }
    for (i = 0; i != nClosedVars; ++i) {
        if (!JS_XDRUint32(xdr, &script->closedSlots[nClosedArgs + i]))
            goto error;
    }

    if (ntrynotes != 0) {
        



        JSTryNote *tn, *tnfirst;
        uint32 kindAndDepth;
        JS_STATIC_ASSERT(sizeof(tn->kind) == sizeof(uint8));
        JS_STATIC_ASSERT(sizeof(tn->stackDepth) == sizeof(uint16));

        tnfirst = script->trynotes()->vector;
        JS_ASSERT(script->trynotes()->length == ntrynotes);
        tn = tnfirst + ntrynotes;
        do {
            --tn;
            if (xdr->mode == JSXDR_ENCODE) {
                kindAndDepth = ((uint32)tn->kind << 16)
                               | (uint32)tn->stackDepth;
            }
            if (!JS_XDRUint32(xdr, &kindAndDepth) ||
                !JS_XDRUint32(xdr, &tn->start) ||
                !JS_XDRUint32(xdr, &tn->length)) {
                goto error;
            }
            if (xdr->mode == JSXDR_DECODE) {
                tn->kind = (uint8)(kindAndDepth >> 16);
                tn->stackDepth = (uint16)kindAndDepth;
            }
        } while (tn != tnfirst);
    }

    for (i = 0; i != nconsts; ++i) {
        if (!JS_XDRValue(xdr, Jsvalify(&script->consts()->vector[i])))
            goto error;
    }

    xdr->script = oldscript;
    return JS_TRUE;

  error:
    if (xdr->mode == JSXDR_DECODE) {
        if (script->filename && !filenameWasSaved) {
            cx->free((void *) script->filename);
            script->filename = NULL;
        }
        js_DestroyScript(cx, script);
        *scriptp = NULL;
    }
    xdr->script = oldscript;
    return JS_FALSE;
}

#endif 

static void
script_finalize(JSContext *cx, JSObject *obj)
{
    JSScript *script = (JSScript *) obj->getPrivate();
    if (script)
        js_DestroyScriptFromGC(cx, script);
}

static void
script_trace(JSTracer *trc, JSObject *obj)
{
    JSScript *script = (JSScript *) obj->getPrivate();
    if (script)
        js_TraceScript(trc, script);
}

Class js_ScriptClass = {
    "Script",
    JSCLASS_HAS_PRIVATE |
    JSCLASS_MARK_IS_TRACE | JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    EnumerateStub,
    ResolveStub,
    ConvertStub,
    script_finalize,
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    JS_CLASS_TRACE(script_trace)
};




static int
js_compare_strings(const void *k1, const void *k2)
{
    return strcmp((const char *) k1, (const char *) k2) == 0;
}


typedef struct ScriptFilenameEntry {
    JSHashEntry         *next;          
    JSHashNumber        keyHash;        
    const void          *key;           
    uint32              flags;          
    JSPackedBool        mark;           
    char                filename[3];    
} ScriptFilenameEntry;

static void *
js_alloc_table_space(void *priv, size_t size)
{
    return js_malloc(size);
}

static void
js_free_table_space(void *priv, void *item, size_t size)
{
    js_free(item);
}

static JSHashEntry *
js_alloc_sftbl_entry(void *priv, const void *key)
{
    size_t nbytes = offsetof(ScriptFilenameEntry, filename) +
                    strlen((const char *) key) + 1;

    return (JSHashEntry *) js_malloc(JS_MAX(nbytes, sizeof(JSHashEntry)));
}

static void
js_free_sftbl_entry(void *priv, JSHashEntry *he, uintN flag)
{
    if (flag != HT_FREE_ENTRY)
        return;
    js_free(he);
}

static JSHashAllocOps sftbl_alloc_ops = {
    js_alloc_table_space,   js_free_table_space,
    js_alloc_sftbl_entry,   js_free_sftbl_entry
};

static void
FinishRuntimeScriptState(JSRuntime *rt)
{
    if (rt->scriptFilenameTable) {
        JS_HashTableDestroy(rt->scriptFilenameTable);
        rt->scriptFilenameTable = NULL;
    }
#ifdef JS_THREADSAFE
    if (rt->scriptFilenameTableLock) {
        JS_DESTROY_LOCK(rt->scriptFilenameTableLock);
        rt->scriptFilenameTableLock = NULL;
    }
#endif
}

JSBool
js_InitRuntimeScriptState(JSRuntime *rt)
{
#ifdef JS_THREADSAFE
    JS_ASSERT(!rt->scriptFilenameTableLock);
    rt->scriptFilenameTableLock = JS_NEW_LOCK();
    if (!rt->scriptFilenameTableLock)
        return JS_FALSE;
#endif
    JS_ASSERT(!rt->scriptFilenameTable);
    rt->scriptFilenameTable =
        JS_NewHashTable(16, JS_HashString, js_compare_strings, NULL,
                        &sftbl_alloc_ops, NULL);
    if (!rt->scriptFilenameTable) {
        FinishRuntimeScriptState(rt);       
        return JS_FALSE;
    }
    JS_INIT_CLIST(&rt->scriptFilenamePrefixes);
    return JS_TRUE;
}

typedef struct ScriptFilenamePrefix {
    JSCList     links;      
    const char  *name;      
    size_t      length;     
    uint32      flags;      
} ScriptFilenamePrefix;

void
js_FreeRuntimeScriptState(JSRuntime *rt)
{
    if (!rt->scriptFilenameTable)
        return;

    while (!JS_CLIST_IS_EMPTY(&rt->scriptFilenamePrefixes)) {
        ScriptFilenamePrefix *sfp = (ScriptFilenamePrefix *)
                                    rt->scriptFilenamePrefixes.next;
        JS_REMOVE_LINK(&sfp->links);
        js_free(sfp);
    }
    FinishRuntimeScriptState(rt);
}

#ifdef DEBUG_brendan
#define DEBUG_SFTBL
#endif
#ifdef DEBUG_SFTBL
size_t sftbl_savings = 0;
#endif

static ScriptFilenameEntry *
SaveScriptFilename(JSRuntime *rt, const char *filename, uint32 flags)
{
    JSHashTable *table;
    JSHashNumber hash;
    JSHashEntry **hep;
    ScriptFilenameEntry *sfe;
    size_t length;
    JSCList *head, *link;
    ScriptFilenamePrefix *sfp;

    table = rt->scriptFilenameTable;
    hash = JS_HashString(filename);
    hep = JS_HashTableRawLookup(table, hash, filename);
    sfe = (ScriptFilenameEntry *) *hep;
#ifdef DEBUG_SFTBL
    if (sfe)
        sftbl_savings += strlen(sfe->filename);
#endif

    if (!sfe) {
        sfe = (ScriptFilenameEntry *)
              JS_HashTableRawAdd(table, hep, hash, filename, NULL);
        if (!sfe)
            return NULL;
        sfe->key = strcpy(sfe->filename, filename);
        sfe->flags = 0;
        sfe->mark = JS_FALSE;
    }

    
    if (flags != 0) {
        
        sfp = NULL;
        length = strlen(filename);
        for (head = link = &rt->scriptFilenamePrefixes;
             link->next != head;
             link = link->next) {
            
            sfp = (ScriptFilenamePrefix *) link->next;
            if (!strcmp(sfp->name, filename))
                break;
            if (sfp->length <= length) {
                sfp = NULL;
                break;
            }
            sfp = NULL;
        }

        if (!sfp) {
            
            sfp = (ScriptFilenamePrefix *) js_malloc(sizeof(ScriptFilenamePrefix));
            if (!sfp)
                return NULL;
            JS_INSERT_AFTER(&sfp->links, link);
            sfp->name = sfe->filename;
            sfp->length = length;
            sfp->flags = 0;
        }

        




        sfe->flags |= flags;
        sfp->flags |= flags;
    }

#ifdef DEBUG
    if (rt->functionMeterFilename) {
        size_t len = strlen(sfe->filename);
        if (len >= sizeof rt->lastScriptFilename)
            len = sizeof rt->lastScriptFilename - 1;
        memcpy(rt->lastScriptFilename, sfe->filename, len);
        rt->lastScriptFilename[len] = '\0';
    }
#endif

    return sfe;
}

const char *
js_SaveScriptFilename(JSContext *cx, const char *filename)
{
    JSRuntime *rt;
    ScriptFilenameEntry *sfe;
    JSCList *head, *link;
    ScriptFilenamePrefix *sfp;

    rt = cx->runtime;
    JS_ACQUIRE_LOCK(rt->scriptFilenameTableLock);
    sfe = SaveScriptFilename(rt, filename, 0);
    if (!sfe) {
        JS_RELEASE_LOCK(rt->scriptFilenameTableLock);
        JS_ReportOutOfMemory(cx);
        return NULL;
    }

    




    for (head = &rt->scriptFilenamePrefixes, link = head->next;
         link != head;
         link = link->next) {
        sfp = (ScriptFilenamePrefix *) link;
        if (!strncmp(sfp->name, filename, sfp->length)) {
            sfe->flags |= sfp->flags;
            break;
        }
    }
    JS_RELEASE_LOCK(rt->scriptFilenameTableLock);
    return sfe->filename;
}

const char *
js_SaveScriptFilenameRT(JSRuntime *rt, const char *filename, uint32 flags)
{
    ScriptFilenameEntry *sfe;

    
    if (!rt->scriptFilenameTable && !js_InitRuntimeScriptState(rt))
        return NULL;

    JS_ACQUIRE_LOCK(rt->scriptFilenameTableLock);
    sfe = SaveScriptFilename(rt, filename, flags);
    JS_RELEASE_LOCK(rt->scriptFilenameTableLock);
    if (!sfe)
        return NULL;

    return sfe->filename;
}




#define FILENAME_TO_SFE(fn) \
    ((ScriptFilenameEntry *) ((fn) - offsetof(ScriptFilenameEntry, filename)))







#define ASSERT_VALID_SFE(sfe)   JS_ASSERT((sfe)->key == (sfe)->filename)

uint32
js_GetScriptFilenameFlags(const char *filename)
{
    ScriptFilenameEntry *sfe;

    sfe = FILENAME_TO_SFE(filename);
    ASSERT_VALID_SFE(sfe);
    return sfe->flags;
}

void
js_MarkScriptFilename(const char *filename)
{
    ScriptFilenameEntry *sfe;

    sfe = FILENAME_TO_SFE(filename);
    ASSERT_VALID_SFE(sfe);
    sfe->mark = JS_TRUE;
}

static intN
js_script_filename_marker(JSHashEntry *he, intN i, void *arg)
{
    ScriptFilenameEntry *sfe = (ScriptFilenameEntry *) he;

    sfe->mark = JS_TRUE;
    return HT_ENUMERATE_NEXT;
}

void
js_MarkScriptFilenames(JSRuntime *rt)
{
    JSCList *head, *link;
    ScriptFilenamePrefix *sfp;

    if (!rt->scriptFilenameTable)
        return;

    if (rt->gcKeepAtoms) {
        JS_HashTableEnumerateEntries(rt->scriptFilenameTable,
                                     js_script_filename_marker,
                                     rt);
    }
    for (head = &rt->scriptFilenamePrefixes, link = head->next;
         link != head;
         link = link->next) {
        sfp = (ScriptFilenamePrefix *) link;
        js_MarkScriptFilename(sfp->name);
    }
}

static intN
js_script_filename_sweeper(JSHashEntry *he, intN i, void *arg)
{
    ScriptFilenameEntry *sfe = (ScriptFilenameEntry *) he;

    if (!sfe->mark)
        return HT_ENUMERATE_REMOVE;
    sfe->mark = JS_FALSE;
    return HT_ENUMERATE_NEXT;
}

void
js_SweepScriptFilenames(JSRuntime *rt)
{
    if (!rt->scriptFilenameTable)
        return;

    



    JS_HashTableEnumerateEntries(rt->scriptFilenameTable,
                                 js_script_filename_sweeper,
                                 rt);
#ifdef DEBUG_notme
#ifdef DEBUG_SFTBL
    printf("script filename table savings so far: %u\n", sftbl_savings);
#endif
#endif
}






























JS_STATIC_ASSERT(sizeof(JSScript) % sizeof(void *) == 0);
JS_STATIC_ASSERT(sizeof(JSObjectArray) % sizeof(void *) == 0);
JS_STATIC_ASSERT(sizeof(JSTryNoteArray) == sizeof(JSObjectArray));
JS_STATIC_ASSERT(sizeof(JSAtom *) == sizeof(JSObject *));
JS_STATIC_ASSERT(sizeof(JSObject *) % sizeof(uint32) == 0);
JS_STATIC_ASSERT(sizeof(JSTryNote) == 3 * sizeof(uint32));
JS_STATIC_ASSERT(sizeof(uint32) % sizeof(jsbytecode) == 0);
JS_STATIC_ASSERT(sizeof(jsbytecode) % sizeof(jssrcnote) == 0);





JS_STATIC_ASSERT(sizeof(JSScript) + 2 * sizeof(JSObjectArray) +
                 sizeof(JSUpvarArray) < JS_BIT(8));

JSScript *
JSScript::NewScript(JSContext *cx, uint32 length, uint32 nsrcnotes, uint32 natoms,
                    uint32 nobjects, uint32 nupvars, uint32 nregexps,
                    uint32 ntrynotes, uint32 nconsts, uint32 nglobals,
                    uint16 nClosedArgs, uint16 nClosedVars)
{
    size_t size, vectorSize;
    JSScript *script;
    uint8 *cursor;
    unsigned constPadding = 0;

    uint32 totalClosed = nClosedArgs + nClosedVars;

    size = sizeof(JSScript) +
           sizeof(JSAtom *) * natoms;
    
    if (nobjects != 0)
        size += sizeof(JSObjectArray) + nobjects * sizeof(JSObject *);
    if (nupvars != 0)
        size += sizeof(JSUpvarArray) + nupvars * sizeof(uint32);
    if (nregexps != 0)
        size += sizeof(JSObjectArray) + nregexps * sizeof(JSObject *);
    if (ntrynotes != 0)
        size += sizeof(JSTryNoteArray) + ntrynotes * sizeof(JSTryNote);
    if (nglobals != 0)
        size += sizeof(GlobalSlotArray) + nglobals * sizeof(GlobalSlotArray::Entry);
    if (totalClosed != 0)
        size += totalClosed * sizeof(uint32);

    if (nconsts != 0) {
        size += sizeof(JSConstArray);
        



        constPadding = (8 - (size % 8)) % 8;
        size += constPadding + nconsts * sizeof(Value);
    }

    size += length * sizeof(jsbytecode) +
            nsrcnotes * sizeof(jssrcnote);

    script = (JSScript *) cx->malloc(size);
    if (!script)
        return NULL;

    PodZero(script);
    script->length = length;
    script->setVersion(cx->findVersion());

    uint8 *scriptEnd = reinterpret_cast<uint8 *>(script + 1);

    cursor = (uint8 *)script + sizeof(JSScript);
    if (nobjects != 0) {
        script->objectsOffset = (uint8)(cursor - scriptEnd);
        cursor += sizeof(JSObjectArray);
    } else {
        script->objectsOffset = JSScript::INVALID_OFFSET;
    }
    if (nupvars != 0) {
        script->upvarsOffset = (uint8)(cursor - scriptEnd);
        cursor += sizeof(JSUpvarArray);
    } else {
        script->upvarsOffset = JSScript::INVALID_OFFSET;
    }
    if (nregexps != 0) {
        script->regexpsOffset = (uint8)(cursor - scriptEnd);
        cursor += sizeof(JSObjectArray);
    } else {
        script->regexpsOffset = JSScript::INVALID_OFFSET;
    }
    if (ntrynotes != 0) {
        script->trynotesOffset = (uint8)(cursor - scriptEnd);
        cursor += sizeof(JSTryNoteArray);
    } else {
        script->trynotesOffset = JSScript::INVALID_OFFSET;
    }
    if (nglobals != 0) {
        script->globalsOffset = (uint8)(cursor - scriptEnd);
        cursor += sizeof(GlobalSlotArray);
    } else {
        script->globalsOffset = JSScript::INVALID_OFFSET;
    }
    JS_ASSERT((cursor - (uint8 *)script) < 0xFF);
    if (nconsts != 0) {
        script->constOffset = (uint8)(cursor - scriptEnd);
        cursor += sizeof(JSConstArray);
    } else {
        script->constOffset = JSScript::INVALID_OFFSET;
    }

    JS_STATIC_ASSERT(sizeof(JSObjectArray) +
                     sizeof(JSUpvarArray) +
                     sizeof(JSObjectArray) +
                     sizeof(JSTryNoteArray) +
                     sizeof(GlobalSlotArray) < 0xFF);

    if (natoms != 0) {
        script->atomMap.length = natoms;
        script->atomMap.vector = (JSAtom **)cursor;
        vectorSize = natoms * sizeof(script->atomMap.vector[0]);

        



        memset(cursor, 0, vectorSize);
        cursor += vectorSize;
    }

    if (nobjects != 0) {
        script->objects()->length = nobjects;
        script->objects()->vector = (JSObject **)cursor;
        vectorSize = nobjects * sizeof(script->objects()->vector[0]);
        memset(cursor, 0, vectorSize);
        cursor += vectorSize;
    }

    if (nregexps != 0) {
        script->regexps()->length = nregexps;
        script->regexps()->vector = (JSObject **)cursor;
        vectorSize = nregexps * sizeof(script->regexps()->vector[0]);
        memset(cursor, 0, vectorSize);
        cursor += vectorSize;
    }

    if (ntrynotes != 0) {
        script->trynotes()->length = ntrynotes;
        script->trynotes()->vector = (JSTryNote *)cursor;
        vectorSize = ntrynotes * sizeof(script->trynotes()->vector[0]);
#ifdef DEBUG
        memset(cursor, 0, vectorSize);
#endif
        cursor += vectorSize;
    }

    if (nglobals != 0) {
        script->globals()->length = nglobals;
        script->globals()->vector = (GlobalSlotArray::Entry *)cursor;
        vectorSize = nglobals * sizeof(script->globals()->vector[0]);
        cursor += vectorSize;
    }

    if (totalClosed != 0) {
        script->nClosedArgs = nClosedArgs;
        script->nClosedVars = nClosedVars;
        script->closedSlots = (uint32 *)cursor;
        cursor += totalClosed * sizeof(uint32);
    }

    



    if (nupvars != 0) {
        script->upvars()->length = nupvars;
        script->upvars()->vector = reinterpret_cast<UpvarCookie *>(cursor);
        vectorSize = nupvars * sizeof(script->upvars()->vector[0]);
        memset(cursor, 0, vectorSize);
        cursor += vectorSize;
    }

    
    if (nconsts != 0) {
        cursor += constPadding;
        script->consts()->length = nconsts;
        script->consts()->vector = (Value *)cursor;
        JS_ASSERT((size_t)cursor % sizeof(double) == 0);
        vectorSize = nconsts * sizeof(script->consts()->vector[0]);
        memset(cursor, 0, vectorSize);
        cursor += vectorSize;
    }

    script->code = script->main = (jsbytecode *)cursor;
    JS_ASSERT(cursor +
              length * sizeof(jsbytecode) +
              nsrcnotes * sizeof(jssrcnote) ==
              (uint8 *)script + size);

    script->compartment = cx->compartment;
#ifdef CHECK_SCRIPT_OWNER
    script->owner = cx->thread;
#endif

    JS_APPEND_LINK(&script->links, &cx->compartment->scripts);
    return script;
}

JSScript *
JSScript::NewScriptFromCG(JSContext *cx, JSCodeGenerator *cg)
{
    uint32 mainLength, prologLength, nsrcnotes, nfixed;
    JSScript *script;
    const char *filename;
    JSFunction *fun;

    
    JS_ASSERT(cg->atomList.count <= INDEX_LIMIT);
    JS_ASSERT(cg->objectList.length <= INDEX_LIMIT);
    JS_ASSERT(cg->regexpList.length <= INDEX_LIMIT);

    mainLength = CG_OFFSET(cg);
    prologLength = CG_PROLOG_OFFSET(cg);

    CG_COUNT_FINAL_SRCNOTES(cg, nsrcnotes);
    uint16 nClosedArgs = uint16(cg->closedArgs.length());
    JS_ASSERT(nClosedArgs == cg->closedArgs.length());
    uint16 nClosedVars = uint16(cg->closedVars.length());
    JS_ASSERT(nClosedVars == cg->closedVars.length());
    script = NewScript(cx, prologLength + mainLength, nsrcnotes,
                       cg->atomList.count, cg->objectList.length,
                       cg->upvarList.count, cg->regexpList.length,
                       cg->ntrynotes, cg->constList.length(),
                       cg->globalUses.length(), nClosedArgs, nClosedVars);
    if (!script)
        return NULL;

    
    script->main += prologLength;
    memcpy(script->code, CG_PROLOG_BASE(cg), prologLength * sizeof(jsbytecode));
    memcpy(script->main, CG_BASE(cg), mainLength * sizeof(jsbytecode));
    nfixed = cg->inFunction()
             ? cg->fun()->u.i.nvars
             : cg->sharpSlots();
    JS_ASSERT(nfixed < SLOTNO_LIMIT);
    script->nfixed = (uint16) nfixed;
    js_InitAtomMap(cx, &script->atomMap, &cg->atomList);

    filename = cg->parser->tokenStream.getFilename();
    if (filename) {
        script->filename = js_SaveScriptFilename(cx, filename);
        if (!script->filename)
            goto bad;
    }
    script->lineno = cg->firstLine;
    if (script->nfixed + cg->maxStackDepth >= JS_BIT(16)) {
        ReportCompileErrorNumber(cx, CG_TS(cg), NULL, JSREPORT_ERROR, JSMSG_NEED_DIET, "script");
        goto bad;
    }
    script->nslots = script->nfixed + cg->maxStackDepth;
    script->staticLevel = uint16(cg->staticLevel);
    script->principals = cg->parser->principals;
    if (script->principals)
        JSPRINCIPALS_HOLD(cx, script->principals);

    if (!js_FinishTakingSrcNotes(cx, cg, script->notes()))
        goto bad;
    if (cg->ntrynotes != 0)
        js_FinishTakingTryNotes(cg, script->trynotes());
    if (cg->objectList.length != 0)
        cg->objectList.finish(script->objects());
    if (cg->regexpList.length != 0)
        cg->regexpList.finish(script->regexps());
    if (cg->constList.length() != 0)
        cg->constList.finish(script->consts());
    if (cg->flags & TCF_NO_SCRIPT_RVAL)
        script->noScriptRval = true;
    if (cg->hasSharps())
        script->hasSharps = true;
    if (cg->flags & TCF_STRICT_MODE_CODE)
        script->strictModeCode = true;
    if (cg->flags & TCF_COMPILE_N_GO)
        script->compileAndGo = true;
    if (cg->callsEval())
        script->usesEval = true;
    if (cg->flags & TCF_FUN_USES_ARGUMENTS)
        script->usesArguments = true;
    if (cg->flags & TCF_HAS_SINGLETONS)
        script->hasSingletons = true;

    if (cg->upvarList.count != 0) {
        JS_ASSERT(cg->upvarList.count <= cg->upvarMap.length);
        memcpy(script->upvars()->vector, cg->upvarMap.vector,
               cg->upvarList.count * sizeof(uint32));
        cg->upvarList.clear();
        cx->free(cg->upvarMap.vector);
        cg->upvarMap.vector = NULL;
    }

    if (cg->globalUses.length()) {
        memcpy(script->globals()->vector, &cg->globalUses[0],
               cg->globalUses.length() * sizeof(GlobalSlotArray::Entry));
    }

    if (script->nClosedArgs)
        memcpy(script->closedSlots, &cg->closedArgs[0], script->nClosedArgs * sizeof(uint32));
    if (script->nClosedVars) {
        memcpy(&script->closedSlots[script->nClosedArgs], &cg->closedVars[0],
               script->nClosedVars * sizeof(uint32));
    }

    



    fun = NULL;
    if (cg->inFunction()) {
        fun = cg->fun();
        JS_ASSERT(FUN_INTERPRETED(fun) && !FUN_SCRIPT(fun));
        if (JSScript::isValidOffset(script->upvarsOffset))
            JS_ASSERT(script->upvars()->length == fun->u.i.nupvars);
        else
            fun->u.i.nupvars = 0;

        fun->freezeLocalNames(cx);
        fun->u.i.script = script;
#ifdef CHECK_SCRIPT_OWNER
        script->owner = NULL;
#endif
        if (cg->flags & TCF_FUN_HEAVYWEIGHT)
            fun->flags |= JSFUN_HEAVYWEIGHT;
    }

    
    js_CallNewScriptHook(cx, script, fun);
#ifdef DEBUG
    {
        jsrefcount newEmptyLive, newLive, newTotal;
        if (script->isEmpty()) {
            newEmptyLive = JS_RUNTIME_METER(cx->runtime, liveEmptyScripts);
            newLive = cx->runtime->liveScripts;
            newTotal =
                JS_RUNTIME_METER(cx->runtime, totalEmptyScripts) + cx->runtime->totalScripts;
        } else {
            newEmptyLive = cx->runtime->liveEmptyScripts;
            newLive = JS_RUNTIME_METER(cx->runtime, liveScripts);
            newTotal =
                cx->runtime->totalEmptyScripts + JS_RUNTIME_METER(cx->runtime, totalScripts);
        }

        jsrefcount oldHigh = cx->runtime->highWaterLiveScripts;
        if (newEmptyLive + newLive > oldHigh) {
            JS_ATOMIC_SET(&cx->runtime->highWaterLiveScripts, newEmptyLive + newLive);
            if (getenv("JS_DUMP_LIVE_SCRIPTS")) {
                fprintf(stderr, "high water script count: %d empty, %d not (total %d)\n",
                        newEmptyLive, newLive, newTotal);
            }
        }
    }
#endif

    return script;

bad:
    js_DestroyScript(cx, script);
    return NULL;
}

JS_FRIEND_API(void)
js_CallNewScriptHook(JSContext *cx, JSScript *script, JSFunction *fun)
{
    JSNewScriptHook hook;

    hook = cx->debugHooks->newScriptHook;
    if (hook) {
        AutoKeepAtoms keep(cx->runtime);
        hook(cx, script->filename, script->lineno, script, fun,
             cx->debugHooks->newScriptHookData);
    }
}

JS_FRIEND_API(void)
js_CallDestroyScriptHook(JSContext *cx, JSScript *script)
{
    JSDestroyScriptHook hook;

    hook = cx->debugHooks->destroyScriptHook;
    if (hook)
        hook(cx, script, cx->debugHooks->destroyScriptHookData);
}

static void
DestroyScript(JSContext *cx, JSScript *script)
{
#ifdef DEBUG
    if (script->isEmpty())
        JS_RUNTIME_UNMETER(cx->runtime, liveEmptyScripts);
    else
        JS_RUNTIME_UNMETER(cx->runtime, liveScripts);
#endif

    js_CallDestroyScriptHook(cx, script);
    JS_ClearScriptTraps(cx, script);

    if (script->principals)
        JSPRINCIPALS_DROP(cx, script->principals);

    if (JS_GSN_CACHE(cx).code == script->code)
        JS_PURGE_GSN_CACHE(cx);

    






















#ifdef CHECK_SCRIPT_OWNER
    JS_ASSERT_IF(cx->runtime->gcRunning, !script->owner);
#endif

    
    if (!cx->runtime->gcRunning) {
        JSStackFrame *fp = js_GetTopStackFrame(cx);

        if (!(fp && fp->isEvalFrame())) {
            JS_PROPERTY_CACHE(cx).purgeForScript(script);

#ifdef CHECK_SCRIPT_OWNER
            JS_ASSERT(script->owner == cx->thread);
#endif
        }
    }

#ifdef JS_TRACER
    PurgeScriptFragments(&script->compartment->traceMonitor, script);
#endif

#if defined(JS_METHODJIT)
    mjit::ReleaseScriptCode(cx, script);
#endif
    JS_REMOVE_LINK(&script->links);

    cx->free(script);
}

void
js_DestroyScript(JSContext *cx, JSScript *script)
{
    JS_ASSERT(!cx->runtime->gcRunning);
    DestroyScript(cx, script);
}

void
js_DestroyScriptFromGC(JSContext *cx, JSScript *script)
{
    JS_ASSERT(cx->runtime->gcRunning);
    DestroyScript(cx, script);
}

void
js_TraceScript(JSTracer *trc, JSScript *script)
{
    JSAtomMap *map = &script->atomMap;
    MarkAtomRange(trc, map->length, map->vector, "atomMap");

    if (JSScript::isValidOffset(script->objectsOffset)) {
        JSObjectArray *objarray = script->objects();
        uintN i = objarray->length;
        do {
            --i;
            if (objarray->vector[i]) {
                JS_SET_TRACING_INDEX(trc, "objects", i);
                Mark(trc, objarray->vector[i]);
            }
        } while (i != 0);
    }

    if (JSScript::isValidOffset(script->regexpsOffset)) {
        JSObjectArray *objarray = script->regexps();
        uintN i = objarray->length;
        do {
            --i;
            if (objarray->vector[i]) {
                JS_SET_TRACING_INDEX(trc, "regexps", i);
                Mark(trc, objarray->vector[i]);
            }
        } while (i != 0);
    }

    if (JSScript::isValidOffset(script->constOffset)) {
        JSConstArray *constarray = script->consts();
        MarkValueRange(trc, constarray->length, constarray->vector, "consts");
    }

    if (script->u.object) {
        JS_SET_TRACING_NAME(trc, "object");
        Mark(trc, script->u.object);
    }

    if (IS_GC_MARKING_TRACER(trc) && script->filename)
        js_MarkScriptFilename(script->filename);
}

JSBool
js_NewScriptObject(JSContext *cx, JSScript *script)
{
    AutoScriptRooter root(cx, script);

    JS_ASSERT(!script->u.object);

    JSObject *obj = NewNonFunction<WithProto::Class>(cx, &js_ScriptClass, NULL, NULL);
    if (!obj)
        return JS_FALSE;
    obj->setPrivate(script);
    script->u.object = obj;

    



    obj->clearProto();

#ifdef CHECK_SCRIPT_OWNER
    script->owner = NULL;
#endif

    return JS_TRUE;
}

typedef struct GSNCacheEntry {
    JSDHashEntryHdr     hdr;
    jsbytecode          *pc;
    jssrcnote           *sn;
} GSNCacheEntry;

#define GSN_CACHE_THRESHOLD     100

void
js_PurgeGSNCache(JSGSNCache *cache)
{
    cache->code = NULL;
    if (cache->table.ops) {
        JS_DHashTableFinish(&cache->table);
        cache->table.ops = NULL;
    }
    GSN_CACHE_METER(cache, purges);
}

jssrcnote *
js_GetSrcNoteCached(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    ptrdiff_t target, offset;
    GSNCacheEntry *entry;
    jssrcnote *sn, *result;
    uintN nsrcnotes;


    target = pc - script->code;
    if ((uint32)target >= script->length)
        return NULL;

    if (JS_GSN_CACHE(cx).code == script->code) {
        JS_METER_GSN_CACHE(cx, hits);
        entry = (GSNCacheEntry *)
                JS_DHashTableOperate(&JS_GSN_CACHE(cx).table, pc,
                                     JS_DHASH_LOOKUP);
        return entry->sn;
    }

    JS_METER_GSN_CACHE(cx, misses);
    offset = 0;
    for (sn = script->notes(); ; sn = SN_NEXT(sn)) {
        if (SN_IS_TERMINATOR(sn)) {
            result = NULL;
            break;
        }
        offset += SN_DELTA(sn);
        if (offset == target && SN_IS_GETTABLE(sn)) {
            result = sn;
            break;
        }
    }

    if (JS_GSN_CACHE(cx).code != script->code &&
        script->length >= GSN_CACHE_THRESHOLD) {
        JS_PURGE_GSN_CACHE(cx);
        nsrcnotes = 0;
        for (sn = script->notes(); !SN_IS_TERMINATOR(sn);
             sn = SN_NEXT(sn)) {
            if (SN_IS_GETTABLE(sn))
                ++nsrcnotes;
        }
        if (!JS_DHashTableInit(&JS_GSN_CACHE(cx).table, JS_DHashGetStubOps(),
                               NULL, sizeof(GSNCacheEntry),
                               JS_DHASH_DEFAULT_CAPACITY(nsrcnotes))) {
            JS_GSN_CACHE(cx).table.ops = NULL;
        } else {
            pc = script->code;
            for (sn = script->notes(); !SN_IS_TERMINATOR(sn);
                 sn = SN_NEXT(sn)) {
                pc += SN_DELTA(sn);
                if (SN_IS_GETTABLE(sn)) {
                    entry = (GSNCacheEntry *)
                            JS_DHashTableOperate(&JS_GSN_CACHE(cx).table, pc,
                                                 JS_DHASH_ADD);
                    entry->pc = pc;
                    entry->sn = sn;
                }
            }
            JS_GSN_CACHE(cx).code = script->code;
            JS_METER_GSN_CACHE(cx, fills);
        }
    }

    return result;
}

uintN
js_FramePCToLineNumber(JSContext *cx, JSStackFrame *fp)
{
    return js_PCToLineNumber(cx, fp->script(),
                             fp->hasImacropc() ? fp->imacropc() : fp->pc(cx));
}

uintN
js_PCToLineNumber(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    JSOp op;
    JSFunction *fun;
    uintN lineno;
    ptrdiff_t offset, target;
    jssrcnote *sn;
    JSSrcNoteType type;

    
    if (!pc)
        return 0;

    



    op = js_GetOpcode(cx, script, pc);
    if (js_CodeSpec[op].format & JOF_INDEXBASE)
        pc += js_CodeSpec[op].length;
    if (*pc == JSOP_DEFFUN) {
        GET_FUNCTION_FROM_BYTECODE(script, pc, 0, fun);
        return fun->u.i.script->lineno;
    }

    




    lineno = script->lineno;
    offset = 0;
    target = pc - script->code;
    for (sn = script->notes(); !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        offset += SN_DELTA(sn);
        type = (JSSrcNoteType) SN_TYPE(sn);
        if (type == SRC_SETLINE) {
            if (offset <= target)
                lineno = (uintN) js_GetSrcNoteOffset(sn, 0);
        } else if (type == SRC_NEWLINE) {
            if (offset <= target)
                lineno++;
        }
        if (offset > target)
            break;
    }
    return lineno;
}


#define SN_LINE_LIMIT   (SN_3BYTE_OFFSET_FLAG << 16)

jsbytecode *
js_LineNumberToPC(JSScript *script, uintN target)
{
    ptrdiff_t offset, best;
    uintN lineno, bestdiff, diff;
    jssrcnote *sn;
    JSSrcNoteType type;

    offset = 0;
    best = -1;
    lineno = script->lineno;
    bestdiff = SN_LINE_LIMIT;
    for (sn = script->notes(); !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        



        if (lineno == target && script->code + offset >= script->main)
            goto out;
        if (lineno >= target) {
            diff = lineno - target;
            if (diff < bestdiff) {
                bestdiff = diff;
                best = offset;
            }
        }
        offset += SN_DELTA(sn);
        type = (JSSrcNoteType) SN_TYPE(sn);
        if (type == SRC_SETLINE) {
            lineno = (uintN) js_GetSrcNoteOffset(sn, 0);
        } else if (type == SRC_NEWLINE) {
            lineno++;
        }
    }
    if (best >= 0)
        offset = best;
out:
    return script->code + offset;
}

JS_FRIEND_API(uintN)
js_GetScriptLineExtent(JSScript *script)
{
    uintN lineno;
    jssrcnote *sn;
    JSSrcNoteType type;

    lineno = script->lineno;
    for (sn = script->notes(); !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        type = (JSSrcNoteType) SN_TYPE(sn);
        if (type == SRC_SETLINE) {
            lineno = (uintN) js_GetSrcNoteOffset(sn, 0);
        } else if (type == SRC_NEWLINE) {
            lineno++;
        }
    }
    return 1 + lineno - script->lineno;
}

class DisablePrincipalsTranscoding {
    JSSecurityCallbacks *callbacks;
    JSPrincipalsTranscoder temp;

  public:
    DisablePrincipalsTranscoding(JSContext *cx)
      : callbacks(JS_GetRuntimeSecurityCallbacks(cx->runtime)),
        temp(NULL)
    {
        if (callbacks) {
            temp = callbacks->principalsTranscoder;
            callbacks->principalsTranscoder = NULL;
        }
    }

    ~DisablePrincipalsTranscoding() {
        if (callbacks)
            callbacks->principalsTranscoder = temp;
    }
};

JSScript *
js_CloneScript(JSContext *cx, JSScript *script)
{
    JS_ASSERT(cx->compartment != script->compartment);
    JS_ASSERT(script->compartment);

    
    JSXDRState *w = JS_XDRNewMem(cx, JSXDR_ENCODE);
    if (!w)
        return NULL;

    
    DisablePrincipalsTranscoding disable(cx);

    if (!JS_XDRScript(w, &script)) {
        JS_XDRDestroy(w);
        return NULL;
    }

    uint32 nbytes;
    void *p = JS_XDRMemGetData(w, &nbytes);
    if (!p) {
        JS_XDRDestroy(w);
        return NULL;
    }

    
    JSXDRState *r = JS_XDRNewMem(cx, JSXDR_DECODE);
    if (!r) {
        JS_XDRDestroy(w);
        return NULL;
    }

    
    
    JS_XDRMemSetData(r, p, nbytes);
    JS_XDRMemSetData(w, NULL, 0);

    
    if (!js_XDRScript(r, &script, NULL))
        return NULL;

    JS_XDRDestroy(r);
    JS_XDRDestroy(w);

    
    script->principals = script->compartment->principals;
    if (script->principals)
        JSPRINCIPALS_HOLD(cx, script->principals);

    return script;
}

void
JSScript::copyClosedSlotsTo(JSScript *other)
{
    memcpy(other->closedSlots, closedSlots, nClosedArgs + nClosedVars);
}
