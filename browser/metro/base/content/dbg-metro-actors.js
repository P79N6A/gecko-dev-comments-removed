




"use strict";
const error = Components.utils.reportError;





function createRootActor(aConnection) {
  let parameters = {
    tabList: new MetroTabList(aConnection),
    globalActorFactories: DebuggerServer.globalActorFactories,
    onShutdown: sendShutdownEvent
  };
  return new RootActor(aConnection, parameters);
}



function MetroTabList(aConnection) {
  BrowserTabList.call(this, aConnection);
}

MetroTabList.prototype = Object.create(BrowserTabList.prototype);
MetroTabList.prototype.constructor = MetroTabList;

MetroTabList.prototype._getSelectedBrowser = function(aWindow) {
  return aWindow.Browser.selectedBrowser;
};

MetroTabList.prototype._getChildren = function(aWindow) {
  return aWindow.Browser.browsers;
};
