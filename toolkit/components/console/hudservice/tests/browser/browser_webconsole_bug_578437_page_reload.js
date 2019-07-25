








































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", onLoad, false);
}

function onLoad() {
  browser.removeEventListener("DOMContentLoaded", onLoad, false);

  openConsole();

  browser.addEventListener("DOMContentLoaded", testPageReload, false);
  content.location.reload();
}

function testPageReload() {

  browser.removeEventListener("DOMContentLoaded", testPageReload, false);

  let hudId = HUDService.displaysIndex()[0];
  let console = browser.contentWindow.wrappedJSObject.console;

  is(typeof console, "object", "window.console is an object, after page reload");
  is(typeof console.log, "function", "console.log is a function");
  is(typeof console.info, "function", "console.info is a function");
  is(typeof console.warn, "function", "console.warn is a function");
  is(typeof console.error, "function", "console.error is a function");
  is(typeof console.exception, "function", "console.exception is a function");

  finishTest();
}

