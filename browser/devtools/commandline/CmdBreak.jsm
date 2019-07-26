



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

this.EXPORTED_SYMBOLS = [ ];

Cu.import("resource:///modules/devtools/gcli.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "HUDService",
                                  "resource:///modules/HUDService.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "TargetFactory",
                                  "resource:///modules/devtools/Target.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "gDevTools",
                                  "resource:///modules/devtools/gDevTools.jsm");




gcli.addCommand({
  name: "break",
  description: gcli.lookup("breakDesc"),
  manual: gcli.lookup("breakManual")
});





gcli.addCommand({
  name: "break list",
  description: gcli.lookup("breaklistDesc"),
  returnType: "html",
  exec: function(args, context) {
    let gBrowser = context.environment.chromeDocument.defaultView.gBrowser;
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    let dbg = gDevTools.getPanelForTarget("jsdebugger", target);

    if (!dbg) {
      return gcli.lookup("breakaddDebuggerStopped");
    }

    let breakpoints = dbg.getAllBreakpoints();

    if (Object.keys(breakpoints).length === 0) {
      return gcli.lookup("breaklistNone");
    }

    let reply = gcli.lookup("breaklistIntro");
    reply += "<ol>";
    for each (let breakpoint in breakpoints) {
      let text = gcli.lookupFormat("breaklistLineEntry",
                                   [breakpoint.location.url,
                                    breakpoint.location.line]);
      reply += "<li>" + text + "</li>";
    };
    reply += "</ol>";
    return reply;
  }
});





gcli.addCommand({
  name: "break add",
  description: gcli.lookup("breakaddDesc"),
  manual: gcli.lookup("breakaddManual")
});




gcli.addCommand({
  name: "break add line",
  description: gcli.lookup("breakaddlineDesc"),
  params: [
    {
      name: "file",
      type: {
        name: "selection",
        data: function() {
          let gBrowser = HUDService.currentContext().gBrowser;
          let target = TargetFactory.forTab(gBrowser.selectedTab);
          let dbg = gDevTools.getPanelForTarget("jsdebugger", target);

          let files = [];
          if (dbg) {
            let sourcesView = dbg.panelWin.DebuggerView.Sources;
            for (let item in sourcesView) {
              files.push(item.value);
            }
          }
          return files;
        }
      },
      description: gcli.lookup("breakaddlineFileDesc")
    },
    {
      name: "line",
      type: { name: "number", min: 1, step: 10 },
      description: gcli.lookup("breakaddlineLineDesc")
    }
  ],
  returnType: "html",
  exec: function(args, context) {
    args.type = "line";

    let gBrowser = context.environment.chromeDocument.defaultView.gBrowser;
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    let dbg = gDevTools.getPanelForTarget("jsdebugger", target);

    if (!dbg) {
      return gcli.lookup("breakaddDebuggerStopped");
    }
    var promise = context.createPromise();
    let position = { url: args.file, line: args.line };
    dbg.addBreakpoint(position, function(aBreakpoint, aError) {
      if (aError) {
        promise.resolve(gcli.lookupFormat("breakaddFailed", [aError]));
        return;
      }
      promise.resolve(gcli.lookup("breakaddAdded"));
    });
    return promise;
  }
});





gcli.addCommand({
  name: "break del",
  description: gcli.lookup("breakdelDesc"),
  params: [
    {
      name: "breakid",
      type: {
        name: "number",
        min: 0,
        max: function() {
          let gBrowser = context.environment.chromeDocument.defaultView.gBrowser;
          let target = TargetFactory.forTab(gBrowser.selectedTab);
          let dbg = gDevTools.getPanelForTarget("jsdebugger", target);

          if (!dbg) {
            return gcli.lookup("breakaddDebuggerStopped");
          }
          return Object.keys(dbg.getAllBreakpoints()).length - 1;
        },
      },
      description: gcli.lookup("breakdelBreakidDesc")
    }
  ],
  returnType: "html",
  exec: function(args, context) {
    let gBrowser = context.environment.chromeDocument.defaultView.gBrowser;
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    let dbg = gDevTools.getPanelForTarget("jsdebugger", target);

    if (!dbg) {
      return gcli.lookup("breakaddDebuggerStopped");
    }

    let breakpoints = dbg.getAllBreakpoints();
    let id = Object.keys(breakpoints)[args.breakid];
    if (!id || !(id in breakpoints)) {
      return gcli.lookup("breakNotFound");
    }

    let promise = context.createPromise();
    try {
      dbg.removeBreakpoint(breakpoints[id], function() {
        promise.resolve(gcli.lookup("breakdelRemoved"));
      });
    } catch (ex) {
      
      promise.resolve(gcli.lookup("breakdelRemoved"));
    }
    return promise;
  }
});
