



var gTab = null;

function load(url, cb) {
  gTab = gBrowser.addTab(url);
  gBrowser.addEventListener("load", function (event) {
    if (event.target.location != url)
      return;

    gBrowser.removeEventListener("load", arguments.callee, true);
    
    gBrowser.selectedTab = gTab;
    cb();
  }, true);
}

function test() {
  waitForExplicitFinish();

  ok(gFindBar.hidden, "Find bar should not be visible by default");

  
  EventUtils.synthesizeKey("f", { accelKey: true });
  ok(!gFindBar.hidden, "Find bar should be visible");

  nextTest();
}

let urls = [
  "about:config",
  "about:addons",
  "about:permissions"
];

function nextTest() {
  let url = urls.shift();
  if (url) {
    testFindDisabled(url, nextTest);
  } else {
    
    testFindEnabled("about:blank", function () {
      EventUtils.synthesizeKey("VK_ESCAPE", { });
      ok(gFindBar.hidden, "Find bar should now be hidden");
      finish();
    });
  }
}

function testFindDisabled(url, cb) {
  load(url, function() {
    ok(gFindBar.hidden, "Find bar should not be visible");
    EventUtils.synthesizeKey("/", {}, gTab.linkedBrowser.contentWindow);
    ok(gFindBar.hidden, "Find bar should not be visible");
    EventUtils.synthesizeKey("f", { accelKey: true });
    ok(gFindBar.hidden, "Find bar should not be visible");
    ok(document.getElementById("cmd_find").getAttribute("disabled"),
       "Find command should be disabled");

    gBrowser.removeTab(gTab);
    cb();
  });
}

function testFindEnabled(url, cb) {
  load(url, function() {
    ok(!document.getElementById("cmd_find").getAttribute("disabled"),
       "Find command should not be disabled");

    
    EventUtils.synthesizeKey("f", { accelKey: true });
    ok(!gFindBar.hidden, "Find bar should be visible again");
    EventUtils.synthesizeKey("VK_ESCAPE", { });
    ok(gFindBar.hidden, "Find bar should now be hidden");

    gBrowser.removeTab(gTab);
    cb();
  });
}
