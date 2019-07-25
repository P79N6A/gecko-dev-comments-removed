




































function test() {
  waitForExplicitFinish();
  
  let tabViewShownCount = 0;
  let onTabViewHidden = function() {
    ok(!TabView.isVisible(), "Tab View is hidden");

    if (tabViewShownCount == 1) {
      document.getElementById("menu_tabview").doCommand();
    } else if (tabViewShownCount == 2) {
      EventUtils.synthesizeKey("e", { accelKey: true });
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
