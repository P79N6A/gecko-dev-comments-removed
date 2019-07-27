







"use strict";

function test() {
  const TEST_URI = "data:text/html;charset=utf8,<p>hello world! bug 874061" +
                   "<button onclick='console.log(\"foobar bug 874061\");" +
                   "fooBazBaz.yummy()'>click</button>";
  let ConsoleAPIStorage = Cc["@mozilla.org/consoleAPI-storage;1"]
                            .getService(Ci.nsIConsoleAPIStorage);
  let privateWindow, privateBrowser, privateTab, privateContent;
  let hud, expectedMessages, nonPrivateMessage;

  
  
  
  requestLongerTimeout(2);
  start();

  function start() {
    gBrowser.selectedTab = gBrowser.addTab("data:text/html;charset=utf8," +
                                           "<p>hello world! I am not private!");
    gBrowser.selectedBrowser.addEventListener("load", onLoadTab, true);
  }

  function onLoadTab() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoadTab, true);
    info("onLoadTab()");

    
    Services.console.reset();
    ConsoleAPIStorage.clearEvents();

    
    content.console.log("bug874061-not-private");

    nonPrivateMessage = {
      name: "console message from a non-private window",
      text: "bug874061-not-private",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    };

    privateWindow = OpenBrowserWindow({ private: true });
    ok(privateWindow, "new private window");
    ok(PrivateBrowsingUtils.isWindowPrivate(privateWindow), "window is private");

    whenDelayedStartupFinished(privateWindow, onPrivateWindowReady);
  }

  function onPrivateWindowReady() {
    info("private browser window opened");
    privateBrowser = privateWindow.gBrowser;

    privateTab = privateBrowser.selectedTab = privateBrowser.addTab(TEST_URI);
    privateBrowser.selectedBrowser.addEventListener("load", function onLoad() {
      info("private tab opened");
      privateBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
      privateContent = privateBrowser.selectedBrowser.contentWindow;
      ok(PrivateBrowsingUtils.isBrowserPrivate(privateBrowser.selectedBrowser),
         "tab window is private");
      openConsole(privateTab).then(consoleOpened);
    }, true);
  }

  function addMessages() {
    let button = privateContent.document.querySelector("button");
    ok(button, "button in page");
    EventUtils.synthesizeMouse(button, 2, 2, {}, privateContent);
  }

  function consoleOpened(aHud) {
    hud = aHud;
    ok(hud, "web console opened");

    addMessages();
    expectedMessages = [
      {
        name: "script error",
        text: "fooBazBaz is not defined",
        category: CATEGORY_JS,
        severity: SEVERITY_ERROR,
      },
      {
        name: "console message",
        text: "foobar bug 874061",
        category: CATEGORY_WEBDEV,
        severity: SEVERITY_LOG,
      },
    ];

    
    
    waitForMessages({
      webconsole: hud,
      messages: expectedMessages,
    }).then(testCachedMessages);
  }

  function testCachedMessages() {
    info("testCachedMessages()");
    closeConsole(privateTab).then(() => {
      info("web console closed");
      openConsole(privateTab).then(consoleReopened);
    });
  }

  function consoleReopened(aHud) {
    hud = aHud;
    ok(hud, "web console reopened");

    
    
    waitForMessages({
      webconsole: hud,
      messages: expectedMessages,
    }).then(testBrowserConsole);
  }

  function testBrowserConsole() {
    info("testBrowserConsole()");
    closeConsole(privateTab).then(() => {
      info("web console closed");
      privateWindow.HUDService.toggleBrowserConsole().then(onBrowserConsoleOpen);
    });
  }

  
  
  function checkNoPrivateMessages() {
    let text = hud.outputNode.textContent;
    is(text.indexOf("fooBazBaz"), -1, "no exception displayed");
    is(text.indexOf("bug 874061"), -1, "no console message displayed");
  }

  function onBrowserConsoleOpen(aHud) {
    hud = aHud;
    ok(hud, "browser console opened");

    checkNoPrivateMessages();
    addMessages();
    expectedMessages.push(nonPrivateMessage);

    
    
    waitForMessages({
      webconsole: hud,
      messages: expectedMessages,
    }).then(testPrivateWindowClose);
  }

  function testPrivateWindowClose() {
    info("close the private window and check if the private messages are removed");
    hud.jsterm.once("private-messages-cleared", () => {
      isnot(hud.outputNode.textContent.indexOf("bug874061-not-private"), -1,
            "non-private messages are still shown after private window closed");
      checkNoPrivateMessages();

      info("close the browser console");
      privateWindow.HUDService.toggleBrowserConsole().then(() => {
        info("reopen the browser console");
        executeSoon(() =>
          HUDService.toggleBrowserConsole().then(onBrowserConsoleReopen));
      });
    });
    privateWindow.BrowserTryToCloseWindow();
  }

  function onBrowserConsoleReopen(aHud) {
    hud = aHud;
    ok(hud, "browser console reopened");

    
    waitForMessages({
      webconsole: hud,
      messages: [nonPrivateMessage],
    }).then(() => {
      
      
      checkNoPrivateMessages();
      executeSoon(finishTest);
    });
  }
}
