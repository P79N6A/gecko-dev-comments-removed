







function test() {
  addTab("http://example.com/browser/browser/devtools/webconsole/" +
         "test/browser/test-bug-658368-time-methods.html");
  openConsole();
  browser.addEventListener("load", onLoad, true);
}

function onLoad(aEvent) {
  browser.removeEventListener(aEvent.type, onLoad, true);

  let hudId = HUDService.getHudIdByWindow(content);
  let hud = HUDService.hudReferences[hudId];
  outputNode = hud.outputNode;

  executeSoon(function() {
    findLogEntry("aTimer: timer started");
    findLogEntry("ms");

    
    
    addTab("data:text/html,<script type='text/javascript'>" +
           "console.timeEnd('bTimer');</script>");
    openConsole();
    browser.addEventListener("load", testTimerIndependenceInTabs, true);
  });
}

function testTimerIndependenceInTabs(aEvent) {
  browser.removeEventListener(aEvent.type, testTimerIndependenceInTabs, true);

  let hudId = HUDService.getHudIdByWindow(content);
  let hud = HUDService.hudReferences[hudId];
  outputNode = hud.outputNode;

  executeSoon(function() {
    testLogEntry(outputNode, "bTimer: timer started", "bTimer was not started",
                 false, true);

    
    
    browser.addEventListener("load", testTimerIndependenceInSameTab, true);
    content.location = "data:text/html,<script type='text/javascript'>" +
           "console.time('bTimer');</script>";
  });
}

function testTimerIndependenceInSameTab(aEvent) {
  browser.removeEventListener(aEvent.type, testTimerIndependenceInSameTab, true);

  let hudId = HUDService.getHudIdByWindow(content);
  let hud = HUDService.hudReferences[hudId];
  outputNode = hud.outputNode;

  executeSoon(function() {
    findLogEntry("bTimer: timer started");
    hud.jsterm.clearOutput();

    
    
    browser.addEventListener("load", testTimerIndependenceInSameTabAgain, true);
    content.location = "data:text/html,<script type='text/javascript'>" +
           "console.timeEnd('bTimer');</script>";
  });
}

function testTimerIndependenceInSameTabAgain(aEvent) {
  browser.removeEventListener(aEvent.type, testTimerIndependenceInSameTabAgain, true);

  let hudId = HUDService.getHudIdByWindow(content);
  let hud = HUDService.hudReferences[hudId];
  outputNode = hud.outputNode;

  executeSoon(function() {
    testLogEntry(outputNode, "bTimer: timer started", "bTimer was not started",
                 false, true);

    finishTest();
  });
}
