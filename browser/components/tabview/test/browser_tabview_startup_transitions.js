


var prefsBranch = Cc["@mozilla.org/preferences-service;1"].
                  getService(Ci.nsIPrefService).
                  getBranch("browser.panorama.");

function animateZoom() prefsBranch.getBoolPref("animate_zoom");

function test() {
  waitForExplicitFinish();

  let win = window.openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no",
                              "about:blank", null, null, null, true);

  registerCleanupFunction(function() {
    prefsBranch.setBoolPref("animate_zoom", true);
    win.close();
  });

  ok(animateZoom(), "By default, we animate on zoom.");
  prefsBranch.setBoolPref("animate_zoom", false);
  ok(!animateZoom(), "animate_zoom = false");
  
  let onLoad = function() {
    win.removeEventListener("load", onLoad, false);

    
    let tabViewWindow = null;
    let transitioned = 0;

    let initCallback = function() {
      tabViewWindow = win.TabView.getContentWindow();
      function onTransitionEnd(event) {
        transitioned++;
        info(transitioned);
      }
      tabViewWindow.document.addEventListener("transitionend", onTransitionEnd, false);

      
      
      
      let onTabViewShown = function() {
        tabViewWindow.removeEventListener("tabviewshown", onTabViewShown, false);
        tabViewWindow.document.removeEventListener("transitionend", onTransitionEnd, false);

        ok(!transitioned, "There should be no transitions");

        finish();
      };
      tabViewWindow.addEventListener("tabviewshown", onTabViewShown, false);
      win.TabView.toggle();
    };

    win.TabView._initFrame(initCallback);
  }
  win.addEventListener("load", onLoad, false);
}
