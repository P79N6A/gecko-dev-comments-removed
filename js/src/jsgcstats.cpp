





































#include "mozilla/Util.h"

#include "jstypes.h"
#include "jscntxt.h"
#include "jsgcstats.h"
#include "jsgc.h"
#include "jsxml.h"
#include "jsbuiltins.h"
#include "jscompartment.h"

#include "jsgcinlines.h"

using namespace mozilla;
using namespace js;
using namespace js::gc;

#define UL(x)       ((unsigned long)(x))
#define PERCENT(x,y)  (100.0 * (double) (x) / (double) (y))

namespace js {
namespace gc {

#if defined(JS_DUMP_CONSERVATIVE_GC_ROOTS)

void
ConservativeGCStats::dump(FILE *fp)
{
    size_t words = 0;
    for (size_t i = 0; i < ArrayLength(counter); ++i)
        words += counter[i];
   
#define ULSTAT(x)       ((unsigned long)(x))
    fprintf(fp, "CONSERVATIVE STACK SCANNING:\n");
    fprintf(fp, "      number of stack words: %lu\n", ULSTAT(words));
    fprintf(fp, "      excluded, low bit set: %lu\n", ULSTAT(counter[CGCT_LOWBITSET]));
    fprintf(fp, "        not withing a chunk: %lu\n", ULSTAT(counter[CGCT_NOTCHUNK]));
    fprintf(fp, "     not within arena range: %lu\n", ULSTAT(counter[CGCT_NOTARENA]));
    fprintf(fp, "     in another compartment: %lu\n", ULSTAT(counter[CGCT_OTHERCOMPARTMENT]));
    fprintf(fp, "       points to free arena: %lu\n", ULSTAT(counter[CGCT_FREEARENA]));
    fprintf(fp, "         excluded, not live: %lu\n", ULSTAT(counter[CGCT_NOTLIVE]));
    fprintf(fp, "            valid GC things: %lu\n", ULSTAT(counter[CGCT_VALID]));
    fprintf(fp, "      valid but not aligned: %lu\n", ULSTAT(unaligned));
#undef ULSTAT
}
#endif

} 

#ifdef JS_DUMP_CONSERVATIVE_GC_ROOTS
void
GCMarker::dumpConservativeRoots()
{
    if (!conservativeDumpFileName)
        return;

    FILE *fp;
    if (!strcmp(conservativeDumpFileName, "stdout")) {
        fp = stdout;
    } else if (!strcmp(conservativeDumpFileName, "stderr")) {
        fp = stderr;
    } else if (!(fp = fopen(conservativeDumpFileName, "aw"))) {
        fprintf(stderr,
                "Warning: cannot open %s to dump the conservative roots\n",
                conservativeDumpFileName);
        return;
    }

    conservativeStats.dump(fp);

    for (void **thingp = conservativeRoots.begin(); thingp != conservativeRoots.end(); ++thingp) {
        void *thing = thingp;
        fprintf(fp, "  %p: ", thing);

        switch (GetGCThingTraceKind(thing)) {
          case JSTRACE_OBJECT: {
            JSObject *obj = (JSObject *) thing;
            fprintf(fp, "object %s", obj->getClass()->name);
            break;
          }
          case JSTRACE_STRING: {
            JSString *str = (JSString *) thing;
            if (str->isLinear()) {
                char buf[50];
                PutEscapedString(buf, sizeof buf, &str->asLinear(), '"');
                fprintf(fp, "string %s", buf);
            } else {
                fprintf(fp, "rope: length %d", (int)str->length());
            }
            break;
          }
          case JSTRACE_SCRIPT: {
            fprintf(fp, "shape");
            break;
          }
          case JSTRACE_SHAPE: {
            fprintf(fp, "shape");
            break;
          }
          case JSTRACE_TYPE_OBJECT: {
            fprintf(fp, "type_object");
            break;
          }
# if JS_HAS_XML_SUPPORT
          case JSTRACE_XML: {
            JSXML *xml = (JSXML *) thing;
            fprintf(fp, "xml %u", (unsigned)xml->xml_class);
            break;
          }
# endif
        }
        fputc('\n', fp);
    }
    fputc('\n', fp);

    if (fp != stdout && fp != stderr)
        fclose(fp);
}
#endif 

} 
