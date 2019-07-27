



"use strict";

const { Cc, Ci, Cu } = require("chrome");

const { ChromeDebuggerActor } = require("devtools/server/actors/script");
const { WebConsoleActor } = require("devtools/server/actors/webconsole");
const makeDebugger = require("devtools/server/actors/utils/make-debugger");
const { ActorPool } = require("devtools/server/main");
const Services = require("Services");
const { dbg_assert } = require("devtools/toolkit/DevToolsUtils");
const { TabSources } = require("./utils/TabSources");

function ChildProcessActor(aConnection) {
  this.conn = aConnection;
  this._contextPool = new ActorPool(this.conn);
  this.conn.addActorPool(this._contextPool);
  this._threadActor = null;

  
  this.makeDebugger = makeDebugger.bind(null, {
    findDebuggees: dbg => dbg.findAllGlobals(),
    shouldAddNewGlobalAsDebuggee: global => true
  });

  
  
  let systemPrincipal = Cc["@mozilla.org/systemprincipal;1"]
    .createInstance(Ci.nsIPrincipal);
  let sandbox = Cu.Sandbox(systemPrincipal);
  this._consoleScope = sandbox;
}
exports.ChildProcessActor = ChildProcessActor;

ChildProcessActor.prototype = {
  actorPrefix: "process",

  get isRootActor() true,

  get exited() {
    return !this._contextPool;
  },

  get url() {
    return undefined;
  },

  get window() {
    return this._consoleScope;
  },

  get sources() {
    if (!this._sources) {
      dbg_assert(this._threadActor, "threadActor should exist when creating sources.");
      this._sources = new TabSources(this._threadActor);
    }
    return this._sources;
  },

  form: function() {
    if (!this._consoleActor) {
      this._consoleActor = new WebConsoleActor(this.conn, this);
      this._contextPool.addActor(this._consoleActor);
    }

    if (!this._threadActor) {
      this._threadActor = new ChromeDebuggerActor(this.conn, this);
      this._contextPool.addActor(this._threadActor);
    }

    return {
      actor: this.actorID,
      name: "Content process",

      consoleActor: this._consoleActor.actorID,
      chromeDebugger: this._threadActor.actorID,

      traits: {
        highlightable: false,
        networkMonitor: false,
      },
    };
  },

  disconnect: function() {
    this.conn.removeActorPool(this._contextPool);
    this._contextPool = null;
  },

  preNest: function() {
    
    
    
  },

  postNest: function() {
  },
};

ChildProcessActor.prototype.requestTypes = {
};
