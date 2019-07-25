










































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testPropertyProvider, false);
}

function testPropertyProvider() {
  browser.removeEventListener("DOMContentLoaded", testPropertyProvider,
                              false);

  openConsole();

  hudId = HUDService.displaysIndex()[0];

  var HUD = HUDService.hudWeakReferences[hudId].get();
  var jsterm = HUD.jsterm;
  var context = jsterm.sandbox.window;
  var completion;

  
  ok (jsterm.propertyProvider !== undefined, "JSPropertyProvider is defined");

  completion = jsterm.propertyProvider(context, "thisIsNotDefined");
  is (completion.matches.length, 0, "no match for 'thisIsNotDefined");

  
  completion = jsterm.propertyProvider(context, "window[1].acb");
  is (completion, null, "no match for 'window[1].acb");

  
  var strComplete =
    'function a() { }document;document.getElementById(window.locatio';
  completion = jsterm.propertyProvider(context, strComplete);
  ok(completion.matches.length == 2, "two matches found");
  ok(completion.matchProp == "locatio", "matching part is 'test'");
  ok(completion.matches[0] == "location", "the first match is 'location'");
  ok(completion.matches[1] == "locationbar", "the second match is 'locationbar'");
  context = completion = null;
  finishTest();
}

