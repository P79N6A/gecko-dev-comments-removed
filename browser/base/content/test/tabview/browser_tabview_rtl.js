



function test() {
  waitForExplicitFinish();

  
  ok(!TabView.isVisible(), "Tab View starts hidden");

  showTabView(onTabViewLoadedAndShown("ltr"));
}


function onTabViewLoadedAndShown(dir) {
  return function() {
    ok(TabView.isVisible(), "Tab View is visible.");

    let contentWindow = document.getElementById("tab-view").contentWindow;
    let contentDocument = contentWindow.document;
    is(contentDocument.documentElement.getAttribute("dir"), dir,
       "The direction should be set to " + dir.toUpperCase());

    
    hideTabView(onTabViewHidden(dir));
  };
}


function onTabViewHidden(dir) {
  return function() {
    ok(!TabView.isVisible(), "Tab View is hidden.");

    if (dir == "ltr") {
      
      Services.prefs.setCharPref("intl.uidirection.en-US", "rtl");

      showTabView(onTabViewLoadedAndShown("rtl"));
    } else {
      
      Services.prefs.clearUserPref("intl.uidirection.en-US");

      finish();
    }
  };
}
