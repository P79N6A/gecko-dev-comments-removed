



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

let EXPORTED_SYMBOLS = [ ];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/gcli.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "HUDService",
                                  "resource:///modules/HUDService.jsm");




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
     let win = HUDService.currentContext();
     win.StyleEditor.openChrome(args.resource.element, args.line);
   }
});
