








"use strict";

let test = asyncTest(function* () {
  let wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                   .getService(Components.interfaces.nsIWindowMediator);
  let currWindow, hud, mainWindow;

  mainWindow = Services.wm.getMostRecentWindow(null);

  yield HUDService.openBrowserConsoleOrFocus();

  hud = HUDService.getBrowserConsole();

  console.log("testmessage");
  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "testmessage"
    }],
  });

  currWindow = Services.wm.getMostRecentWindow(null);
  is(currWindow.document.documentURI, devtools.Tools.webConsole.url,
     "The Browser Console is open and has focus");

  mainWindow.focus();

  yield HUDService.openBrowserConsoleOrFocus();

  currWindow = Services.wm.getMostRecentWindow(null);
  is(currWindow.document.documentURI, devtools.Tools.webConsole.url,
     "The Browser Console is open and has focus");

  yield HUDService.toggleBrowserConsole();

  hud = HUDService.getBrowserConsole();
  ok(!hud, "Browser Console has been closed");
});
