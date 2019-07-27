







let IndexedDB = {
  _permissionsPrompt: "indexedDB-permissions-prompt",
  _permissionsResponse: "indexedDB-permissions-response",

  _notificationIcon: "indexedDB-notification-icon",

  receiveMessage: function(aMessage) {
    switch (aMessage.name) {
      case "IndexedDB:Prompt":
        this.showPrompt(aMessage);
    }
  },

  showPrompt: function(aMessage) {
    let browser = aMessage.target;
    let payload = aMessage.json;
    let host = payload.host;
    let topic = payload.topic;
    let type;

    if (topic == this._permissionsPrompt) {
      type = "indexedDB";
      payload.responseTopic = this._permissionsResponse;
    }

    let prompt = Cc["@mozilla.org/content-permission/prompt;1"].createInstance(Ci.nsIContentPermissionPrompt);
    let types = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
    let promptType = {
      type: type,
      access: "unused",
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentPermissionType])
    };
    types.appendElement(promptType, false);

    
    let timeoutId = setTimeout(function() {
      payload.permission = Ci.nsIPermissionManager.UNKNOWN_ACTION;
      browser.messageManager.sendAsyncMessage("IndexedDB:Response", payload);
      timeoutId = null;
    }, 30000);
 
    function checkTimeout() {
      if (timeoutId === null) return true;
      clearTimeout(timeoutId);
      timeoutId = null;
      return false;
    }

    prompt.prompt({
      types: types,
      uri: Services.io.newURI(payload.location, null, null),
      window: null,
      element: aMessage.target,

      cancel: function() {
        if (checkTimeout()) return;
        payload.permission = Ci.nsIPermissionManager.DENY_ACTION;
        browser.messageManager.sendAsyncMessage("IndexedDB:Response", payload);
      },

      allow: function() {
        if (checkTimeout()) return;
        payload.permission = Ci.nsIPermissionManager.ALLOW_ACTION;
        browser.messageManager.sendAsyncMessage("IndexedDB:Response", payload);
      },
    });
  },
};

