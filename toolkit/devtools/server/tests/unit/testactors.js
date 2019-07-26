


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
  iterator: function() {
    for (let actor of this._tabActors) {
      yield actor;
    }
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
  this._threadActor = new ThreadActor(this, this._global);
  this.conn.addActor(this._threadActor);
  this._attached = false;
}

TestTabActor.prototype = {
  constructor: TestTabActor,
  actorPrefix: "TestTabActor",

  grip: function() {
    return { actor: this.actorID, title: this._global.__name };
  },

  onAttach: function(aRequest) {
    this._attached = true;
    return { type: "tabAttached", threadActor: this._threadActor.actorID };
  },

  onDetach: function(aRequest) {
    if (!this._attached) {
      return { "error":"wrongState" };
    }
    return { type: "detached" };
  },

  
  addToParentPool: function(aActor) {
    this.conn.addActor(aActor);
  },

  removeFromParentPool: function(aActor) {
    this.conn.removeActor(aActor);
  }
};

TestTabActor.prototype.requestTypes = {
  "attach": TestTabActor.prototype.onAttach,
  "detach": TestTabActor.prototype.onDetach
};
