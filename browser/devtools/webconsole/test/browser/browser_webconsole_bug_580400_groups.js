









































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testGroups, false);
}

function testGroups() {
  browser.removeEventListener("DOMContentLoaded", testGroups, false);

  openConsole();

  let HUD = HUDService.getHudByWindow(content);
  let jsterm = HUD.jsterm;
  let outputNode = jsterm.outputNode;

  
  
  
  

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

  jsterm.clearOutput();
  jsterm.history.splice(0, jsterm.history.length);   

  finishTest();
}

