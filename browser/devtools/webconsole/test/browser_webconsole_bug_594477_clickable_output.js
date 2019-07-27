









const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";
let HUD;
let outputItem;
let outputNode;

"use strict";

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  HUD = yield openConsole();
  outputNode = HUD.outputNode;

  
  BrowserReload();
  yield loadBrowser(gBrowser.selectedBrowser);

  let event = yield clickEvents();
  yield testClickAgain(event);
  yield networkPanelHidden();

  HUD = outputItem = outputNode = null;
});

function clickEvents() {
  let deferred = promise.defer();

  waitForMessages({
    webconsole: HUD,
    messages: [{
      text: "test-console.html",
      category: CATEGORY_NETWORK,
      severity: SEVERITY_LOG,
    }],
  }).then(([result]) => {
    let msg = [...result.matched][0];
    outputItem = msg.querySelector(".message-body .url");
    ok(outputItem, "found a network message");
    document.addEventListener("popupshown", function onPanelShown(event) {
      document.removeEventListener("popupshown", onPanelShown, false);
      deferred.resolve(event);
    }, false);

    
    EventUtils.sendMouseEvent({type: "mousedown"}, outputItem);
    EventUtils.sendMouseEvent({type: "click"}, outputItem);
  });

  return deferred.promise;
}

function testClickAgain(event) {
  info("testClickAgain");

  let deferred = promise.defer();

  document.addEventListener("popupshown", networkPanelShowFailure, false);

  
  EventUtils.sendMouseEvent({type: "mousedown"}, outputItem);
  EventUtils.sendMouseEvent({type: "click"}, outputItem);

  executeSoon(function() {
    document.addEventListener("popuphidden", function onHidden() {
      document.removeEventListener("popuphidden", onHidden, false);
      deferred.resolve();
    }, false);
    event.target.hidePopup();
  });

  return deferred.promise;
}

function networkPanelShowFailure() {
  ok(false, "the network panel should not show");
}

function networkPanelHidden() {
  let deferred = promise.defer();

  info("networkPanelHidden");

  
  
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

    
    
    HUD.jsterm.execute("document").then((msg) => {
      info("jsterm execute 'document' callback");

      HUD.jsterm.once("variablesview-open", deferred.resolve);
      let outputItem = msg.querySelector(".message-body a");
      ok(outputItem, "jsterm output message found");

      
      EventUtils.sendMouseEvent({type: "mousedown"}, outputItem);
      EventUtils.sendMouseEvent({type: "click"}, outputItem);
    });
  });

  return deferred.promise;
}
