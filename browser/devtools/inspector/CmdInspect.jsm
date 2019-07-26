



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
this.EXPORTED_SYMBOLS = [ ];

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, "gDevTools",
                                  "resource:///modules/devtools/gDevTools.jsm");

const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const gcli = devtools.require('gcli/index');




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
    let target = devtools.TargetFactory.forTab(gBrowser.selectedTab);

    return gDevTools.showToolbox(target, "inspector").then(function(toolbox) {
      toolbox.getCurrentPanel().selection.setNode(args.selector, "gcli");
    }.bind(this));
  }
});
