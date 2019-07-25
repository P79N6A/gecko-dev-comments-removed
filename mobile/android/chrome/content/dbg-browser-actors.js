




"use strict";









function createRootActor(aConnection) {
  return new FennecRootActor(aConnection);
}










function FennecRootActor(aConnection) {
  BrowserRootActor.call(this, aConnection);
}

FennecRootActor.prototype = new BrowserRootActor();






FennecRootActor.prototype.onListTabs = function FRA_onListTabs() {
  
  
  

  let actorPool = new ActorPool(this.conn);
  let actorList = [];

  let win = windowMediator.getMostRecentWindow("navigator:browser");
  this.browser = win.BrowserApp.selectedBrowser;

  
  
  this.watchWindow(win);

  let tabs = win.BrowserApp.tabs;
  let selected;

  for each (let tab in tabs) {
    let browser = tab.browser;

    if (browser == this.browser) {
      selected = actorList.length;
    }

    let actor = this._tabActors.get(browser);
    if (!actor) {
      actor = new BrowserTabActor(this.conn, browser);
      actor.parentID = this.actorID;
      this._tabActors.set(browser, actor);
    }

    actorPool.addActor(actor);
    actorList.push(actor);
  }

  
  
  
  if (this._tabActorPool) {
    this.conn.removeActorPool(this._tabActorPool);
  }

  this._tabActorPool = actorPool;
  this.conn.addActorPool(this._tabActorPool);

  return { "from": "root",
           "selected": selected,
           "tabs": [actor.grip()
                    for each (actor in actorList)] };
};




FennecRootActor.prototype.getTabContainer = function FRA_getTabContainer(aWindow) {
  return aWindow.document.getElementById("browsers");
};





FennecRootActor.prototype.onTabClosed = function FRA_onTabClosed(aEvent) {
  this.exitTabActor(aEvent.target.browser);
};


FennecRootActor.prototype.onCloseWindow = function FRA_onCloseWindow(aWindow) {
  if (aWindow.BrowserApp) {
    this.unwatchWindow(aWindow);
  }
};




FennecRootActor.prototype.requestTypes = {
  "listTabs": FennecRootActor.prototype.onListTabs
};
