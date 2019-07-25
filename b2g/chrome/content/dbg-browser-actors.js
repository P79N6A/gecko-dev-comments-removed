





'use strict';





function createRootActor(connection) {
  return new DeviceRootActor(connection);
}










function DeviceRootActor(connection) {
  this.conn = connection;
  this._tabActors = new WeakMap();
  this._tabActorPool = null;
  this._actorFactories = null;
  this.browser = Services.wm.getMostRecentWindow('navigator:browser');
}

DeviceRootActor.prototype = {
  


  sayHello: function DRA_sayHello() {
    return {
      from: 'root',
      applicationType: 'browser',
      traits: []
    };
  },

  


  disconnect: function DRA_disconnect() {
    let actor = this._tabActors.get(this.browser);
    if (actor) {
      actor.exit();
    }
  },

  




  onListTabs: function DRA_onListTabs() {
    let actor = this._tabActors.get(this.browser);
    if (!actor) {
      actor = new DeviceTabActor(this.conn, this.browser);
      
      actor.parentID = this.actorID;
      this._tabActors.set(this.browser, actor);
    }

    let actorPool = new ActorPool(this.conn);
    actorPool.addActor(actor);

    
    
    if (this._tabActorPool) {
      this.conn.removeActorPool(this._tabActorPool);
    }
    this._tabActorPool = actorPool;
    this.conn.addActorPool(this._tabActorPool);

    return {
      'from': 'root',
      'selected': 0,
      'tabs': [actor.grip()]
    };
  }

};




DeviceRootActor.prototype.requestTypes = {
  'listTabs': DeviceRootActor.prototype.onListTabs
};










function DeviceTabActor(connection, browser) {
  this.conn = connection;
  this._browser = browser;
}

DeviceTabActor.prototype = {
  get browser() {
    return this._browser;
  },

  get exited() {
    return !this.browser;
  },

  get attached() {
    return !!this._attached
  },

  _tabPool: null,
  get tabActorPool() {
    return this._tabPool;
  },

  _contextPool: null,
  get contextActorPool() {
    return this._contextPool;
  },

  actorPrefix: 'tab',

  grip: function DTA_grip() {
    dbg_assert(!this.exited,
               'grip() should not be called on exited browser actor.');
    dbg_assert(this.actorID,
               'tab should have an actorID.');
    return {
      'actor': this.actorID,
      'title': this.browser.contentTitle,
      'url': this.browser.document.documentURI
    }
  },

  


  disconnect: function DTA_disconnect() {
    this._detach();
  },

  


  exit: function DTA_exit() {
    if (this.exited) {
      return;
    }

    if (this.attached) {
      this._detach();
      this.conn.send({
        'from': this.actorID,
        'type': 'tabDetached'
      });
    }

    this._browser = null;
  },

  


  _attach: function DTA_attach() {
    if (this._attached) {
      return;
    }

    
    dbg_assert(!this._tabPool, 'Should not have a tab pool if we were not attached.');
    this._tabPool = new ActorPool(this.conn);
    this.conn.addActorPool(this._tabPool);

    
    this._pushContext();

    this._attached = true;
  },

  



  _pushContext: function DTA_pushContext() {
    dbg_assert(!this._contextPool, "Can't push multiple contexts");

    this._contextPool = new ActorPool(this.conn);
    this.conn.addActorPool(this._contextPool);

    this.threadActor = new ThreadActor(this);
    this._addDebuggees(this.browser.content.wrappedJSObject);
    this._contextPool.addActor(this.threadActor);
  },

  


  _addDebuggees: function DTA__addDebuggees(content) {
    this.threadActor.addDebuggee(content);
    let frames = content.frames;
    for (let i = 0; i < frames.length; i++) {
      this._addDebuggees(frames[i]);
    }
  },

  



  _popContext: function DTA_popContext() {
    dbg_assert(!!this._contextPool, 'No context to pop.');

    this.conn.removeActorPool(this._contextPool);
    this._contextPool = null;
    this.threadActor.exit();
    this.threadActor = null;
  },

  


  _detach: function DTA_detach() {
    if (!this.attached) {
      return;
    }

    this._popContext();

    
    this.conn.removeActorPool(this._tabPool);
    this._tabPool = null;

    this._attached = false;
  },

  

  onAttach: function DTA_onAttach(aRequest) {
    if (this.exited) {
      return { type: 'exited' };
    }

    this._attach();

    return { type: 'tabAttached', threadActor: this.threadActor.actorID };
  },

  onDetach: function DTA_onDetach(aRequest) {
    if (!this.attached) {
      return { error: 'wrongState' };
    }

    this._detach();

    return { type: 'detached' };
  },

  


  preNest: function DTA_preNest() {
    let windowUtils = this.browser.content
                          .QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIDOMWindowUtils);
    windowUtils.suppressEventHandling(true);
    windowUtils.suspendTimeouts();
  },

  


  postNest: function DTA_postNest(aNestData) {
    let windowUtils = this.browser.content
                          .QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIDOMWindowUtils);
    windowUtils.resumeTimeouts();
    windowUtils.suppressEventHandling(false);
  }

};




DeviceTabActor.prototype.requestTypes = {
  'attach': DeviceTabActor.prototype.onAttach,
  'detach': DeviceTabActor.prototype.onDetach
};
