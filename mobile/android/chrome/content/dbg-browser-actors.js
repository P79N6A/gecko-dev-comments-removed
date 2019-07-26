




"use strict";















function createRootActor(aConnection)
{
  let parameters = {
    tabList: new MobileTabList(aConnection),
    globalActorFactories: DebuggerServer.globalActorFactories,
    onShutdown: sendShutdownEvent
  };
  return new RootActor(aConnection, parameters);
}

















function MobileTabList(aConnection)
{
  BrowserTabList.call(this, aConnection);
}

MobileTabList.prototype = Object.create(BrowserTabList.prototype);

MobileTabList.prototype.constructor = MobileTabList;

MobileTabList.prototype._getSelectedBrowser = function(aWindow) {
  return aWindow.BrowserApp.selectedBrowser;
};

MobileTabList.prototype._getChildren = function(aWindow) {
  return aWindow.BrowserApp.tabs.map(tab => tab.browser);
};
