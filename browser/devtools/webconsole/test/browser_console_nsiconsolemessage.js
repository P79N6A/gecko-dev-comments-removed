







const TEST_URI = "data:text/html;charset=utf8,<title>bug859756</title>\n" +
                 "<p>hello world\n<p>nsIConsoleMessages ftw!";

function test()
{
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);

    
    Services.console.logStringMessage("test1 for bug859756");

    info("open web console");
    openConsole(null, consoleOpened);
  }, true);
}

function consoleOpened(hud)
{
  ok(hud, "web console opened");
  Services.console.logStringMessage("do-not-show-me");
  content.console.log("foobarz");

  waitForMessages({
    webconsole: hud,
    messages: [
      {
        text: "foobarz",
        category: CATEGORY_WEBDEV,
        severity: SEVERITY_LOG,
      },
    ],
  }).then(() => {
    let text = hud.outputNode.textContent;
    is(text.indexOf("do-not-show-me"), -1,
       "nsIConsoleMessages are not displayed");
    is(text.indexOf("test1 for bug859756"), -1,
       "nsIConsoleMessages are not displayed (confirmed)");
    closeConsole(null, onWebConsoleClose);
  });
}

function onWebConsoleClose()
{
  info("web console closed");
  HUDConsoleUI.toggleBrowserConsole().then(onBrowserConsoleOpen);
}

function onBrowserConsoleOpen(hud)
{
  ok(hud, "browser console opened");
  Services.console.logStringMessage("test2 for bug859756");

  waitForMessages({
    webconsole: hud,
    messages: [
      {
        text: "test1 for bug859756",
        category: CATEGORY_JS,
      },
      {
        text: "test2 for bug859756",
        category: CATEGORY_JS,
      },
      {
        text: "do-not-show-me",
        category: CATEGORY_JS,
      },
    ],
  }).then(finishTest);
}
