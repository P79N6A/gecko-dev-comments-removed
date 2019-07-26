



"use strict";

















function ContentAppActor(connection, browser)
{
  BrowserTabActor.call(this, connection, browser);
}

ContentAppActor.prototype = Object.create(BrowserTabActor.prototype);

ContentAppActor.prototype.constructor = ContentAppActor;

Object.defineProperty(ContentAppActor.prototype, "title", {
  get: function() {
    return this.browser.title;
  },
  enumerable: true,
  configurable: false
});

Object.defineProperty(ContentAppActor.prototype, "url", {
  get: function() {
    return this.browser.document.documentURI;
  },
  enumerable: true,
  configurable: false
});

Object.defineProperty(ContentAppActor.prototype, "window", {
  get: function() {
    return this.browser;
  },
  enumerable: true,
  configurable: false
});





ContentAppActor.prototype.grip = function () {
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

