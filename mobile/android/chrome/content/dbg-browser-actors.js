




"use strict";









function createRootActor(aConnection) {
  return new DeviceRootActor(aConnection);
}










function DeviceRootActor(aConnection) {
  BrowserRootActor.call(this, aConnection);
}

DeviceRootActor.prototype = new BrowserRootActor();






DeviceRootActor.prototype.onListTabs = function DRA_onListTabs() {
  
  
  

  let actorPool = new ActorPool(this.conn);
  let tabActorList = [];

  
  let actor = this._chromeDebugger;
  if (!actor) {
    actor = new ChromeDebuggerActor(this);
    actor.parentID = this.actorID;
    this._chromeDebugger = actor;
    actorPool.addActor(actor);
  }

  let win = windowMediator.getMostRecentWindow("navigator:browser");
  this.browser = win.BrowserApp.selectedBrowser;

  
  
  this.watchWindow(win);

  let tabs = win.BrowserApp.tabs;
  let selected;

  for each (let tab in tabs) {
    let browser = tab.browser;

    if (browser == this.browser) {
      selected = tabActorList.length;
    }

    let actor = this._tabActors.get(browser);
    if (!actor) {
      actor = new BrowserTabActor(this.conn, browser);
      actor.parentID = this.actorID;
      this._tabActors.set(browser, actor);
    }

    actorPool.addActor(actor);
    tabActorList.push(actor);
  }

  this._createExtraActors(DebuggerServer.globalActorFactories, actorPool);

  
  
  
  if (this._tabActorPool) {
    this.conn.removeActorPool(this._tabActorPool);
  }

  this._tabActorPool = actorPool;
  this.conn.addActorPool(this._tabActorPool);

  let response = {
    "from": "root",
    "selected": selected,
    "tabs": [actor.grip() for (actor of tabActorList)],
    "chromeDebugger": this._chromeDebugger.actorID
  };
  this._appendExtraActors(response);
  return response;
};




DeviceRootActor.prototype.getTabContainer = function DRA_getTabContainer(aWindow) {
  return aWindow.document.getElementById("browsers");
};





DeviceRootActor.prototype.onTabClosed = function DRA_onTabClosed(aEvent) {
  this.exitTabActor(aEvent.target.browser);
};


DeviceRootActor.prototype.onCloseWindow = function DRA_onCloseWindow(aWindow) {
  if (aWindow.BrowserApp) {
    this.unwatchWindow(aWindow);
  }
};




DeviceRootActor.prototype.requestTypes = {
  "listTabs": DeviceRootActor.prototype.onListTabs
};
