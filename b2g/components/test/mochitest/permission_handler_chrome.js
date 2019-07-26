



"use strict";

function debug(str) {
  dump("CHROME PERMISSON HANDLER -- " + str + "\n");
}

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const { Services } = Cu.import("resource://gre/modules/Services.jsm");

let browser = Services.wm.getMostRecentWindow("navigator:browser");
let shell;

function loadShell() {
  if (!browser) {
    debug("no browser");
    return false;
  }
  shell = browser.shell;
  return true;
}

function getContentWindow() {
  return shell.contentBrowser.contentWindow;
}

if (loadShell()) {
  let content = getContentWindow();
  let eventHandler = function(evt) {
    if (!evt.detail || evt.detail.type !== "permission-prompt") {
      return;
    }

    sendAsyncMessage("permission-request", evt.detail);
  };

  content.addEventListener("mozChromeEvent", eventHandler);

  
  addMessageListener("teardown", function() {
    content.removeEventListener("mozChromeEvent", eventHandler);
  });

  addMessageListener("permission-response", function(detail) {
    let event = content.document.createEvent('CustomEvent');
    event.initCustomEvent('mozContentEvent', true, true, detail);
    content.dispatchEvent(event);
  });
}

