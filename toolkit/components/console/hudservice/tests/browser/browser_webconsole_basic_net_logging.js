










































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";
const TEST_NETWORK_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-network.html" + "?_date=" + Date.now();

function test() {
  addTab(TEST_NETWORK_URI);
  browser.addEventListener("DOMContentLoaded", onLoad, false);
}

function onLoad() {
  browser.removeEventListener("DOMContentLoaded", onLoad, false);
  openConsole();
  hudId = HUDService.displaysIndex()[0];
  
  
  browser.addEventListener("DOMContentLoaded", testBasicNetLogging,
                            false);
  browser.contentWindow.wrappedJSObject.document.location = TEST_NETWORK_URI;
}

function testBasicNetLogging() {
  browser.removeEventListener("DOMContentLoaded", testBasicNetLogging,
                              false);
  hudBox = HUDService.getHeadsUpDisplay(hudId);
  outputNode = hudBox.querySelector(".hud-output-node");
  log(outputNode);
  let testOutput = [];
  let nodes = outputNode.querySelectorAll(".hud-msg-node");
  log(nodes);

  executeSoon(function (){

    ok(nodes.length == 2, "2 children in output");
    ok(nodes[0].textContent.indexOf("test-network") > -1, "found test-network");
    
    
    
    ok(nodes[1].textContent.indexOf("network console") > -1, "found network console");
    finishTest();
  });
}


