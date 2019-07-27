





#ifndef mozilla_plugins_PluginBridge_h
#define mozilla_plugins_PluginBridge_h

namespace mozilla {

namespace dom {
class ContentParent;
}

namespace plugins {

bool
SetupBridge(uint32_t aPluginId, dom::ContentParent* aContentParent,
            bool aForceBridgeNow, nsresult* aResult, uint32_t* aRunID);

bool
FindPluginsForContent(uint32_t aPluginEpoch,
                      nsTArray<PluginTag>* aPlugins,
                      uint32_t* aNewPluginEpoch);

void
TerminatePlugin(uint32_t aPluginId);

} 
} 

#endif 
