






"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/test-console.html";

let browser;

function test() {
  loadTab(TEST_URI).then(() => {
    openConsole().then((tab) => {
      browser = tab.browser;

      browser.addEventListener("DOMContentLoaded", testPageReload, false);
      content.location.reload();
    });
  });
  browser.addEventListener("DOMContentLoaded", onLoad, false);
}

function testPageReload() {
  browser.removeEventListener("DOMContentLoaded", testPageReload, false);

  let console = browser.contentWindow.wrappedJSObject.console;

  is(typeof console, "object", "window.console is an object, after page reload");
  is(typeof console.log, "function", "console.log is a function");
  is(typeof console.info, "function", "console.info is a function");
  is(typeof console.warn, "function", "console.warn is a function");
  is(typeof console.error, "function", "console.error is a function");
  is(typeof console.exception, "function", "console.exception is a function");

  browser = null;
  finishTest();
}
