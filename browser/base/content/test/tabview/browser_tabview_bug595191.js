




































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
    let search = contentWindow.document.getElementById("search");
    ok(search.style.display != "none", "Search is enabled");
    contentWindow.removeEventListener(
      "tabviewsearchenabled", onSearchEnabled, false);
    escapeTest(contentWindow);
  }
  contentWindow.addEventListener("tabviewsearchenabled", onSearchEnabled, 
    false);
  
  EventUtils.sendMouseEvent({ type: "mousedown" }, searchButton, 
    contentWindow);
}

function escapeTest(contentWindow) {  
  let onSearchDisabled = function() {
    let search = contentWindow.document.getElementById("search");

    ok(search.style.display == "none", "Search is disabled");

    contentWindow.removeEventListener(
      "tabviewsearchdisabled", onSearchDisabled, false);
    toggleTabViewTest(contentWindow);
  }
  contentWindow.addEventListener("tabviewsearchdisabled", onSearchDisabled, 
    false);
  
  
  setTimeout( function() {
    EventUtils.synthesizeKey("VK_ESCAPE", {});
  }, 0);
}

function toggleTabViewTest(contentWindow) {
  let onTabViewHidden = function() {
    contentWindow.removeEventListener("tabviewhidden", onTabViewHidden, false);

    ok(!TabView.isVisible(), "Tab View is hidden");

    finish();
  }
  contentWindow.addEventListener("tabviewhidden", onTabViewHidden, false);
  
  
  setTimeout( function() {
    
    if(navigator.platform.indexOf("Mac") >= 0) {
      EventUtils.synthesizeKey("VK_SPACE", {altKey : true}, contentWindow);
    } else {
      EventUtils.synthesizeKey("VK_SPACE", {ctrlKey : true}, contentWindow);
    }
  }, 0);
}
