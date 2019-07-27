



"use strict";

let { TabActor } = require("devtools/server/actors/webbrowser");

















function ContentActor(connection, chromeGlobal)
{
  this._chromeGlobal = chromeGlobal;
  TabActor.call(this, connection, chromeGlobal);
  this.traits.reconfigure = false;
  this._sendForm = this._sendForm.bind(this);
  this._chromeGlobal.addMessageListener("debug:form", this._sendForm);

  Object.defineProperty(this, "docShell", {
    value: this._chromeGlobal.docShell,
    configurable: true
  });
}

ContentActor.prototype = Object.create(TabActor.prototype);

ContentActor.prototype.constructor = ContentActor;

Object.defineProperty(ContentActor.prototype, "title", {
  get: function() {
    return this.window.document.title;
  },
  enumerable: true,
  configurable: true
});

ContentActor.prototype.exit = function() {
  this._chromeGlobal.removeMessageListener("debug:form", this._sendForm);
  this._sendForm = null;
  TabActor.prototype.exit.call(this);
};





ContentActor.prototype.form = function () {
  let response = {
    "actor": this.actorID,
    "title": this.title,
    "url": this.url
  };

  
  let actorPool = new ActorPool(this.conn);
  this._createExtraActors(DebuggerServer.tabActorFactories, actorPool);
  if (!actorPool.isEmpty()) {
    this._tabActorPool2 = actorPool;
    this.conn.addActorPool(this._tabActorPool2);
  }

  this._appendExtraActors(response);
  return response;
};





ContentActor.prototype._sendForm = function() {
  this._chromeGlobal.sendAsyncMessage("debug:form", this.form());
};
