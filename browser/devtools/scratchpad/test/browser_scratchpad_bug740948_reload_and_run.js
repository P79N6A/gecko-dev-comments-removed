



let DEVTOOLS_CHROME_ENABLED = "devtools.chrome.enabled";
let EDITOR_TEXT = [
  "var evt = new CustomEvent('foo', { bubbles: true });",
  "document.body.innerHTML = 'Modified text';",
  "window.dispatchEvent(evt);"
].join("\n");

function test()
{
  waitForExplicitFinish();
  Services.prefs.setBoolPref(DEVTOOLS_CHROME_ENABLED, true);

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    openScratchpad(runTests);
  }, true);

  content.location = "data:text/html,Scratchpad test for bug 740948";
}

function runTests()
{
  let sp = gScratchpadWindow.Scratchpad;
  ok(sp, "Scratchpad object exists in new window");

  
  

  let reloadAndRun = gScratchpadWindow.document.
    getElementById("sp-cmd-reloadAndRun");
  ok(reloadAndRun, "Reload And Run command exists");
  ok(!reloadAndRun.hasAttribute("disabled"),
      "Reload And Run command is enabled");

  sp.setBrowserContext();
  ok(reloadAndRun.hasAttribute("disabled"),
      "Reload And Run command is disabled in the browser context.");

  
  
  
  
  

  sp.setContentContext();
  sp.setText(EDITOR_TEXT);

  let browser = gBrowser.selectedBrowser;

  browser.addEventListener("DOMWindowCreated", function onWindowCreated() {
    browser.removeEventListener("DOMWindowCreated", onWindowCreated, true);

    browser.contentWindow.addEventListener("foo", function onFoo() {
      browser.contentWindow.removeEventListener("foo", onFoo, true);

      is(browser.contentWindow.document.body.innerHTML, "Modified text",
        "After reloading, HTML is different.");

      Services.prefs.clearUserPref(DEVTOOLS_CHROME_ENABLED);
      finish();
    }, true);
  }, true);

  ok(browser.contentWindow.document.body.innerHTML !== "Modified text",
      "Before reloading, HTML is intact.");
  sp.reloadAndRun();
}

