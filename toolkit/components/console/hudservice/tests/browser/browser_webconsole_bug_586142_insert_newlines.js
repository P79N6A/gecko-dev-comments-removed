












const TEST_URI = "http://example.com/";

function test()
{
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", onLoad, false);
}

function onLoad() {
  browser.removeEventListener("DOMContentLoaded", onLoad,
                                               false);
  executeSoon(testNewlines);
}

function testNewlines() {
  openConsole();
  hudId = HUDService.displaysIndex()[0];
  ok(hudId != null, "we have the HUD ID");

  HUDService.clearDisplay(hudId);

  let contentWindow = browser.contentWindow;
  let console = contentWindow.wrappedJSObject.console;
  ok(console != null, "we have the console object");

  for (let i = 0; i < 20; i++) {
    console.log("Hello world!");
  }

  let hudNode = HUDService.getOutputNodeById(hudId);
  let outputNode = hudNode.querySelector(".hud-output-node");
  ok(outputNode != null, "we have the output node");

  let labels = outputNode.querySelectorAll("label");
  is(labels.length, 20, "we found 20 labels in the output node");

  for (let i = 0; i < labels.length; i++) {
    let value = labels[i].textContent;
    is(value[value.length - 1], "\n", "the value of label " + i + " ends " +
       "with a newline");
  }

  finishTest();
}

