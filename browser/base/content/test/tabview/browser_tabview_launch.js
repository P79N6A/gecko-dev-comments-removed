




































let tabViewShownCount = 0;


function test() {
  waitForExplicitFinish();
  
  
  ok(!TabView.isVisible(), "Tab View starts hidden");

  
  window.addEventListener("tabviewshown", onTabViewLoadedAndShown, false);
  let button = document.getElementById("tabview-button");
  ok(button, "Tab View button exists");
  button.doCommand();
}


function onTabViewLoadedAndShown() {
  window.removeEventListener("tabviewshown", onTabViewLoadedAndShown, false);
  
  ok(TabView.isVisible(), "Tab View is visible. Count: " + tabViewShownCount);
  tabViewShownCount++;
  
  
  window.addEventListener("tabviewshown", onTabViewShown, false);
  window.addEventListener("tabviewhidden", onTabViewHidden, false);
  TabView.toggle();
}


function onTabViewShown() {
  
  ok(TabView.isVisible(), "Tab View is visible. Count: " + tabViewShownCount);
  tabViewShownCount++;
  TabView.toggle();
}


function onTabViewHidden() {
  ok(!TabView.isVisible(), "Tab View is hidden. Count: " + tabViewShownCount);

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