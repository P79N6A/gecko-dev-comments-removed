


const { ActorPool, appendExtraActors, createExtraActors } = require("devtools/server/actors/common");
const { RootActor } = require("devtools/server/actors/root");
const { ThreadActor } = require("devtools/server/actors/script");
const { DebuggerServer } = require("devtools/server/main");
const promise = require("promise");
const makeDebugger = require("devtools/server/actors/utils/make-debugger");

var gTestGlobals = [];
DebuggerServer.addTestGlobal = function(aGlobal) {
  gTestGlobals.push(aGlobal);
};








function TestTabList(aConnection) {
  this.conn = aConnection;

  
  
  this._tabActors = [];

  
  this._tabActorPool = new ActorPool(aConnection);

  for (let global of gTestGlobals) {
    let actor = new TestTabActor(aConnection, global);
    actor.selected = false;
    this._tabActors.push(actor);
    this._tabActorPool.addActor(actor);
  }
  if (this._tabActors.length > 0) {
    this._tabActors[0].selected = true;
  }

  aConnection.addActorPool(this._tabActorPool);
}

TestTabList.prototype = {
  constructor: TestTabList,
  getList: function () {
    return Promise.resolve([tabActor for (tabActor of this._tabActors)]);
  }
};

function createRootActor(aConnection)
{
  let root = new RootActor(aConnection,
                           { tabList: new TestTabList(aConnection) });
  root.applicationType = "xpcshell-tests";
  return root;
}

function TestTabActor(aConnection, aGlobal)
{
  this.conn = aConnection;
  this._global = aGlobal;
  this._global.wrappedJSObject = aGlobal;
  this.threadActor = new ThreadActor(this, this._global);
  this.conn.addActor(this.threadActor);
  this._attached = false;
  this._extraActors = {};
  this.makeDebugger = makeDebugger.bind(null, {
    findDebuggees: () => [this._global],
    shouldAddNewGlobalAsDebuggee: g => g.hostAnnotations &&
                                       g.hostAnnotations.type == "document" &&
                                       g.hostAnnotations.element === this._global

  });
}

TestTabActor.prototype = {
  constructor: TestTabActor,
  actorPrefix: "TestTabActor",

  get window() {
    return { wrappedJSObject: this._global };
  },

  get url() {
    return this._global.__name;
  },

  form: function() {
    let response = { actor: this.actorID, title: this._global.__name };

    
    let actorPool = new ActorPool(this.conn);
    this._createExtraActors(DebuggerServer.tabActorFactories, actorPool);
    if (!actorPool.isEmpty()) {
      this._tabActorPool = actorPool;
      this.conn.addActorPool(this._tabActorPool);
    }

    this._appendExtraActors(response);

    return response;
  },

  onAttach: function(aRequest) {
    this._attached = true;

    let response = { type: "tabAttached", threadActor: this.threadActor.actorID };
    this._appendExtraActors(response);

    return response;
  },

  onDetach: function(aRequest) {
    if (!this._attached) {
      return { "error":"wrongState" };
    }
    return { type: "detached" };
  },

  
  _createExtraActors: createExtraActors,
  _appendExtraActors: appendExtraActors
};

TestTabActor.prototype.requestTypes = {
  "attach": TestTabActor.prototype.onAttach,
  "detach": TestTabActor.prototype.onDetach
};

exports.register = function(handle) {
  handle.setRootActor(createRootActor);
};

exports.unregister = function(handle) {
  handle.setRootActor(null);
};
