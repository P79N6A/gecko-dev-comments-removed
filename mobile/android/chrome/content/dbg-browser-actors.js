




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

MobileTabList.prototype.iterator = function() {
  
  
  let initialMapSize = this._actorByBrowser.size;
  let foundCount = 0;

  
  
  
  

  
  for (let win of allAppShellDOMWindows("navigator:browser")) {
    let selectedTab = win.BrowserApp.selectedBrowser;

    
    
    
    
    for (let tab of win.BrowserApp.tabs) {
      let browser = tab.browser;
      
      let actor = this._actorByBrowser.get(browser);
      if (actor) {
        foundCount++;
      } else {
        actor = new BrowserTabActor(this._connection, browser);
        this._actorByBrowser.set(browser, actor);
      }

      
      actor.selected = (browser === selectedTab);
    }
  }

  if (this._testing && initialMapSize !== foundCount)
    throw Error("_actorByBrowser map contained actors for dead tabs");

  this._mustNotify = true;
  this._checkListening();

  
  for (let [browser, actor] of this._actorByBrowser) {
    yield actor;
  }
};
