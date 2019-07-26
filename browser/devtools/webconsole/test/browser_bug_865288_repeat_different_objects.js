







const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-repeated-messages.html";

let hud = null;

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

function consoleOpened(aHud) {
  hud = aHud;

  
  info("waiting for 3 console.log objects");

  hud.jsterm.clearOutput(true);
  content.wrappedJSObject.testConsoleObjects();

  waitForMessages({
    webconsole: hud,
    messages: [{
      name: "3 console.log messages",
      text: "abba",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
      count: 3,
      repeats: 1,
      objects: true,
    }],
  }).then(checkMessages);
}

function checkMessages(aResults)
{
  let result = aResults[0];
  let msgs = [...result.matched];
  is(msgs.length, 3, "3 message elements");
  let m = -1;

  function nextMessage()
  {
    let msg = msgs[++m];
    if (msg) {
      ok(msg, "message element #" + m);

      let clickable = msg.querySelector(".hud-clickable");
      ok(clickable, "clickable object #" + m);

      scrollOutputToNode(msg);
      clickObject(clickable);
    }
    else {
      finishTest();
    }
  }

  nextMessage();

  function clickObject(aObject)
  {
    hud.jsterm.once("variablesview-fetched", onObjectFetch);
    EventUtils.synthesizeMouse(aObject, 2, 2, {}, hud.iframeWindow);
  }

  function onObjectFetch(aEvent, aVar)
  {
    findVariableViewProperties(aVar, [
      { name: "id", value: "abba" + m },
    ], { webconsole: hud }).then(nextMessage);
  }
}
