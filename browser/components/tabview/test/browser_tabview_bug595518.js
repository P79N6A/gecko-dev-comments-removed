


function test() {
  waitForExplicitFinish();
  
  
  window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
  TabView.toggle();
}

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);
  ok(TabView.isVisible(), "Tab View is visible");

  let contentWindow = document.getElementById("tab-view").contentWindow;

  let onTabViewHidden = function() {
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);

    
    ok(!TabView.isVisible(), "Tab View is hidden");
    
    
    is(contentWindow.iQ("#exit-button:focus").length, 0, 
       "The exit button doesn't have the focus");

    
    
    window.addEventListener("tabviewshown", onTabViewShown, false);
    EventUtils.synthesizeKey("e", { accelKey: true, shiftKey: true }, contentWindow);
  }
  
  let onTabViewShown = function() {
    window.removeEventListener("tabviewshown", onTabViewShown, false);
    
    
    ok(TabView.isVisible(), "Tab View is visible");

    
    let endGame = function() {
      window.removeEventListener("tabviewhidden", endGame, false);

      ok(!TabView.isVisible(), "Tab View is hidden");
      finish();
    }
    window.addEventListener("tabviewhidden", endGame, false);
    TabView.toggle();
  }

  window.addEventListener("tabviewhidden", onTabViewHidden, false);

  
  let button = contentWindow.document.getElementById("exit-button");
  ok(button, "Exit button exists");

  
  button.focus();
  EventUtils.sendMouseEvent({ type: "click" }, button, contentWindow);
}
