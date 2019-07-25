









const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";
let HUD;

let outputItem;

function consoleOpened(aHud) {
  HUD = aHud;

  outputNode = HUD.outputNode;

  browser.addEventListener("load", tabLoad2, true);

  
  browser.contentWindow.location.reload();
}

function tabLoad2(aEvent) {
  browser.removeEventListener(aEvent.type, tabLoad2, true);

  waitForSuccess({
    name: "network message displayed",
    validatorFn: function()
    {
      return outputNode.querySelector(".hud-networkinfo .hud-clickable");
    },
    successFn: function() {
      outputItem = outputNode.querySelector(".hud-networkinfo .hud-clickable");
      ok(outputItem, "found a network message");
      document.addEventListener("popupshown", networkPanelShown, false);

      
      EventUtils.sendMouseEvent({type: "mousedown"}, outputItem);
      EventUtils.sendMouseEvent({type: "click"}, outputItem);
    },
    failureFn: finishTest,
  });
}

function networkPanelShown(aEvent) {
  document.removeEventListener(aEvent.type, networkPanelShown, false);

  document.addEventListener("popupshown", networkPanelShowFailure, false);

  
  EventUtils.sendMouseEvent({type: "mousedown"}, outputItem);
  EventUtils.sendMouseEvent({type: "click"}, outputItem);

  executeSoon(function() {
    aEvent.target.addEventListener("popuphidden", networkPanelHidden, false);
    aEvent.target.hidePopup();
  });
}

function networkPanelShowFailure(aEvent) {
  document.removeEventListener(aEvent.type, networkPanelShowFailure, false);

  ok(false, "the network panel should not show");
}

function networkPanelHidden(aEvent) {
  this.removeEventListener(aEvent.type, networkPanelHidden, false);

  
  
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

    waitForSuccess({
      name: "jsterm output message",
      validatorFn: function()
      {
        return outputNode.querySelector(".webconsole-msg-output .hud-clickable");
      },
      successFn: function()
      {
        document.addEventListener("popupshown", propertyPanelShown, false);

        
        EventUtils.sendMouseEvent({type: "mousedown"}, outputItem);
        EventUtils.sendMouseEvent({type: "click"}, outputItem);
      },
      failureFn: finishTest,
    });
  });
}

function propertyPanelShown(aEvent) {
  document.removeEventListener(aEvent.type, propertyPanelShown, false);

  document.addEventListener("popupshown", propertyPanelShowFailure, false);

  
  EventUtils.sendMouseEvent({type: "mousedown"}, outputItem);
  EventUtils.sendMouseEvent({type: "click"}, outputItem);

  executeSoon(function() {
    aEvent.target.addEventListener("popuphidden", propertyPanelHidden, false);
    aEvent.target.hidePopup();
  });
}

function propertyPanelShowFailure(aEvent) {
  document.removeEventListener(aEvent.type, propertyPanelShowFailure, false);

  ok(false, "the property panel should not show");
}

function propertyPanelHidden(aEvent) {
  this.removeEventListener(aEvent.type, propertyPanelHidden, false);

  
  
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
    HUD = outputItem = null;
    executeSoon(finishTest);
  });
}

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

