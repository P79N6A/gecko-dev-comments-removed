







const TEST_URI = "data:text/html;charset=utf8,<p>test the JS property provider";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", testPropertyProvider, true);
}

function testPropertyProvider() {
  browser.removeEventListener("load", testPropertyProvider, true);

  let tmp = {};
  Cu.import("resource://gre/modules/devtools/WebConsoleUtils.jsm", tmp);
  let JSPropertyProvider = tmp.JSPropertyProvider;
  tmp = null;

  let completion = JSPropertyProvider(content, "thisIsNotDefined");
  is (completion.matches.length, 0, "no match for 'thisIsNotDefined");

  
  completion = JSPropertyProvider(content, "window[1].acb");
  is (completion, null, "no match for 'window[1].acb");

  
  var strComplete =
    'function a() { }document;document.getElementById(window.locatio';
  completion = JSPropertyProvider(content, strComplete);
  ok(completion.matches.length == 2, "two matches found");
  ok(completion.matchProp == "locatio", "matching part is 'test'");
  ok(completion.matches[0] == "location", "the first match is 'location'");
  ok(completion.matches[1] == "locationbar", "the second match is 'locationbar'");

  finishTest();
}

