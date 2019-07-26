



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
this.EXPORTED_SYMBOLS = [ ];

Cu.import("resource:///modules/devtools/gcli.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

XPCOMUtils.defineLazyModuleGetter(this, "gDevTools",
                                  "resource:///modules/devtools/gDevTools.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TargetFactory",
                                  "resource:///modules/devtools/Target.jsm");




gcli.addCommand({
  name: "inspect",
  description: gcli.lookup("inspectDesc"),
  manual: gcli.lookup("inspectManual"),
  params: [
    {
      name: "selector",
      type: "node",
      description: gcli.lookup("inspectNodeDesc"),
      manual: gcli.lookup("inspectNodeManual")
    }
  ],
  exec: function Command_inspect(args, context) {
    let gBrowser = context.environment.chromeDocument.defaultView.gBrowser;
    let target = TargetFactory.forTab(gBrowser.selectedTab);

    let node = args.selector;

    let inspector = gDevTools.getPanelForTarget("inspector", target);
    if (inspector && inspector.isReady) {
      inspector.selection.setNode(node, "gcli");
    } else {
      let toolbox = gDevTools.openToolboxForTab(target, "inspector");
      toolbox.once("inspector-ready", function(event, panel) {
        let inspector = gDevTools.getPanelForTarget("inspector", target);
        inspector.selection.setNode(node, "gcli");
      }.bind(this));
    }
  }
});
