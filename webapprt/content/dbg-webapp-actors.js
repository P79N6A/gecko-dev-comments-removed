



'use strict';

let promise = Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js", {}).Promise;
















function createRootActor(connection)
{
  let parameters = {
    tabList: new WebappTabList(connection),
    globalActorFactories: DebuggerServer.globalActorFactories,
    onShutdown: sendShutdownEvent
  };
  return new RootActor(connection, parameters);
}












function WebappTabList(connection)
{
  BrowserTabList.call(this, connection);
}

WebappTabList.prototype = Object.create(BrowserTabList.prototype);

WebappTabList.prototype.constructor = WebappTabList;

WebappTabList.prototype.getList = function() {
  let topXULWindow = Services.wm.getMostRecentWindow(this._windowType);

  
  
  let initialMapSize = this._actorByBrowser.size;
  let foundCount = 0;

  
  
  
  

  
  for (let win of allAppShellDOMWindows(this._windowType)) {
    let browser = win.document.getElementById("content");
    if (!browser) {
      continue;
    }

    
    let actor = this._actorByBrowser.get(browser);
    if (actor) {
      foundCount++;
    } else {
      actor = new WebappTabActor(this._connection, browser);
      this._actorByBrowser.set(browser, actor);
    }

    actor.selected = (win == topXULWindow);
  }

  if (this._testing && initialMapSize !== foundCount) {
    throw Error("_actorByBrowser map contained actors for dead tabs");
  }

  this._mustNotify = true;
  this._checkListening();

  return promise.resolve([actor for ([_, actor] of this._actorByBrowser)]);
};
















function WebappTabActor(connection, browser)
{
  BrowserTabActor.call(this, connection, browser);
}

WebappTabActor.prototype.constructor = WebappTabActor;

WebappTabActor.prototype = Object.create(BrowserTabActor.prototype);

Object.defineProperty(WebappTabActor.prototype, "title", {
  get: function() {
    return this.browser.ownerDocument.defaultView.document.title;
  },
  enumerable: true,
  configurable: false
});
