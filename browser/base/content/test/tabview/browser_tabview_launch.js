




































function test() {
  waitForExplicitFinish();
  
  let tabViewShownCount = 0;
  let onTabViewHidden = function() {
    ok(!TabView.isVisible(), "Tab View is hidden");

    if (tabViewShownCount == 1) {
      document.getElementById("menu_tabview").doCommand();
    } else if (tabViewShownCount == 2) {
      let utils = window.QueryInterface(Ci.nsIInterfaceRequestor).
                        getInterface(Ci.nsIDOMWindowUtils);
      let keyCode = 0;
      let charCode;
      let eventObject;
      if (navigator.platform.indexOf("Mac") != -1) {
        charCode = 160;
        eventObject = { altKey: true };
       } else {
        charCode = 32;
        eventObject = { ctrlKey: true };
      }
      let modifiers = EventUtils._parseModifiers(eventObject);
      let keyDownDefaultHappened =
        utils.sendKeyEvent("keydown", keyCode, charCode, modifiers);
      utils.sendKeyEvent("keypress", keyCode, charCode, modifiers,
                         !keyDownDefaultHappened);
      utils.sendKeyEvent("keyup", keyCode, charCode, modifiers);
    } else if (tabViewShownCount == 3) {
      window.removeEventListener("tabviewshown", onTabViewShown, false);
      window.removeEventListener("tabviewhidden", onTabViewHidden, false);
      finish();
    }
  }
  let onTabViewShown = function() {
    
    ok(TabView.isVisible(), "Tab View is visible. Count: " + tabViewShownCount);
    tabViewShownCount++
    executeSoon(function() { TabView.toggle(); });
  }
  window.addEventListener("tabviewshown", onTabViewShown, false);
  window.addEventListener("tabviewhidden", onTabViewHidden, false);
  
  ok(!TabView.isVisible(), "Tab View is hidden");

  let button = document.getElementById("tabview-button");
  ok(button, "Tab View button exists");
  button.doCommand();
}
