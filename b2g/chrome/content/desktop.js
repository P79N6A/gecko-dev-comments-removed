
window.addEventListener("ContentStart", function(evt) {
  
  
  let require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {})
                  .devtools.require;
  let { TouchEventHandler } = require("devtools/touch-events");
  let touchEventHandler = new TouchEventHandler(shell.contentBrowser);
  touchEventHandler.start();
});
