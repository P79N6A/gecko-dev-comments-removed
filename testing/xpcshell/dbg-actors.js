



'use strict';

const { Promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
let { Services } = Cu.import("resource://gre/modules/Services.jsm", {});
const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const { RootActor } = devtools.require("devtools/server/actors/root");
const { BrowserTabList } = devtools.require("devtools/server/actors/webbrowser");










function createRootActor(connection)
{
  let parameters = {
    tabList: new XPCSTTabList(connection),
    globalActorFactories: DebuggerServer.globalActorFactories,
    onShutdown() {
      
      
      Services.obs.notifyObservers(null, "xpcshell-test-devtools-shutdown", null);
    }
  };
  return new RootActor(connection, parameters);
}





function XPCSTTabList(connection)
{
  BrowserTabList.call(this, connection);
}

XPCSTTabList.prototype = Object.create(BrowserTabList.prototype);

XPCSTTabList.prototype.constructor = XPCSTTabList;

XPCSTTabList.prototype.getList = function() {
  return Promise.resolve([]);
};
