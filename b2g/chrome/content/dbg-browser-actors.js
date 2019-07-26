





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
  let actor = this._tabActors.get(this.browser);
  if (!actor) {
    actor = new DeviceTabActor(this.conn, this.browser);
    
    actor.parentID = this.actorID;
    this._tabActors.set(this.browser, actor);
  }

  let actorPool = new ActorPool(this.conn);
  actorPool.addActor(actor);

  this._createExtraActors(DebuggerServer.globalActorFactories, actorPool);

  
  
  if (this._tabActorPool) {
    this.conn.removeActorPool(this._tabActorPool);
  }
  this._tabActorPool = actorPool;
  this.conn.addActorPool(this._tabActorPool);

  let response = {
    'from': 'root',
    'selected': 0,
    'tabs': [actor.grip()]
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

DeviceTabActor.prototype.grip = function DTA_grip() {
  dbg_assert(!this.exited,
             'grip() should not be called on exited browser actor.');
  dbg_assert(this.actorID,
             'tab should have an actorID.');

  let response = {
    'actor': this.actorID,
    'title': this.browser.title,
    'url': this.browser.document.documentURI
  };

  
  let actorPool = new ActorPool(this.conn);
  this._createExtraActors(DebuggerServer.globalActorFactories, actorPool);
  if (!actorPool.isEmpty()) {
    this._tabActorPool = actorPool;
    this.conn.addActorPool(this._tabActorPool);
  }

  this._appendExtraActors(response);
  return response;
};





DeviceTabActor.prototype._pushContext = function DTA_pushContext() {
  dbg_assert(!this._contextPool, "Can't push multiple contexts");

  this._contextPool = new ActorPool(this.conn);
  this.conn.addActorPool(this._contextPool);

  this.threadActor = new ThreadActor(this);
  this._addDebuggees(this.browser.wrappedJSObject);
  this._contextPool.addActor(this.threadActor);
};






DeviceTabActor.prototype.preNest = function DTA_preNest() {
  let windowUtils = this.browser
                        .QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIDOMWindowUtils);
  windowUtils.suppressEventHandling(true);
  windowUtils.suspendTimeouts();
};




DeviceTabActor.prototype.postNest = function DTA_postNest(aNestData) {
  let windowUtils = this.browser
                        .QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIDOMWindowUtils);
  windowUtils.resumeTimeouts();
  windowUtils.suppressEventHandling(false);
};
