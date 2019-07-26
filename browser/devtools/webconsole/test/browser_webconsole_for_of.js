




const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-for-of.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testForOf);
  }, true);
}

function testForOf(hud) {
  var jsterm = hud.jsterm;
  jsterm.execute("{ [x.tagName for (x of document.body.childNodes) if (x.nodeType === 1)].join(' '); }",
    (node) => {
      ok(/H1 DIV H2 P/.test(node.textContent),
        "for-of loop should find all top-level nodes");
      finishTest();
    });
}
