



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
let EXPORTED_SYMBOLS = [ ];

Cu.import("resource:///modules/devtools/gcli.jsm");




gcli.addCommand({
  name: "export",
  description: gcli.lookup("exportDesc"),
});





gcli.addCommand({
  name: "export html",
  description: gcli.lookup("exportHtmlDesc"),
  exec: function(args, context) {
    let document = context.environment.contentDocument;
    let window = document.defaultView;
    let page = document.documentElement.outerHTML;
    window.open('data:text/plain;charset=utf8,' + encodeURIComponent(page));
  }
});
