


let tabViewShownCount = 0;
let timerId;


function test() {
  waitForExplicitFinish();

  
  ok(!TabView.isVisible(), "Tab View starts hidden");

  
  window.addEventListener("tabviewshown", onTabViewLoadedAndShown, false);
  TabView.toggle();

  registerCleanupFunction(function () {
    window.removeEventListener("tabviewshown", onTabViewLoadedAndShown, false);
    if (timerId) 
      clearTimeout(timerId);
    TabView.hide()
  });
}


function onTabViewLoadedAndShown() {
  window.removeEventListener("tabviewshown", onTabViewLoadedAndShown, false);

  
  
  
  
  
  
  let deck = document.getElementById("tab-view-deck");
  let iframe = document.getElementById("tab-view");
  ok(iframe, "The tab view iframe exists");
  
  function waitForSwitch() {
    if (deck.selectedPanel == iframe) {
      ok(TabView.isVisible(), "Tab View is visible. Count: " + tabViewShownCount);
      tabViewShownCount++;

      
      window.addEventListener("tabviewshown", onTabViewShown, false);
      window.addEventListener("tabviewhidden", onTabViewHidden, false);

      registerCleanupFunction(function () {
        window.removeEventListener("tabviewshown", onTabViewShown, false);
        window.removeEventListener("tabviewhidden", onTabViewHidden, false);
      });
      TabView.toggle();
    } else {
      timerId = setTimeout(waitForSwitch, 10);
    }
  }

  timerId = setTimeout(waitForSwitch, 1);
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
    EventUtils.synthesizeKey("E", { accelKey: true, shiftKey: true });
  } else if (tabViewShownCount == 3) {
    window.removeEventListener("tabviewshown", onTabViewShown, false);
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);
    finish();
  }
}
