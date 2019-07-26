



window.addEventListener("ContentStart", function(evt) {
  
  
  let require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {})
                  .devtools.require;
  let { TouchEventHandler } = require("devtools/touch-events");
  let chromeEventHandler = window.QueryInterface(Ci.nsIInterfaceRequestor)
                                 .getInterface(Ci.nsIWebNavigation)
                                 .QueryInterface(Ci.nsIDocShell)
                                 .chromeEventHandler || window;
  let touchEventHandler = new TouchEventHandler(chromeEventHandler);
  touchEventHandler.start();
});
