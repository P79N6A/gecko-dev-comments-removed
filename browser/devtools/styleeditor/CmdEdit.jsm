



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

this.EXPORTED_SYMBOLS = [ ];

let devtools = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
const gcli = devtools.require("gcli/index");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "gDevTools",
                                  "resource:///modules/devtools/gDevTools.jsm");




gcli.addCommand({
  name: "edit",
  description: gcli.lookup("editDesc"),
  manual: gcli.lookup("editManual2"),
  params: [
     {
       name: 'resource',
       type: {
         name: 'resource',
         include: 'text/css'
       },
       description: gcli.lookup("editResourceDesc")
     },
     {
       name: "line",
       defaultValue: 1,
       type: {
         name: "number",
         min: 1,
         step: 10
       },
       description: gcli.lookup("editLineToJumpToDesc")
     }
   ],
   exec: function(args, context) {
     let target = context.environment.target;
     return gDevTools.showToolbox(target, "styleeditor").then(function(toolbox) {
       let styleEditor = toolbox.getCurrentPanel();
       styleEditor.selectStyleSheet(args.resource.element, args.line);
       return null;
     });
   }
});
