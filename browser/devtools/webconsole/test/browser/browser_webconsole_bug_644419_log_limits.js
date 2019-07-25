








const TEST_URI = "http://example.com/browser/browser/devtools/" +
                 "webconsole/test/browser/test-bug-644419-log-limits.html";

var gOldPref, gHudId;

function test() {
  addTab("data:text/html,Web Console test for bug 644419: Console should " +
         "have user-settable log limits for each message category");
  browser.addEventListener("load", onLoad, true);
}

function onLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  openConsole();

  gHudId = HUDService.getHudIdByWindow(content);
  browser.addEventListener("load", testWebDevLimits, true);
  expectUncaughtException();
  content.location = TEST_URI;
}

function testWebDevLimits(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);
  gOldPref = Services.prefs.getIntPref("devtools.hud.loglimit.console");
  Services.prefs.setIntPref("devtools.hud.loglimit.console", 10);

  let hud = HUDService.hudReferences[gHudId];
  outputNode = hud.outputNode;

  executeSoon(function() {
    
    findLogEntry("bar is not defined");

    
    for (let i = 0; i < 11; i++) {
      hud.console.log("test message " + i);
    }
    testLogEntry(outputNode, "test message 0", "first message is pruned", false, true);
    findLogEntry("test message 1");
    
    findLogEntry("bar is not defined");

    Services.prefs.setIntPref("devtools.hud.loglimit.console", gOldPref);
    testJsLimits();
  });
}

function testJsLimits(aEvent) {
  gOldPref = Services.prefs.getIntPref("devtools.hud.loglimit.exception");
  Services.prefs.setIntPref("devtools.hud.loglimit.exception", 10);

  let hud = HUDService.hudReferences[gHudId];
  hud.jsterm.clearOutput();
  outputNode = hud.outputNode;
  hud.console.log("testing JS limits");

  
  findLogEntry("testing JS limits");
  
  let head = content.document.getElementsByTagName("head")[0];
  for (let i = 0; i < 11; i++) {
    var script = content.document.createElement("script");
    script.text = "fubar" + i + ".bogus(6);";
    expectUncaughtException();
    head.insertBefore(script, head.firstChild);
  }

  executeSoon(function() {
    testLogEntry(outputNode, "fubar0 is not defined", "first message is pruned", false, true);
    findLogEntry("fubar1 is not defined");
    
    findLogEntry("testing JS limits");

    Services.prefs.setIntPref("devtools.hud.loglimit.exception", gOldPref);
    testNetLimits();
  });
}

var gCounter, gImage;

function testNetLimits(aEvent) {
  gOldPref = Services.prefs.getIntPref("devtools.hud.loglimit.network");
  Services.prefs.setIntPref("devtools.hud.loglimit.network", 10);

  let hud = HUDService.hudReferences[gHudId];
  hud.jsterm.clearOutput();
  outputNode = hud.outputNode;
  hud.console.log("testing Net limits");

  
  findLogEntry("testing Net limits");
  
  gCounter = 0;
  loadImage();
}

function loadImage() {
  if (gCounter < 11) {
    let body = content.document.getElementsByTagName("body")[0];
    gImage && gImage.removeEventListener("load", loadImage, true);
    gImage = content.document.createElement("img");
    gImage.src = "test-image.png?_fubar=" + gCounter;
    body.insertBefore(gImage, body.firstChild);
    gImage.addEventListener("load", loadImage, true);
    gCounter++;
    return;
  }
  is(gCounter, 11, "loaded 11 files");
  testLogEntry(outputNode, "test-image.png?_fubar=0", "first message is pruned", false, true);
  findLogEntry("test-image.png?_fubar=1");
  
  findLogEntry("testing Net limits");

  Services.prefs.setIntPref("devtools.hud.loglimit.network", gOldPref);
  testCssLimits();
}

function testCssLimits(aEvent) {
  gOldPref = Services.prefs.getIntPref("devtools.hud.loglimit.cssparser");
  Services.prefs.setIntPref("devtools.hud.loglimit.cssparser", 10);

  let hud = HUDService.hudReferences[gHudId];
  hud.jsterm.clearOutput();
  outputNode = hud.outputNode;
  hud.console.log("testing CSS limits");

  
  findLogEntry("testing CSS limits");

  
  let body = content.document.getElementsByTagName("body")[0];
  for (let i = 0; i < 11; i++) {
    var div = content.document.createElement("div");
    div.setAttribute("style", "-moz-foobar" + i + ": 42;");
    body.insertBefore(div, body.firstChild);
  }
  executeSoon(function() {
    testLogEntry(outputNode, "Unknown property '-moz-foobar0'", "first message is pruned", false, true);
    findLogEntry("Unknown property '-moz-foobar1'");
    
    findLogEntry("testing CSS limits");

    Services.prefs.setIntPref("devtools.hud.loglimit.cssparser", gOldPref);
    finishTest();
  });
}
