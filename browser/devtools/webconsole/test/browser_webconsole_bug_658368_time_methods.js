







function test() {
  addTab("http://example.com/browser/browser/devtools/webconsole/" +
         "test/test-bug-658368-time-methods.html");
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    Task.spawn(runner);
  }, true);

  function* runner() {
    let hud1 = yield openConsole();

    yield waitForMessages({
      webconsole: hud1,
      messages: [{
        name: "aTimer started",
        consoleTime: "aTimer",
      }, {
        name: "aTimer end",
        consoleTimeEnd: "aTimer",
      }],
    });

    let deferred = promise.defer();

    
    
    addTab("data:text/html;charset=utf-8,<script>" +
           "console.timeEnd('bTimer');</script>");
    browser.addEventListener("load", function onLoad() {
      browser.removeEventListener("load", onLoad, true);
      openConsole().then((hud) => {
        deferred.resolve(hud);
      });
    }, true);

    let hud2 = yield deferred.promise;

    testLogEntry(hud2.outputNode, "bTimer: timer started",
                 "bTimer was not started", false, true);

    
    
    content.location = "data:text/html;charset=utf-8,<script>" +
                       "console.time('bTimer');</script>";

    yield waitForMessages({
      webconsole: hud2,
      messages: [{
        name: "bTimer started",
        consoleTime: "bTimer",
      }],
    });

    hud2.jsterm.clearOutput();

    deferred = promise.defer();

    
    
    browser.addEventListener("load", function onLoad() {
      browser.removeEventListener("load", onLoad, true);
      deferred.resolve(null);
    }, true);

    content.location = "data:text/html;charset=utf-8," +
                       "<script>console.timeEnd('bTimer');</script>";

    yield deferred.promise;

    testLogEntry(hud2.outputNode, "bTimer: timer started",
                 "bTimer was not started", false, true);

    yield closeConsole(gBrowser.selectedTab);

    gBrowser.removeCurrentTab();

    executeSoon(finishTest);
  }
}
