





































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
