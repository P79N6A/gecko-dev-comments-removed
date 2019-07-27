



this.EXPORTED_SYMBOLS = ["CompatWarning"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");

function section(number, url)
{
  const baseURL = "https://developer.mozilla.org/en-US/Firefox/Multiprocess_Firefox/Limitations_of_chrome_scripts";
  return { number, url: baseURL + url };
}

let CompatWarning = {
  
  
  
  
  
  delayedWarning: function(msg, addon, warning) {
    function isShimLayer(filename) {
      return filename.indexOf("CompatWarning.jsm") != -1 ||
        filename.indexOf("RemoteAddonsParent.jsm") != -1 ||
        filename.indexOf("RemoteAddonsChild.jsm") != -1 ||
        filename.indexOf("multiprocessShims.js") != -1;
    };

    let stack = Components.stack;
    while (stack && isShimLayer(stack.filename))
      stack = stack.caller;

    let alreadyWarned = false;

    return function() {
      if (alreadyWarned) {
        return;
      }
      alreadyWarned = true;

      if (addon) {
        let histogram = Services.telemetry.getKeyedHistogramById("ADDON_SHIM_USAGE");
        histogram.add(addon, warning ? warning.number : 0);
      }

      if (!Preferences.get("dom.ipc.shims.enabledWarnings", false))
        return;

      let error = Cc['@mozilla.org/scripterror;1'].createInstance(Ci.nsIScriptError);
      if (!error || !Services.console) {
        
        return;
      }

      let message = `Warning: ${msg}`;
      if (warning)
        message += `\nMore info at: ${warning.url}`;

      error.init(
                  message,
                  stack ? stack.filename : "",
                  stack ? stack.sourceLine : "",
                  stack ? stack.lineNumber : 0,
                  0,
                  Ci.nsIScriptError.warningFlag,
                  "chrome javascript");
      Services.console.logMessage(error);
    };
  },

  warn: function(msg, addon, warning) {
    let delayed = this.delayedWarning(msg, addon, warning);
    delayed();
  },

  warnings: {
    content: section(1, "#gBrowser.contentWindow.2C_window.content..."),
    limitations_of_CPOWs: section(2, "#Limitations_of_CPOWs"),
    nsIContentPolicy: section(3, "#nsIContentPolicy"),
    nsIWebProgressListener: section(4, "#nsIWebProgressListener"),
    observers: section(5, "#Observers_in_the_chrome_process"),
    DOM_events: section(6, "#DOM_Events"),
    sandboxes: section(7, "#Sandboxes"),
    JSMs: section(8, "#JavaScript_code_modules_(JSMs)"),
    nsIAboutModule: section(9, "#nsIAboutModule"),
    
    
  },
};
