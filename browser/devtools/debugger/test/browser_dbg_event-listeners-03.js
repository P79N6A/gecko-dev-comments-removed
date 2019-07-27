







const TAB_URL = EXAMPLE_URL + "doc_native-event-handler.html";

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
      .then(() => attachThreadActorForUrl(gClient, TAB_URL))
      .then(pauseDebuggee)
      .then(testEventListeners)
      .then(closeConnection)
      .then(finish)
      .then(null, aError => {
        ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
      });
  });
}

function pauseDebuggee(aThreadClient) {
  let deferred = promise.defer();

  gClient.addOneTimeListener("paused", (aEvent, aPacket) => {
    is(aPacket.type, "paused",
      "We should now be paused.");
    is(aPacket.why.type, "debuggerStatement",
      "The debugger statement was hit.");

    deferred.resolve(aThreadClient);
  });

  
  
  executeSoon(() => {
    EventUtils.sendMouseEvent({ type: "click" },
      content.document.querySelector("button"),
      content);
  });

  return deferred.promise;
}

function testEventListeners(aThreadClient) {
  let deferred = promise.defer();

  aThreadClient.eventListeners(aPacket => {
    if (aPacket.error) {
      let msg = "Error getting event listeners: " + aPacket.message;
      ok(false, msg);
      deferred.reject(msg);
      return;
    }

    
    
    is(aPacket.listeners.length, 4, "Found all event listeners.");
    aThreadClient.resume(deferred.resolve);
  });

  return deferred.promise;
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
