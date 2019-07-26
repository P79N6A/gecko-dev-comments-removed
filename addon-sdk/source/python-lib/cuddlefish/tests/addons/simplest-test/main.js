



const { Cc, Ci } = require("chrome");

exports.main = function(options, callbacks) {
  
  require("api-utils/window-utils").activeBrowserWindow.close();

  
  if (require("api-utils/runtime").OS == "Darwin") {
    let appStartup = Cc['@mozilla.org/toolkit/app-startup;1'].
                     getService(Ci.nsIAppStartup);
    appStartup.quit(appStartup.eAttemptQuit);
  }
}
