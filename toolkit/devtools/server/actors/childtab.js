



"use strict";

















function ContentTabActor(connection, browser)
{
  BrowserTabActor.call(this, connection, browser);
}

ContentTabActor.prototype = Object.create(BrowserTabActor.prototype);

ContentTabActor.prototype.constructor = ContentTabActor;

Object.defineProperty(ContentTabActor.prototype, "title", {
  get: function() {
    return this.browser.title;
  },
  enumerable: true,
  configurable: false
});

Object.defineProperty(ContentTabActor.prototype, "url", {
  get: function() {
    return this.browser.document.documentURI;
  },
  enumerable: true,
  configurable: false
});

Object.defineProperty(ContentTabActor.prototype, "contentWindow", {
  get: function() {
    return this.browser;
  },
  enumerable: true,
  configurable: false
});





ContentTabActor.prototype.grip = function () {
  let response = {
    'actor': this.actorID,
    'title': this.title,
    'url': this.url
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

