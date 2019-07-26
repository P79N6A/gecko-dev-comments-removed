


"use strict";




function test() {
  
  
  
  const buttonID = "Test-non-widget-non-removable-button";
  let btn = document.createElement("toolbarbutton");
  btn.id = buttonID;
  btn.label = "Hi";
  btn.setAttribute("style", "width: 20px; height: 20px; background-color: red");
  document.getElementById("nav-bar").appendChild(btn);
  registerCleanupFunction(function() {
    btn.remove();
  });

  
  
  
  CustomizableUI.addWidgetToArea(buttonID, CustomizableUI.AREA_TABSTRIP);
  let placement = CustomizableUI.getPlacementOfWidget(buttonID);
  
  ok(placement, "Button should be placed");
  is(placement && placement.area, CustomizableUI.AREA_TABSTRIP, "Should be placed on tabstrip.");
  
  is(btn.parentNode && btn.parentNode.id, "nav-bar", "Actual button should still be on navbar.");
  
  
  
  CustomizableUI.removeWidgetFromArea(buttonID);
  placement = CustomizableUI.getPlacementOfWidget(buttonID);
  
  ok(!placement, "Button should no longer have a placement.");
  
  is(btn.parentNode && btn.parentNode.id, "nav-bar", "Actual button should still be on navbar.");
}

