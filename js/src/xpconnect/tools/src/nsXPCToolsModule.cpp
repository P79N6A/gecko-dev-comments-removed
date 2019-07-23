









































#include "xpctools_private.h"



NS_GENERIC_FACTORY_CONSTRUCTOR(nsXPCToolsCompiler)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsXPCToolsProfiler)


#define COMPILER_CID \
    { 0x331148c0, 0xe599, 0x11d3, \
        { 0x8f, 0x65, 0x0, 0x10, 0xa4, 0xe7, 0x3d, 0x9a } }


#define PROFILER_CID \
    { 0x7f5d12e0, 0xe97b, 0x11d3, \
        { 0x8f, 0x69, 0x0, 0x10, 0xa4, 0xe7, 0x3d, 0x9a } }

static const nsModuleComponentInfo components[] = {
 {nsnull, COMPILER_CID, XPCTOOLS_COMPILER_CONTRACTID, nsXPCToolsCompilerConstructor},
 {nsnull, PROFILER_CID, XPCTOOLS_PROFILER_CONTRACTID, nsXPCToolsProfilerConstructor}
};

NS_IMPL_NSGETMODULE(xpctools, components)
