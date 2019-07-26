





'use strict';















function createRootActor(connection)
{
  let parameters = {
#ifndef MOZ_WIDGET_GONK
    tabList: new ContentTabList(connection),
#else
    tabList: [],
#endif
    globalActorFactories: DebuggerServer.globalActorFactories,
    onShutdown: sendShutdownEvent
  };
  let root = new RootActor(connection, parameters);
  root.applicationType = "operating-system";
  return root;
}












function ContentTabList(connection)
{
  BrowserTabList.call(this, connection);
}

ContentTabList.prototype = Object.create(BrowserTabList.prototype);

ContentTabList.prototype.constructor = ContentTabList;

ContentTabList.prototype.iterator = function() {
  let browser = Services.wm.getMostRecentWindow('navigator:browser');
  
  let actor = this._actorByBrowser.get(browser);
  if (!actor) {
    actor = new ContentTabActor(this._connection, browser);
    this._actorByBrowser.set(browser, actor);
    actor.selected = true;
  }

  yield actor;
};

ContentTabList.prototype.onCloseWindow = makeInfallible(function(aWindow) {
  




  Services.tm.currentThread.dispatch(makeInfallible(() => {
    



    for (let [browser, actor] of this._actorByBrowser) {
      this._handleActorClose(actor, browser);
    }
  }, "ContentTabList.prototype.onCloseWindow's delayed body"), 0);
}, "ContentTabList.prototype.onCloseWindow");











function ContentTabActor(connection, browser)
{
  BrowserTabActor.call(this, connection, browser);
}

ContentTabActor.prototype.constructor = ContentTabActor;

ContentTabActor.prototype = Object.create(BrowserTabActor.prototype);

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

Object.defineProperty(ContentTabActor.prototype, "window", {
  get: function() {
    return this.browser;
  },
  enumerable: true,
  configurable: false
});
