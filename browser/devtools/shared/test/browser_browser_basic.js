




const TEST_URI = "data:text/html;charset=utf-8,<p id=id>Text</p>";

let imported = {};
Components.utils.import("resource:///modules/devtools/Browser.jsm", imported);

registerCleanupFunction(function tearDown() {
  imported = undefined;
});

function test() {
  addTab(TEST_URI, function(browser, tab, document) {
    runTest(browser, tab, document);
  });
}

function runTest(browser, tab, document) {
  var p = document.getElementById("id");

  ok(p instanceof imported.Node, "Node correctly defined");
  ok(p instanceof imported.HTMLElement, "HTMLElement correctly defined");
}
