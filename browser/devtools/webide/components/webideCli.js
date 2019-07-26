



const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services", "resource://gre/modules/Services.jsm");







function webideCli() { }

webideCli.prototype = {
  handle: function(cmdLine) {
    let param;

    try {
      
      
      param = cmdLine.handleFlagWithParam("webide", false);
      if (!param) {
        return;
      }
    } catch(e) {
      
      cmdLine.handleFlag("webide", false);
    }

    
    
    
    
    
    cmdLine.preventDefault = true;

    let win = Services.wm.getMostRecentWindow("devtools:webide");
    if (win) {
      win.focus();
      if (param) {
        win.handleCommandline(param);
      }
      return;
    }

    win = Services.ww.openWindow(null,
                                 "chrome://webide/content/",
                                 "webide",
                                 "chrome,centerscreen,resizable,dialog=no",
                                 null);

    if (param) {
      win.addEventListener("load", function onLoad() {
        win.removeEventListener("load", onLoad, true);
        
        win.setTimeout(() => win.handleCommandline(param), 0);
      }, true);
    }

    if (cmdLine.state == Ci.nsICommandLine.STATE_INITIAL_LAUNCH) {
      
      
      
      Services.obs.notifyObservers(null, "sessionstore-windows-restored", "");
    }
  },


  helpInfo : "",

  classID: Components.ID("{79b7b44e-de5e-4e4c-b7a2-044003c615d9}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler]),
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([webideCli]);
