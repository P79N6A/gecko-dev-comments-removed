


"use strict";

XPCOMUtils.defineLazyModuleGetter(this, "ConsoleAPI",
                                  "resource://gre/modules/devtools/Console.jsm");





var EmbedRT = {
  _scopes: {},

  observe: function(subject, topic, data) {
    switch(topic) {
      case "GeckoView:ImportScript":
        this.importScript(data);
        break;
    }
  },

  


  importScript: function(scriptURL) {
    if (scriptURL in this._scopes) {
      return;
    }

    let principal = Cc["@mozilla.org/systemprincipal;1"].createInstance(Ci.nsIPrincipal);

    let sandbox = new Cu.Sandbox(principal,
      {
        sandboxName: scriptURL,
        wantGlobalProperties: ["indexedDB"]
      }
    );

    sandbox["console"] = new ConsoleAPI({ consoleID: "script/" + scriptURL });
    sandbox["GeckoView"] = {
      sendRequest: function(data) {
        if (!data) {
          throw new Error("Invalid parameter: 'data' can't be null.");
        }

        let message = { type: "GeckoView:Message", data: data };
        Messaging.sendRequest(message);
      },
      sendRequestForResult: function(data) {
        if (!data) {
          throw new Error("Invalid parameter: 'data' can't be null.");
        }

        let message = { type: "GeckoView:Message", data: data };
        return Messaging.sendRequestForResult(message);
      }
    };

    
    
    
    sandbox.__SCRIPT_URI_SPEC__ = scriptURL;
    Cu.evalInSandbox("Components.classes['@mozilla.org/moz/jssubscript-loader;1'].createInstance(Components.interfaces.mozIJSSubScriptLoader).loadSubScript(__SCRIPT_URI_SPEC__);", sandbox, "ECMAv5");

    this._scopes[scriptURL] = sandbox;

    if ("load" in sandbox) {
      let params = {
        window: window,
        resourceURI: scriptURL,
      };

      try {
        sandbox["load"](params);
      } catch(e) {
        dump("Exception calling 'load' method in script: " + scriptURL + "\n" + e);
      }
    }
  }
};
