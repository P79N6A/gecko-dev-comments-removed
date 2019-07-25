




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

function Prompter() {
    this.init();
}

Prompter.prototype = {
    classDescription : "Prompter",
    contractID       : "@mozilla.org/prompter;1",
    classID          : Components.ID("{1c978d25-b37f-43a8-a2d6-0c7a239ead87}"),
    QueryInterface   : XPCOMUtils.generateQI([Ci.nsISupports]),


    


    debug    : true,

    init : function () {
        this.log("initialized!");
    },


    log : function (message) {
        if (!this.debug)
            return;
        dump("Prompter: " + message + "\n");
        Services.console.logStringMessage("Prompter: " + message);
    },


    
};


var component = [Prompter];
function NSGetModule (compMgr, fileSpec) {
    return XPCOMUtils.generateModule(component);
}
