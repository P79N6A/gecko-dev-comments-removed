



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services", "resource://gre/modules/Services.jsm");

function oopCommandlineHandler() {
}

oopCommandlineHandler.prototype = {
    handle: function(cmdLine) {
        let oopFlag = cmdLine.handleFlag("oop", false);
        if (oopFlag) {
            




            let prefs = Services.prefs
            let branch = prefs.getDefaultBranch("");

            try {
                
                
                branch.setBoolPref("dom.ipc.tabs.disabled", false);
                branch.setBoolPref("layers.acceleration.disabled", false);
                branch.setBoolPref("layers.offmainthreadcomposition.enabled", true);
                branch.setBoolPref("layers.offmainthreadcomposition.async-animations", true);
                branch.setBoolPref("layers.async-video.enabled", true);
                branch.setBoolPref("layers.async-pan-zoom.enabled", true);
                branch.setCharPref("gfx.content.azure.backends", "cairo");
            } catch (e) { }

        }
        if (cmdLine.state == Ci.nsICommandLine.STATE_REMOTE_AUTO) {
            cmdLine.preventDefault = true;
        }
    },

    helpInfo: "  --oop        Use out-of-process model in B2G\n",
    classID: Components.ID("{e30b0e13-2d12-4cb0-bc4c-4e617a1bf76e}"),
    QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler]),
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([oopCommandlineHandler]);
