



"use strict";

const l10n = require("gcli/l10n");

exports.items = [{
  item: "command",
  runAt: "server",
  name: "inspect",
  description: l10n.lookup("inspectDesc"),
  manual: l10n.lookup("inspectManual"),
  params: [
    {
      name: "selector",
      type: "node",
      description: l10n.lookup("inspectNodeDesc"),
      manual: l10n.lookup("inspectNodeManual")
    }
  ],
  exec: function(args, context) {
    let target = context.environment.target;
    let gDevTools = require("resource:///modules/devtools/gDevTools.jsm").gDevTools;

    return gDevTools.showToolbox(target, "inspector").then(toolbox => {
      toolbox.getCurrentPanel().selection.setNode(args.selector, "gcli");
    });
  }
}];
