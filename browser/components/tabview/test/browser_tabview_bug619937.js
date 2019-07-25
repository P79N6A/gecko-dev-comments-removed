


function test() {
  waitForExplicitFinish();

  window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
  TabView.toggle();
}

function endGame() {
  window.removeEventListener("tabviewhidden", endGame, false);
  ok(!TabView.isVisible(), "Tab View is hidden");

  finish();
};

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);
  ok(TabView.isVisible(), "Tab View is visible");

  let contentWindow = document.getElementById("tab-view").contentWindow;

  
  is(contentWindow.GroupItems.groupItems.length, 1,
      "we start with one group (the default)");
  is(gBrowser.tabs.length, 1, "we start with one tab");
  let originalTab = gBrowser.tabs[0];

  let bg = contentWindow.document.getElementById("bg");

  EventUtils.sendMouseEvent({ type: "click" }, bg, contentWindow);

  is(contentWindow.GroupItems.groupItems.length, 1,
      "we should still only have one group");

  window.addEventListener("tabviewhidden", endGame, false);
  TabView.toggle();
}
