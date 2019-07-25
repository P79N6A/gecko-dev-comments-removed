




































let tabViewShownCount = 0;


function test() {
  waitForExplicitFinish();

  
  ok(!TabView.isVisible(), "Tab View starts hidden");

  
  window.addEventListener("tabviewshown", onTabViewLoadedAndShown("ltr"), false);
  toggleTabView();
}

function toggleTabView() {
  let button = document.getElementById("tabview-button");
  ok(button, "Tab View button exists");
  button.doCommand();
}


function onTabViewLoadedAndShown(dir) {
  return function() {
    window.removeEventListener("tabviewshown", arguments.callee, false);
    ok(TabView.isVisible(), "Tab View is visible.");

    let contentWindow = document.getElementById("tab-view").contentWindow;
    let contentDocument = contentWindow.document;
    is(contentDocument.documentElement.getAttribute("dir"), dir,
       "The direction should be set to " + dir.toUpperCase());

    
    window.addEventListener("tabviewhidden", onTabViewHidden(dir), false);
    TabView.toggle();
  };
}


function onTabViewHidden(dir) {
  return function() {
    window.removeEventListener("tabviewhidden", arguments.callee, false);
    ok(!TabView.isVisible(), "Tab View is hidden.");

    if (dir == "ltr") {
      
      Services.prefs.setCharPref("intl.uidirection.en-US", "rtl");

      
      window.addEventListener("tabviewshown", onTabViewLoadedAndShown("rtl"), false);
      toggleTabView();
    } else {
      
      Services.prefs.clearUserPref("intl.uidirection.en-US");

      finish();
    }
  };
}

