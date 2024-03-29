



"use strict";

const { Cc, Ci, Cu } = require("chrome");
const TargetFactory = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.TargetFactory;
const l10n = require("gcli/l10n");
const gcli = require("gcli/index");

loader.lazyImporter(this, "gDevTools", "resource:///modules/devtools/gDevTools.jsm");

loader.lazyGetter(this, "Debugger", () => {
  let global = Cu.getGlobalForObject({});
  let JsDebugger = Cu.import("resource://gre/modules/jsdebugger.jsm", {});
  JsDebugger.addDebuggerToGlobal(global);
  return global.Debugger;
});

let debuggers = [];
let chromeDebuggers = [];
let sandboxes = [];

exports.items = [
  {
    name: "calllog",
    description: l10n.lookup("calllogDesc")
  },
  {
    item: "command",
    runAt: "client",
    name: "calllog start",
    description: l10n.lookup("calllogStartDesc"),

    exec: function(args, context) {
      let contentWindow = context.environment.window;

      let dbg = new Debugger(contentWindow);
      dbg.onEnterFrame = function(frame) {
        
        contentWindow.console.log("Method call: " + this.callDescription(frame));
      }.bind(this);

      debuggers.push(dbg);

      let gBrowser = context.environment.chromeDocument.defaultView.gBrowser;
      let target = TargetFactory.forTab(gBrowser.selectedTab);
      gDevTools.showToolbox(target, "webconsole");

      return l10n.lookup("calllogStartReply");
    },

    callDescription: function(frame) {
      let name = "<anonymous>";
      if (frame.callee.name) {
        name = frame.callee.name;
      }
      else {
        let desc = frame.callee.getOwnPropertyDescriptor("displayName");
        if (desc && desc.value && typeof desc.value == "string") {
          name = desc.value;
        }
      }

      let args = frame.arguments.map(this.valueToString).join(", ");
      return name + "(" + args + ")";
    },

    valueToString: function(value) {
      if (typeof value !== "object" || value === null) {
        return uneval(value);
      }
      return "[object " + value.class + "]";
    }
  },
  {
    item: "command",
    runAt: "client",
    name: "calllog stop",
    description: l10n.lookup("calllogStopDesc"),

    exec: function(args, context) {
      let numDebuggers = debuggers.length;
      if (numDebuggers == 0) {
        return l10n.lookup("calllogStopNoLogging");
      }

      for (let dbg of debuggers) {
        dbg.onEnterFrame = undefined;
      }
      debuggers = [];

      return l10n.lookupFormat("calllogStopReply", [ numDebuggers ]);
    }
  },
  {
    item: "command",
    runAt: "client",
    name: "calllog chromestart",
    description: l10n.lookup("calllogChromeStartDesc"),
    get hidden() {
      return gcli.hiddenByChromePref();
    },
    params: [
      {
        name: "sourceType",
        type: {
          name: "selection",
          data: ["content-variable", "chrome-variable", "jsm", "javascript"]
        }
      },
      {
        name: "source",
        type: "string",
        description: l10n.lookup("calllogChromeSourceTypeDesc"),
        manual: l10n.lookup("calllogChromeSourceTypeManual"),
      }
    ],
    exec: function(args, context) {
      let globalObj;
      let contentWindow = context.environment.window;

      if (args.sourceType == "jsm") {
        try {
          globalObj = Cu.import(args.source);
        }
        catch (e) {
          return l10n.lookup("callLogChromeInvalidJSM");
        }
      } else if (args.sourceType == "content-variable") {
        if (args.source in contentWindow) {
          globalObj = Cu.getGlobalForObject(contentWindow[args.source]);
        } else {
          throw new Error(l10n.lookup("callLogChromeVarNotFoundContent"));
        }
      } else if (args.sourceType == "chrome-variable") {
        let chromeWin = context.environment.chromeDocument.defaultView;
        if (args.source in chromeWin) {
          globalObj = Cu.getGlobalForObject(chromeWin[args.source]);
        } else {
          return l10n.lookup("callLogChromeVarNotFoundChrome");
        }
      } else {
        let chromeWin = context.environment.chromeDocument.defaultView;
        let sandbox = new Cu.Sandbox(chromeWin,
                                    {
                                      sandboxPrototype: chromeWin,
                                      wantXrays: false,
                                      sandboxName: "gcli-cmd-calllog-chrome"
                                    });
        let returnVal;
        try {
          returnVal = Cu.evalInSandbox(args.source, sandbox, "ECMAv5");
          sandboxes.push(sandbox);
        } catch(e) {
          
          
          let msg = l10n.lookup("callLogChromeEvalException") + ": " + e;
          Cu.nukeSandbox(sandbox);
          return msg;
        }

        if (typeof returnVal == "undefined") {
          return l10n.lookup("callLogChromeEvalNeedsObject");
        }

        globalObj = Cu.getGlobalForObject(returnVal);
      }

      let dbg = new Debugger(globalObj);
      chromeDebuggers.push(dbg);

      dbg.onEnterFrame = function(frame) {
        
        contentWindow.console.log(l10n.lookup("callLogChromeMethodCall") +
                                  ": " + this.callDescription(frame));
      }.bind(this);

      let gBrowser = context.environment.chromeDocument.defaultView.gBrowser;
      let target = TargetFactory.forTab(gBrowser.selectedTab);
      gDevTools.showToolbox(target, "webconsole");

      return l10n.lookup("calllogChromeStartReply");
    },

    valueToString: function(value) {
      if (typeof value !== "object" || value === null)
        return uneval(value);
      return "[object " + value.class + "]";
    },

    callDescription: function(frame) {
      let name = frame.callee.name || l10n.lookup("callLogChromeAnonFunction");
      let args = frame.arguments.map(this.valueToString).join(", ");
      return name + "(" + args + ")";
    }
  },
  {
    item: "command",
    runAt: "client",
    name: "calllog chromestop",
    description: l10n.lookup("calllogChromeStopDesc"),
    get hidden() {
      return gcli.hiddenByChromePref();
    },
    exec: function(args, context) {
      let numDebuggers = chromeDebuggers.length;
      if (numDebuggers == 0) {
        return l10n.lookup("calllogChromeStopNoLogging");
      }

      for (let dbg of chromeDebuggers) {
        dbg.onEnterFrame = undefined;
        dbg.enabled = false;
      }
      for (let sandbox of sandboxes) {
        Cu.nukeSandbox(sandbox);
      }
      chromeDebuggers = [];
      sandboxes = [];

      return l10n.lookupFormat("calllogChromeStopReply", [ numDebuggers ]);
    }
  }
];
