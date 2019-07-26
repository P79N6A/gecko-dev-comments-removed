






const TAB_URL = EXAMPLE_URL + "doc_event-listeners.html";

let gClient, gThreadClient, gInput, gButton;

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
      .then(setupGlobals)
      .then(pauseDebuggee)
      .then(testBreakOnAll)
      .then(testBreakOnDisabled)
      .then(testBreakOnNone)
      .then(testBreakOnClick)
      .then(closeConnection)
      .then(finish)
      .then(null, aError => {
        ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
      });
  });
}

function setupGlobals(aThreadClient) {
  gThreadClient = aThreadClient;
  gInput = content.document.querySelector("input");
  gButton = content.document.querySelector("button");
}

function pauseDebuggee() {
  let deferred = promise.defer();

  gClient.addOneTimeListener("paused", (aEvent, aPacket) => {
    is(aPacket.type, "paused",
      "We should now be paused.");
    is(aPacket.why.type, "debuggerStatement",
      "The debugger statement was hit.");

    deferred.resolve();
  });

  
  
  executeSoon(triggerButtonClick);

  return deferred.promise;
}


function testBreakOnAll() {
  let deferred = promise.defer();

  
  gThreadClient.pauseOnDOMEvents("*", (aPacket) => {
    is(aPacket, undefined,
      "The pause-on-any-event request completed successfully.");

    gClient.addOneTimeListener("paused", (aEvent, aPacket) => {
      is(aPacket.why.type, "pauseOnDOMEvents",
        "A hidden breakpoint was hit.");
      is(aPacket.frame.callee.name, "keyupHandler",
        "The keyupHandler is entered.");

      gClient.addOneTimeListener("paused", (aEvent, aPacket) => {
        is(aPacket.why.type, "pauseOnDOMEvents",
          "A hidden breakpoint was hit.");
        is(aPacket.frame.callee.name, "clickHandler",
          "The clickHandler is entered.");

        gClient.addOneTimeListener("paused", (aEvent, aPacket) => {
          is(aPacket.why.type, "pauseOnDOMEvents",
            "A hidden breakpoint was hit.");
          is(aPacket.frame.callee.name, "onchange",
            "The onchange handler is entered.");

          gThreadClient.resume(deferred.resolve);
        });

        gThreadClient.resume(triggerInputChange);
      });

      gThreadClient.resume(triggerButtonClick);
    });

    gThreadClient.resume(triggerInputKeyup);
  });

  return deferred.promise;
}


function testBreakOnDisabled() {
  let deferred = promise.defer();

  
  gThreadClient.pauseOnDOMEvents(["click"], (aPacket) => {
    is(aPacket.error, undefined,
      "The pause-on-click-only request completed successfully.");

    gClient.addListener("paused", unexpectedListener);

    
    
    once(gInput, "keyup").then(() => {
      is(content.wrappedJSObject.foobar, "keyupHandler",
        "No hidden breakpoint was hit.");

      gClient.removeListener("paused", unexpectedListener);
      deferred.resolve();
    });

    triggerInputKeyup();
  });

  return deferred.promise;
}


function testBreakOnNone() {
  let deferred = promise.defer();

  
  gThreadClient.pauseOnDOMEvents([], (aPacket) => {
    is(aPacket.error, undefined,
      "The pause-on-none request completed successfully.");

    gClient.addListener("paused", unexpectedListener);

    
    
    once(gInput, "keyup").then(() => {
      is(content.wrappedJSObject.foobar, "keyupHandler",
        "No hidden breakpoint was hit.");

      gClient.removeListener("paused", unexpectedListener);
      deferred.resolve();
    });

    triggerInputKeyup();
  });

  return deferred.promise;
}


function testBreakOnClick() {
  let deferred = promise.defer();

  
  gThreadClient.pauseOnDOMEvents(["click"], (aPacket) => {
    is(aPacket.error, undefined,
      "The pause-on-click request completed successfully.");

    gClient.addOneTimeListener("paused", (aEvent, aPacket) => {
      is(aPacket.why.type, "pauseOnDOMEvents",
        "A hidden breakpoint was hit.");
      is(aPacket.frame.callee.name, "clickHandler",
        "The clickHandler is entered.");

      gThreadClient.resume(deferred.resolve);
    });

    triggerButtonClick();
  });

  return deferred.promise;
}

function closeConnection() {
  let deferred = promise.defer();
  gClient.close(deferred.resolve);
  return deferred.promise;
}

function unexpectedListener() {
  gClient.removeListener("paused", unexpectedListener);
  ok(false, "An unexpected hidden breakpoint was hit.");
  gThreadClient.resume(testBreakOnClick);
}

function triggerInputKeyup() {
  
  
  window.focus();
  gBrowser.selectedBrowser.focus();
  gButton.focus();

  
  once(gInput, "focus").then(() => {
    executeSoon(() => {
      EventUtils.synthesizeKey("e", { shiftKey: 1 }, content);
    });
  });

  gInput.focus();
}

function triggerButtonClick() {
  EventUtils.sendMouseEvent({ type: "click" }, gButton);
}

function triggerInputChange() {
  gInput.focus();
  gInput.value = "foo";
  gInput.blur();
}

registerCleanupFunction(function() {
  removeTab(gBrowser.selectedTab);
  gClient = null;
  gThreadClient = null;
  gInput = null;
  gButton = null;
});
