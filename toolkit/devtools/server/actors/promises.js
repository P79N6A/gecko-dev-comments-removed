



"use strict";

const protocol = require("devtools/server/protocol");
const { method, RetVal, Arg, types } = protocol;
const { expectState, ActorPool } = require("devtools/server/actors/common");
const { ObjectActor, createValueGrip } = require("devtools/server/actors/object");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
loader.lazyRequireGetter(this, "events", "sdk/event/core");


types.addType("ObjectActor", {
  write: actor => actor.grip(),
  read: grip => grip
});





let PromisesActor = protocol.ActorClass({
  typeName: "promises",

  events: {
    
    
    "new-promises": {
      type: "new-promises",
      data: Arg(0, "array:ObjectActor"),
    },
    
    "promises-settled": {
      type: "promises-settled",
      data: Arg(0, "array:ObjectActor")
    }
  },

  



  initialize: function(conn, parent) {
    protocol.Actor.prototype.initialize.call(this, conn);

    this.conn = conn;
    this.parent = parent;
    this.state = "detached";
    this._dbg = null;
    this._gripDepth = 0;
    this._navigationLifetimePool = null;
    this._newPromises = null;
    this._promisesSettled = null;

    this.objectGrip = this.objectGrip.bind(this);
    this._makePromiseEventHandler = this._makePromiseEventHandler.bind(this);
    this._onWindowReady = this._onWindowReady.bind(this);
  },

  destroy: function() {
    protocol.Actor.prototype.destroy.call(this, this.conn);

    if (this.state === "attached") {
      this.detach();
    }
  },

  get dbg() {
    if (!this._dbg) {
      this._dbg = this.parent.makeDebugger();
    }
    return this._dbg;
  },

  


  attach: method(expectState("detached", function() {
    this.dbg.addDebuggees();

    this._navigationLifetimePool = this._createActorPool();
    this.conn.addActorPool(this._navigationLifetimePool);

    this._newPromises = [];
    this._promisesSettled = [];

    events.on(this.parent, "window-ready", this._onWindowReady);

    this.state = "attached";
  }, `attaching to the PromisesActor`), {
    request: {},
    response: {}
  }),

  


  detach: method(expectState("attached", function() {
    this.dbg.removeAllDebuggees();
    this.dbg.enabled = false;
    this._dbg = null;
    this._newPromises = null;
    this._promisesSettled = null;

    if (this._navigationLifetimePool) {
      this.conn.removeActorPool(this._navigationLifetimePool);
      this._navigationLifetimePool = null;
    }

    events.off(this.parent, "window-ready", this._onWindowReady);

    this.state = "detached";
  }, `detaching from the PromisesActor`), {
    request: {},
    response: {}
  }),

  _createActorPool: function() {
    let pool = new ActorPool(this.conn);
    pool.objectActors = new WeakMap();
    return pool;
  },

  







  _createObjectActorForPromise: function(promise) {
    if (this._navigationLifetimePool.objectActors.has(promise)) {
      return this._navigationLifetimePool.objectActors.get(promise);
    }

    let actor = new ObjectActor(promise, {
      getGripDepth: () => this._gripDepth,
      incrementGripDepth: () => this._gripDepth++,
      decrementGripDepth: () => this._gripDepth--,
      createValueGrip: v =>
        createValueGrip(v, this._navigationLifetimePool, this.objectGrip),
      sources: () => DevToolsUtils.reportException("PromisesActor",
        Error("sources not yet implemented")),
      createEnvironmentActor: () => DevToolsUtils.reportException(
        "PromisesActor", Error("createEnvironmentActor not yet implemented"))
    });

    this._navigationLifetimePool.addActor(actor);
    this._navigationLifetimePool.objectActors.set(promise, actor);

    return actor;
  },

  







  objectGrip: function(value) {
    return this._createObjectActorForPromise(value).grip();
  },

  


  listPromises: method(function() {
    let promises = this.dbg.findObjects({ class: "Promise" });

    this.dbg.onNewPromise = this._makePromiseEventHandler(this._newPromises,
      "new-promises");
    this.dbg.onPromiseSettled = this._makePromiseEventHandler(
      this._promisesSettled, "promises-settled");

    return promises.map(p => this._createObjectActorForPromise(p));
  }, {
    request: {
    },
    response: {
      promises: RetVal("array:ObjectActor")
    }
  }),

  









  _makePromiseEventHandler: function(array, eventName) {
    return promise => {
      let actor = this._createObjectActorForPromise(promise)
      let needsScheduling = array.length == 0;

      array.push(actor);

      if (needsScheduling) {
        DevToolsUtils.executeSoon(() => {
          events.emit(this, eventName, array.splice(0, array.length));
        });
      }
    }
  },

  _onWindowReady: expectState("attached", function({ isTopLevel }) {
    if (!isTopLevel) {
      return;
    }

    this._navigationLifetimePool.cleanup();
    this.dbg.removeAllDebuggees();
    this.dbg.addDebuggees();
  })
});

exports.PromisesActor = PromisesActor;

exports.PromisesFront = protocol.FrontClass(PromisesActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
    this.actorID = form.promisesActor;
    this.manage(this);
  }
});
