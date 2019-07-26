






const TAB_URL = EXAMPLE_URL + "doc_inline-debugger-statement.html";

let gClient;

function test() {
  if (!DebuggerServer.initialized) {
    DebuggerServer.init(() => true);
    DebuggerServer.addBrowserActors();
  }

  let transport = DebuggerServer.connectPipe();
  gClient = new DebuggerClient(transport);
  gClient.connect((aType, aTraits) => {
    is(aType, "browser",
      "Root actor should identify itself as a browser.");

    addTab(TAB_URL)
      .then(() => attachTabActorForUrl(gClient, TAB_URL))
      .then(testEarlyDebuggerStatement)
      .then(testDebuggerStatement)
      .then(closeConnection)
      .then(finish)
      .then(null, aError => {
        ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
      });
  });
}

function testEarlyDebuggerStatement([aGrip, aResponse]) {
  let deferred = promise.defer();

  let onPaused = function(aEvent, aPacket) {
    ok(false, "Pause shouldn't be called before we've attached!");
    deferred.reject();
  };

  gClient.addListener("paused", onPaused);

  
  
  let debuggee = gBrowser.selectedTab.linkedBrowser.contentWindow.wrappedJSObject;
  debuggee.runDebuggerStatement();

  gClient.removeListener("paused", onPaused);

  
  gClient.request({ to: aResponse.threadActor, type: "attach" }, () => {
    gClient.request({ to: aResponse.threadActor, type: "resume" }, () => {
      ok(true, "Pause wasn't called before we've attached.");
      deferred.resolve([aGrip, aResponse]);
    });
  });

  return deferred.promise;
}

function testDebuggerStatement([aGrip, aResponse]) {
  let deferred = promise.defer();

  gClient.addListener("paused", (aEvent, aPacket) => {
    gClient.request({ to: aResponse.threadActor, type: "resume" }, () => {
      ok(true, "The pause handler was triggered on a debugger statement.");
      deferred.resolve();
    });
  });

  
  let debuggee = gBrowser.selectedTab.linkedBrowser.contentWindow.wrappedJSObject;
  debuggee.runDebuggerStatement();
}

function closeConnection() {
  let deferred = promise.defer();
  gClient.close(deferred.resolve);
  return deferred.promise;
}

registerCleanupFunction(function() {
  removeTab(gBrowser.selectedTab);
  gClient = null;
});

