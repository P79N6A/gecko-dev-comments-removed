





#ifndef dom_plugins_ipc_PluginDataResolver_h
#define dom_plugins_ipc_PluginDataResolver_h

namespace mozilla {
namespace plugins {

class PluginAsyncSurrogate;
class PluginInstanceParent;

class PluginDataResolver
{
public:
    virtual PluginAsyncSurrogate* GetAsyncSurrogate() = 0;
    virtual PluginInstanceParent* GetInstance() = 0;
};

} 
} 

#endif 
