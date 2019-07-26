


var gTestGlobals = [];

function createRootActor()
{
  let actor = {
    sayHello: function() {
      this._tabActors = [];
      for each (let g in gTestGlobals) {
        let actor = new BrowserTabActor(this.conn);
        actor.thread = new ThreadActor({});
        actor.thread.addDebuggee(g);
        actor.thread.global = g;
        actor.json = function() {
          return { actor: actor.actorID,
                   url: "http://www.example.com/",
                   title: actor.thread.global.__name };
        };
        actor.requestTypes["attach"] = function (aRequest) {
          dump("actor.thread.actorID = " + actor.thread.actorID + "\n");
          return {
            from: actor.actorID,
            type: "tabAttached",
            threadActor: actor.thread.actorID
          };
        };
        this.conn.addActor(actor);
        this.conn.addActor(actor.thread);
        this._tabActors.push(actor);
      }

      this.conn.send = (function (aOldSend) {
        return function (aPacket) {
          if (aPacket.type === "newSource") {
            
            
            return undefined;
          } else {
            return aOldSend.call(this, aPacket);
          }
        };
      }(this.conn.send));

      return { from: "root",
               applicationType: "xpcshell-tests",
               traits: {} };
    },

    listTabs: function(aRequest) {
      return {
        from: "root",
        selected: 0,
        tabs: [ actor.json() for (actor of this._tabActors) ]
      };
    },
  };

  actor.requestTypes = {
    "listTabs": actor.listTabs,
    "echo": function(aRequest) { return aRequest; },
    
    
    "sources": function () {
      return {
        error: "unrecognizedPacketType"
      }
    },
  };
  return actor;
}

DebuggerServer.addTestGlobal = function addTestGlobal(aGlobal)
{
  gTestGlobals.push(aGlobal);
}
