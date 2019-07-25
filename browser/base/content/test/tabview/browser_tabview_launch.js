


let tabViewShownCount = 0;


function test() {
  waitForExplicitFinish();

  
  ok(!TabView.isVisible(), "Tab View starts hidden");

  
  window.addEventListener("tabviewshown", onTabViewLoadedAndShown, false);
  let tabViewCommand = document.getElementById("Browser:ToggleTabView");
  tabViewCommand.doCommand();
}


function onTabViewLoadedAndShown() {
  window.removeEventListener("tabviewshown", onTabViewLoadedAndShown, false);

  
  
  
  
  
  
  let deck = document.getElementById("tab-view-deck");
  function waitForSwitch() {
    if (deck.selectedIndex == 1) {
      ok(TabView.isVisible(), "Tab View is visible. Count: " + tabViewShownCount);
      tabViewShownCount++;

      
      window.addEventListener("tabviewshown", onTabViewShown, false);
      window.addEventListener("tabviewhidden", onTabViewHidden, false);
      TabView.toggle();
    } else {
      setTimeout(waitForSwitch, 10);
    }
  }

  setTimeout(waitForSwitch, 1);
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
    EventUtils.synthesizeKey("e", { accelKey: true, shiftKey: true });
  } else if (tabViewShownCount == 3) {
    window.removeEventListener("tabviewshown", onTabViewShown, false);
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);
    finish();
  }
}
