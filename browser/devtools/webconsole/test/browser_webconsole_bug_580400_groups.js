






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testGroups);
  }, true);
}

function testGroups(HUD) {
  let jsterm = HUD.jsterm;
  let outputNode = HUD.outputNode;

  
  
  
  

  let timestamp0 = Date.now();
  jsterm.execute("0");
  is(outputNode.querySelectorAll(".webconsole-new-group").length, 0,
     "no group dividers exist after the first console message");

  jsterm.execute("1");
  let timestamp1 = Date.now();
  if (timestamp1 - timestamp0 < 5000) {
    is(outputNode.querySelectorAll(".webconsole-new-group").length, 0,
       "no group dividers exist after the second console message");
  }

  for (let i = 0; i < outputNode.itemCount; i++) {
    outputNode.getItemAtIndex(i).timestamp = 0;   
  }

  jsterm.execute("2");
  is(outputNode.querySelectorAll(".webconsole-new-group").length, 1,
     "one group divider exists after the third console message");

  finishTest();
}

