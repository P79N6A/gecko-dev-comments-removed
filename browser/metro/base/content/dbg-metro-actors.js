




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





MetroTabList.prototype.iterator = function() {

  let initialMapSize = this._actorByBrowser.size;
  let foundCount = 0;
  for (let win of allAppShellDOMWindows("navigator:browser")) {
    let selectedTab = win.Browser.selectedBrowser;

    for (let browser of win.Browser.browsers) {
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

  if (this._testing && initialMapSize !== foundCount) {
    throw error("_actorByBrowser map contained actors for dead tabs");
  }

  this._mustNotify = true;
  this._checkListening();

  for (let [browser, actor] of this._actorByBrowser) {
    yield actor;
  }
};
