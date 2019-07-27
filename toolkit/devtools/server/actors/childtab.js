



"use strict";

let { TabActor } = require("devtools/server/actors/webbrowser");




















function ContentActor(connection, chromeGlobal, prefix)
{
  this._chromeGlobal = chromeGlobal;
  this._prefix = prefix;
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
  if (this._sendForm) {
    this._chromeGlobal.removeMessageListener("debug:form", this._sendForm);
    this._sendForm = null;
  }
  return TabActor.prototype.exit.call(this);
};





ContentActor.prototype._sendForm = function() {
  this._chromeGlobal.sendAsyncMessage("debug:form", this.form());
};
