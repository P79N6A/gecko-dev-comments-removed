




"use strict";




const { RootActor } = require("devtools/server/actors/root");
const { DebuggerServer } = require("devtools/server/main");
const { BrowserTabList, BrowserAddonList, sendShutdownEvent } =
  require("devtools/server/actors/webbrowser");












function createRootActor(aConnection)
{
  let parameters = {
    tabList: new MobileTabList(aConnection),
    addonList: new BrowserAddonList(aConnection),
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

exports.register = function(handle) {
  handle.setRootActor(createRootActor);
};

exports.unregister = function(handle) {
  handle.setRootActor(null);
};
