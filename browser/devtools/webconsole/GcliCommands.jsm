





































let EXPORTED_SYMBOLS = [ "GcliCommands" ];

Components.utils.import("resource:///modules/gcli.jsm");
Components.utils.import("resource:///modules/HUDService.jsm");





gcli.addCommand({
  name: "echo",
  description: gcli.lookup("echoDesc"),
  params: [
    {
      name: "message",
      type: "string",
      description: gcli.lookup("echoMessageDesc")
    }
  ],
  returnType: "string",
  exec: function Command_echo(args, context) {
    return args.message;
  }
});





gcli.addCommand({
  name: "console",
  description: gcli.lookup("consoleDesc"),
  manual: gcli.lookup("consoleManual")
});




gcli.addCommand({
  name: "console clear",
  description: gcli.lookup("consoleclearDesc"),
  exec: function Command_consoleClear(args, context) {
    let window = context.environment.chromeDocument.defaultView;
    let hud = HUDService.getHudReferenceById(context.environment.hudId);

    
    let threadManager = Components.classes["@mozilla.org/thread-manager;1"]
        .getService(Components.interfaces.nsIThreadManager);
    threadManager.mainThread.dispatch({
      run: function() {
        hud.gcliterm.clearOutput();
      }
    }, Components.interfaces.nsIThread.DISPATCH_NORMAL);
  }
});





gcli.addCommand({
  name: "console close",
  description: gcli.lookup("consolecloseDesc"),
  exec: function Command_consoleClose(args, context) {
    let tab = HUDService.getHudReferenceById(context.environment.hudId).tab;
    HUDService.deactivateHUDForContext(tab);
  }
});




gcli.addCommand({
  name: "inspect",
  description: gcli.lookup("inspectDesc"),
  manual: gcli.lookup("inspectManual"),
  params: [
    {
      name: "node",
      type: "node",
      description: gcli.lookup("inspectNodeDesc"),
      manual: gcli.lookup("inspectNodeManual")
    }
  ],
  exec: function Command_inspect(args, context) {
    let document = context.environment.chromeDocument;
    document.defaultView.InspectorUI.openInspectorUI(args.node);
  }
});

let breakpoints = [];




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
    if (breakpoints.length === 0) {
      return gcli.lookup("breaklistNone");
    }

    let reply = gcli.lookup("breaklistIntro");
    reply += "<ol>";
    breakpoints.forEach(function(breakpoint) {
      let text = gcli.lookupFormat("breaklistLineEntry",
                                   [breakpoint.file, breakpoint.line]);
      reply += "<li>" + text + "</li>";
    });
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
          let win = HUDService.currentContext();
          let dbg = win.DebuggerUI.getDebugger(win.gBrowser.selectedTab);
          let files = [];
          if (dbg) {
            let scriptsView = dbg.frame.contentWindow.DebuggerView.Scripts;
            for each (let script in scriptsView.scriptLocations()) {
              files.push(script);
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
    let win = HUDService.currentContext();
    let dbg = win.DebuggerUI.getDebugger(win.gBrowser.selectedTab);
    if (!dbg) {
      return gcli.lookup("breakaddDebuggerStopped");
    }
    var promise = context.createPromise();
    let position = { url: args.file, line: args.line };
    dbg.activeThread.setBreakpoint(position, function(aResponse, aBpClient) {
      if (aResponse.error) {
        promise.resolve(gcli.lookupFormat("breakaddFailed",
                        [ aResponse.error ]));
        return;
      }
      args.client = aBpClient;
      breakpoints.push(args);
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
        max: function() { return breakpoints.length - 1; }
      },
      description: gcli.lookup("breakdelBreakidDesc")
    }
  ],
  returnType: "html",
  exec: function(args, context) {
    let breakpoint = breakpoints.splice(args.breakid, 1)[0];
    var promise = context.createPromise();
    try {
      breakpoint.client.remove(function(aResponse) {
                                 promise.resolve(gcli.lookup("breakdelRemoved"));
                               });
    } catch (ex) {
      
      promise.resolve(gcli.lookup("breakdelRemoved"));
    }
    return promise;
  }
});
