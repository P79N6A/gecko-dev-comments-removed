



"use strict";

const l10n = require("gcli/l10n");

















exports.items = [{
  item: "command",
  runAt: "server",
  name: "edit",
  description: l10n.lookup("editDesc"),
  manual: l10n.lookup("editManual2"),
  params: [
     {
       name: 'resource',
       type: {
         name: 'resource',
         include: 'text/css'
       },
       description: l10n.lookup("editResourceDesc")
     },
     {
       name: "line",
       defaultValue: 1,
       type: {
         name: "number",
         min: 1,
         step: 10
       },
       description: l10n.lookup("editLineToJumpToDesc")
     }
   ],
   returnType: "editArgs",
   exec: args => {
     return { href: args.resource.name, line: args.line };
   }
}, {
  item: "converter",
  from: "editArgs",
  to: "dom",
   exec: function(args, context) {
     let target = context.environment.target;
     let gDevTools = require("resource:///modules/devtools/gDevTools.jsm").gDevTools;
     return gDevTools.showToolbox(target, "styleeditor").then(function(toolbox) {
       let styleEditor = toolbox.getCurrentPanel();
       styleEditor.selectStyleSheet(args.href, args.line);
       return null;
     });
   }
}];
