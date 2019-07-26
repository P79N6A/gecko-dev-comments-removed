



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

this.EXPORTED_SYMBOLS = [ ];

Cu.import("resource:///modules/devtools/gcli.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "HUDService",
                                  "resource:///modules/HUDService.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "gDevTools",
                                  "resource:///modules/devtools/gDevTools.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TargetFactory",
                                  "resource:///modules/devtools/Target.jsm");




gcli.addCommand({
  name: "console",
  description: gcli.lookup("consoleDesc"),
  manual: gcli.lookup("consoleManual")
});




gcli.addCommand({
  name: "console clear",
  description: gcli.lookup("consoleclearDesc"),
  exec: function Command_consoleClear(args, context) {
    let window = context.environment.contentDocument.defaultView;
    let hud = HUDService.getHudByWindow(window);
    
    if (hud) {
      hud.jsterm.clearOutput();
    }
  }
});




gcli.addCommand({
  name: "console close",
  description: gcli.lookup("consolecloseDesc"),
  exec: function Command_consoleClose(args, context) {
    let gBrowser = context.environment.chromeDocument.defaultView.gBrowser;
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    gDevTools.closeToolbox(target);
  }
});




gcli.addCommand({
  name: "console open",
  description: gcli.lookup("consoleopenDesc"),
  exec: function Command_consoleOpen(args, context) {
    let gBrowser = context.environment.chromeDocument.defaultView.gBrowser;
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    gDevTools.openToolboxForTab(target, "webconsole");
  }
});
