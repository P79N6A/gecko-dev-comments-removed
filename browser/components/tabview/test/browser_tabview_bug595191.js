


function test() {
  waitForExplicitFinish();

  
  window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
  ok(!TabView.isVisible(), "Tab View is hidden");
  TabView.toggle();
}

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);

  ok(TabView.isVisible(), "Tab View is visible");

  let contentWindow = document.getElementById("tab-view").contentWindow;
  let searchButton = contentWindow.document.getElementById("searchbutton");

  ok(searchButton, "Search button exists");
  
  let onSearchEnabled = function() {
    contentWindow.removeEventListener(
      "tabviewsearchenabled", onSearchEnabled, false);
    let search = contentWindow.document.getElementById("search");
    ok(search.style.display != "none", "Search is enabled");
    escapeTest(contentWindow);
  }
  contentWindow.addEventListener("tabviewsearchenabled", onSearchEnabled, 
    false);
  
  EventUtils.sendMouseEvent({ type: "mousedown" }, searchButton, 
    contentWindow);
}

function escapeTest(contentWindow) {  
  let onSearchDisabled = function() {
    contentWindow.removeEventListener(
      "tabviewsearchdisabled", onSearchDisabled, false);

    let search = contentWindow.document.getElementById("search");
    ok(search.style.display == "none", "Search is disabled");
    toggleTabViewTest(contentWindow);
  }
  contentWindow.addEventListener("tabviewsearchdisabled", onSearchDisabled, 
    false);
  EventUtils.synthesizeKey("VK_ESCAPE", { }, contentWindow);
}

function toggleTabViewTest(contentWindow) {
  let onTabViewHidden = function() {
    contentWindow.removeEventListener("tabviewhidden", onTabViewHidden, false);

    ok(!TabView.isVisible(), "Tab View is hidden");
    finish();
  }
  contentWindow.addEventListener("tabviewhidden", onTabViewHidden, false);
  
  EventUtils.synthesizeKey("e", { accelKey: true, shiftKey: true });
}
