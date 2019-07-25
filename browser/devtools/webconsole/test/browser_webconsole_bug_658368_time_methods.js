







function test() {
  addTab("http://example.com/browser/browser/devtools/webconsole/" +
         "test/test-bug-658368-time-methods.html");
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

function consoleOpened(hud) {
  outputNode = hud.outputNode;

  executeSoon(function() {
    findLogEntry("aTimer: timer started");
    findLogEntry("ms");

    
    
    addTab("data:text/html;charset=utf-8,<script type='text/javascript'>" +
           "console.timeEnd('bTimer');</script>");
    browser.addEventListener("load", function onLoad() {
      browser.removeEventListener("load", onLoad, true);
      openConsole(null, testTimerIndependenceInTabs);
    }, true);
  });
}

function testTimerIndependenceInTabs(hud) {
  outputNode = hud.outputNode;

  executeSoon(function() {
    testLogEntry(outputNode, "bTimer: timer started", "bTimer was not started",
                 false, true);

    
    
    browser.addEventListener("load", function onLoad() {
      browser.removeEventListener("load", onLoad, true);
      executeSoon(testTimerIndependenceInSameTab);
    }, true);
    content.location = "data:text/html;charset=utf-8,<script type='text/javascript'>" +
           "console.time('bTimer');</script>";
  });
}

function testTimerIndependenceInSameTab() {
  let hudId = HUDService.getHudIdByWindow(content);
  let hud = HUDService.hudReferences[hudId];
  outputNode = hud.outputNode;

  executeSoon(function() {
    findLogEntry("bTimer: timer started");
    hud.jsterm.clearOutput();

    
    
    browser.addEventListener("load", function onLoad() {
      browser.removeEventListener("load", onLoad, true);
      executeSoon(testTimerIndependenceInSameTabAgain);
    }, true);
    content.location = "data:text/html;charset=utf-8,<script type='text/javascript'>" +
           "console.timeEnd('bTimer');</script>";
  });
}

function testTimerIndependenceInSameTabAgain(hud) {
  let hudId = HUDService.getHudIdByWindow(content);
  let hud = HUDService.hudReferences[hudId];
  outputNode = hud.outputNode;

  executeSoon(function() {
    testLogEntry(outputNode, "bTimer: timer started", "bTimer was not started",
                 false, true);

    closeConsole(gBrowser.selectedTab, function() {
      gBrowser.removeCurrentTab();
      executeSoon(finishTest);
    });
  });
}
