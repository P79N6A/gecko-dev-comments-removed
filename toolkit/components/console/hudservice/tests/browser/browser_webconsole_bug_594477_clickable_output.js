









const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";
let HUD;

let outputItem;

function tabLoad1(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  openConsole();

  let hudId = HUDService.getHudIdByWindow(browser.contentWindow);
  HUD = HUDService.hudWeakReferences[hudId].get();

  let display = HUDService.getOutputNodeById(hudId);
  outputNode = display.querySelector(".hud-output-node");

  browser.addEventListener("load", tabLoad2, true);

  
  browser.contentWindow.location.reload();
  log(document);
}

function tabLoad2(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  outputItem = outputNode.querySelector(".hud-network");
  ok(outputItem, "found a network message");
  document.addEventListener("popupshown", networkPanelShown, false);

  
  EventUtils.sendMouseEvent({type: "mousedown"}, outputItem);
  EventUtils.sendMouseEvent({type: "click"}, outputItem);
}

function networkPanelShown(aEvent) {
  document.removeEventListener(aEvent.type, arguments.callee, false);

  document.addEventListener("popupshown", networkPanelShowFailure, false);

  
  EventUtils.sendMouseEvent({type: "mousedown"}, outputItem);
  EventUtils.sendMouseEvent({type: "click"}, outputItem);

  executeSoon(function() {
    aEvent.target.addEventListener("popuphidden", networkPanelHidden, false);
    aEvent.target.hidePopup();
  });
}

function networkPanelShowFailure(aEvent) {
  document.removeEventListener(aEvent.type, arguments.callee, false);

  ok(false, "the network panel should not show");
}

function networkPanelHidden(aEvent) {
  this.removeEventListener(aEvent.type, arguments.callee, false);

  
  
  EventUtils.sendMouseEvent({type: "mousedown", clientX: 3, clientY: 4},
    outputItem);
  EventUtils.sendMouseEvent({type: "click", clientX: 5, clientY: 6},
    outputItem);

  
  EventUtils.sendMouseEvent({type: "mousedown", button: 1},
    outputItem);
  EventUtils.sendMouseEvent({type: "click", button: 1},
    outputItem);

  
  EventUtils.sendMouseEvent({type: "mousedown", button: 2},
    outputItem);
  EventUtils.sendMouseEvent({type: "click", button: 2},
    outputItem);

  executeSoon(function() {
    document.removeEventListener("popupshown", networkPanelShowFailure, false);

    
    
    HUD.jsterm.setInputValue("document");
    HUD.jsterm.execute();

    outputItem = outputNode.querySelector(".jsterm-output-line");
    ok(outputItem, "found a jsterm output message");

    document.addEventListener("popupshown", properyPanelShown, false);

    
    EventUtils.sendMouseEvent({type: "mousedown"}, outputItem);
    EventUtils.sendMouseEvent({type: "click"}, outputItem);
  });
}

function properyPanelShown(aEvent) {
  document.removeEventListener(aEvent.type, arguments.callee, false);

  document.addEventListener("popupshown", propertyPanelShowFailure, false);

  
  EventUtils.sendMouseEvent({type: "mousedown"}, outputItem);
  EventUtils.sendMouseEvent({type: "click"}, outputItem);

  executeSoon(function() {
    aEvent.target.addEventListener("popuphidden", propertyPanelHidden, false);
    let closeButton = aEvent.target.querySelector(".jsPropertyPanelCloseButton");
    closeButton.doCommand();
  });
}

function propertyPanelShowFailure(aEvent) {
  document.removeEventListener(aEvent.type, arguments.callee, false);

  ok(false, "the property panel should not show");
}

function propertyPanelHidden(aEvent) {
  this.removeEventListener(aEvent.type, arguments.callee, false);

  
  
  EventUtils.sendMouseEvent({type: "mousedown", clientX: 3, clientY: 4},
    outputItem);
  EventUtils.sendMouseEvent({type: "click", clientX: 5, clientY: 6},
    outputItem);

  
  EventUtils.sendMouseEvent({type: "mousedown", button: 1},
    outputItem);
  EventUtils.sendMouseEvent({type: "click", button: 1},
    outputItem);

  
  EventUtils.sendMouseEvent({type: "mousedown", button: 2},
    outputItem);
  EventUtils.sendMouseEvent({type: "click", button: 2},
    outputItem);

  executeSoon(function() {
    document.removeEventListener("popupshown", propertyPanelShowFailure, false);
    outputItem = null;
    finishTest();
  });
}

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", tabLoad1, true);
}

