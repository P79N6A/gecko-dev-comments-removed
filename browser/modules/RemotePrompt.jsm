




"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

this.EXPORTED_SYMBOLS = [ "RemotePrompt" ];

Cu.import("resource:///modules/PlacesUIUtils.jsm");
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/SharedPromptUtils.jsm");

let RemotePrompt = {
  init: function() {
    let mm = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
    mm.addMessageListener("Prompt:Open", this);
  },

  receiveMessage: function(message) {
    switch (message.name) {
      case "Prompt:Open":
        if (message.data.uri) {
          this.openModalWindow(message.data, message.target);
        } else {
          this.openTabPrompt(message.data, message.target)
        }
        break;
    }
  },

  openTabPrompt: function(args, browser) {
    let window = browser.ownerDocument.defaultView;
    let tabPrompt = window.gBrowser.getTabModalPromptBox(browser)
    let callbackInvoked = false;
    let newPrompt;
    let promptId = args._remoteId;

    function onPromptClose(forceCleanup) {
      if (newPrompt)
        tabPrompt.removePrompt(newPrompt);

      PromptUtils.fireDialogEvent(window, "DOMModalDialogClosed", browser);
      browser.messageManager.sendAsyncMessage("Prompt:Close", args);
    }

    browser.messageManager.addMessageListener("Prompt:ForceClose", function listener(message) {
      
      if (message.data._remoteId !== promptId) {
        return;
      }

      browser.messageManager.removeMessageListener("Prompt:ForceClose", listener);

      if (newPrompt) {
        newPrompt.abortPrompt();
      }
    });

    try {
      PromptUtils.fireDialogEvent(window, "DOMWillOpenModalDialog", browser);

      args.promptActive = true;

      newPrompt = tabPrompt.appendPrompt(args, onPromptClose);

      
      
      
    } catch (ex) {
      onPromptClose(true);
    }
  },

  openModalWindow: function(args, browser) {
    let window = browser.ownerDocument.defaultView;
    try {
      PromptUtils.fireDialogEvent(window, "DOMWillOpenModalDialog", browser);
      let bag = PromptUtils.objectToPropBag(args);

      Services.ww.openWindow(window, args.uri, "_blank",
                             "centerscreen,chrome,modal,titlebar", bag);

      PromptUtils.propBagToObject(bag, args);
    } finally {
      PromptUtils.fireDialogEvent(window, "DOMModalDialogClosed", browser);
      browser.messageManager.sendAsyncMessage("Prompt:Close", args);
    }
  }
};
