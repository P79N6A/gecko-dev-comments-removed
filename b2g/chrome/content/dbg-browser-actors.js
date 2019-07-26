





'use strict';









function createRootActor(connection) {
  return new DeviceRootActor(connection);
}










function DeviceRootActor(connection) {
  BrowserRootActor.call(this, connection);
  this.browser = Services.wm.getMostRecentWindow('navigator:browser');
}

DeviceRootActor.prototype = new BrowserRootActor();




DeviceRootActor.prototype.disconnect = function DRA_disconnect() {
  this._extraActors = null;
  let actor = this._tabActors.get(this.browser);
  if (actor) {
    actor.exit();
  }
};






DeviceRootActor.prototype.onListTabs = function DRA_onListTabs() {
  let actorPool = new ActorPool(this.conn);

  this._createExtraActors(DebuggerServer.globalActorFactories, actorPool);

  
  
  if (this._tabActorPool) {
    this.conn.removeActorPool(this._tabActorPool);
  }
  this._tabActorPool = actorPool;
  this.conn.addActorPool(this._tabActorPool);

  let response = {
    'from': 'root',
    'selected': 0,
    'tabs': []
  };
  this._appendExtraActors(response);
  return response;
};




DeviceRootActor.prototype.requestTypes = {
  'listTabs': DeviceRootActor.prototype.onListTabs
};










function DeviceTabActor(connection, browser) {
  BrowserTabActor.call(this, connection, browser);
}

DeviceTabActor.prototype = new BrowserTabActor();

Object.defineProperty(DeviceTabActor.prototype, "title", {
  get: function() {
    return this.browser.title;
  },
  enumerable: true,
  configurable: false
});

Object.defineProperty(DeviceTabActor.prototype, "url", {
  get: function() {
    return this.browser.document.documentURI;
  },
  enumerable: true,
  configurable: false
});

Object.defineProperty(DeviceTabActor.prototype, "contentWindow", {
  get: function() {
    return this.browser;
  },
  enumerable: true,
  configurable: false
});
