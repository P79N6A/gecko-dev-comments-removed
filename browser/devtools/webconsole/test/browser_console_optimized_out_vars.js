





function test() {
  Task.spawn(function* () {
    const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-closure-optimized-out.html";
    let {tab} = yield loadTab(TEST_URI);
    let hud = yield openConsole(tab);
    let { toolbox, panel, panelWin } = yield openDebugger();

    yield waitForThreadEvents(panel, "resumed");
    ok(true, "Debugger resumed");

    let sources = panelWin.DebuggerView.Sources;
    yield panel.addBreakpoint({ url: sources.values[0], line: 18 });
    yield ensureThreadClientState(panel, "resumed");

    let fetchedScopes = panelWin.once(panelWin.EVENTS.FETCHED_SCOPES);
    let button = content.document.querySelector("button");
    ok(button, "Button element found");
    
    
    executeSoon(() => button.click());

    let packet = yield fetchedScopes;
    ok(true, "Scopes were fetched");

    yield toolbox.selectTool("webconsole");

    
    hud.jsterm.execute("upvar");
    yield waitForMessages({
            webconsole: hud,
            messages: [{
              text: "optimized out",
              category: CATEGORY_OUTPUT,
            }]
          });

    finishTest();
  }).then(null, aError => {
    ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
  });
}



function ensureThreadClientState(aPanel, aState) {
  let thread = aPanel.panelWin.gThreadClient;
  let state = thread.state;

  info("Thread is: '" + state + "'.");

  if (state == aState) {
    return promise.resolve(null);
  } else {
    return waitForThreadEvents(aPanel, aState);
  }
}

function waitForThreadEvents(aPanel, aEventName, aEventRepeat = 1) {
  info("Waiting for thread event: '" + aEventName + "' to fire: " + aEventRepeat + " time(s).");

  let deferred = promise.defer();
  let thread = aPanel.panelWin.gThreadClient;
  let count = 0;

  thread.addListener(aEventName, function onEvent(aEventName, ...aArgs) {
    info("Thread event '" + aEventName + "' fired: " + (++count) + " time(s).");

    if (count == aEventRepeat) {
      ok(true, "Enough '" + aEventName + "' thread events have been fired.");
      thread.removeListener(aEventName, onEvent);
      deferred.resolve.apply(deferred, aArgs);
    }
  });

  return deferred.promise;
}
