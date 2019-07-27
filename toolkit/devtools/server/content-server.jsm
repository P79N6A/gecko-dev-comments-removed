



"use strict";

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

const { Services } = Cu.import("resource://gre/modules/Services.jsm", {});
const { DevToolsLoader } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});

this.EXPORTED_SYMBOLS = ["init"];

let started = false;

function init(msg) {
  if (started) {
    return;
  }
  started = true;

  
  
  
  let devtools = new DevToolsLoader();
  devtools.invisibleToDebugger = true;
  devtools.main("devtools/server/main");
  let { DebuggerServer, ActorPool } = devtools;

  if (!DebuggerServer.initialized) {
    DebuggerServer.init();
  }

  
  
  
  
  DebuggerServer.addChildActors();

  let mm = msg.target;
  mm.QueryInterface(Ci.nsISyncMessageSender);
  let prefix = msg.data.prefix;

  
  let conn = DebuggerServer.connectToParent(prefix, mm);

  let { ChildProcessActor } = devtools.require("devtools/server/actors/child-process");
  let actor = new ChildProcessActor(conn);
  let actorPool = new ActorPool(conn);
  actorPool.addActor(actor);
  conn.addActorPool(actorPool);

  let response = {actor: actor.form()};
  mm.sendAsyncMessage("debug:content-process-actor", response);

  mm.addMessageListener("debug:content-process-destroy", function onDestroy() {
    mm.removeMessageListener("debug:content-process-destroy", onDestroy);

    DebuggerServer.destroy();
    started = false;
  });
}
