



const { Cc, Ci } = require("chrome");

exports.main = function(options, callbacks) {
  
  require("sdk/deprecated/window-utils").activeBrowserWindow.close();

  
  if (require("sdk/system/runtime").OS == "Darwin") {
    let appStartup = Cc['@mozilla.org/toolkit/app-startup;1'].
                     getService(Ci.nsIAppStartup);
    appStartup.quit(appStartup.eAttemptQuit);
  }
}
