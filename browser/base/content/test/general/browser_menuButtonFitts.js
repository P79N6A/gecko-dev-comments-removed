



function test () {
  waitForExplicitFinish();
  window.maximize();

  
  var navBar = document.getElementById("nav-bar");
  var boundingRect = navBar.getBoundingClientRect();
  var yPixel = boundingRect.top + Math.floor(boundingRect.height / 2);
  var xPixel = boundingRect.width - 1; 

  function onPopupHidden() {
    PanelUI.panel.removeEventListener("popuphidden", onPopupHidden);
    window.restore();
    finish();
  }
  function onPopupShown() {
    PanelUI.panel.removeEventListener("popupshown", onPopupShown);
    ok(true, "Clicking at the far edge of the window opened the menu popup.");
    PanelUI.panel.addEventListener("popuphidden", onPopupHidden);
    PanelUI.hide();
  }
  registerCleanupFunction(function() {
    PanelUI.panel.removeEventListener("popupshown", onPopupShown);
    PanelUI.panel.removeEventListener("popuphidden", onPopupHidden);
  });
  PanelUI.panel.addEventListener("popupshown", onPopupShown);
  EventUtils.synthesizeMouseAtPoint(xPixel, yPixel, {}, window);
}
