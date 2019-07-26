



"use strict";

const gcli = require("gcli/index");

exports.items = [{
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
  exec: function(args, context) {
    let target = context.environment.target;
    let gDevTools = require("resource:///modules/devtools/gDevTools.jsm").gDevTools;

    return gDevTools.showToolbox(target, "inspector").then(toolbox => {
      toolbox.getCurrentPanel().selection.setNode(args.selector, "gcli");
    });
  }
}];
