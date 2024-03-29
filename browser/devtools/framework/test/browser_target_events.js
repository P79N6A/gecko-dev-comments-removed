



var target;

function test()
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", onLoad, true);
}

function onLoad(evt) {
  gBrowser.selectedBrowser.removeEventListener(evt.type, onLoad, true);

  target = TargetFactory.forTab(gBrowser.selectedTab);

  is(target.tab, gBrowser.selectedTab, "Target linked to the right tab.");

  target.once("hidden", onHidden);
  gBrowser.selectedTab = gBrowser.addTab();
}

function onHidden() {
  ok(true, "Hidden event received");
  target.once("visible", onVisible);
  gBrowser.removeCurrentTab();
}

function onVisible() {
  ok(true, "Visible event received");
  target.once("will-navigate", onWillNavigate);
  let mm = getFrameScript();
  mm.sendAsyncMessage("devtools:test:navigate", { location: "data:text/html,<meta charset='utf8'/>test navigation" });
}

function onWillNavigate(event, request) {
  ok(true, "will-navigate event received");
  
  
  target.once("navigate", executeSoon.bind(null, onNavigate));
}

function onNavigate() {
  ok(true, "navigate event received");
  target.once("close", onClose);
  gBrowser.removeCurrentTab();
}

function onClose() {
  ok(true, "close event received");

  target = null;
  finish();
}
