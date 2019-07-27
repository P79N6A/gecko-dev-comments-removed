



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

  



  initialize: function(conn, parent) {
    protocol.Actor.prototype.initialize.call(this, conn);

    this.conn = conn;
    this.parent = parent;
    this.state = "detached";
    this._dbg = null;
    this._gripDepth = 0;
    this._navigationLifetimePool = null;

    this.objectGrip = this.objectGrip.bind(this);
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
    let object = new ObjectActor(promise, {
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
    this._navigationLifetimePool.addActor(object);
    return object;
  },

  







  objectGrip: function(value) {
    let pool = this._navigationLifetimePool;

    if (pool.objectActors.has(value)) {
      return pool.objectActors.get(value).grip();
    }

    let actor = this._createObjectActorForPromise(value);
    pool.objectActors.set(value, actor);
    return actor.grip();
  },

  


  listPromises: method(function() {
    let promises = this.dbg.findObjects({ class: "Promise" });
    return promises.map(p => this._createObjectActorForPromise(p));
  }, {
    request: {
    },
    response: {
      promises: RetVal("array:ObjectActor")
    }
  }),

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
