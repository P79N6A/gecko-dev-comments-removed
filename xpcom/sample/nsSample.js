

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");





function mySample() { }

mySample.prototype = {
    



    classID: Components.ID("{dea98e50-1dd1-11b2-9344-8902b4805a2e}"),

    




    classDescription: "nsSample: JS version", 
    contractID: "@mozilla.org/jssample;1",

    



    QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsISample]),

    





    get value()       { return this.val; },
    set value(newval) { return this.val = newval; },

    writeValue: function (aPrefix) {
        debug("mySample::writeValue => " + aPrefix + this.val + "\n");
    },
    poke: function (aValue) { this.val = aValue; },

    val: "<default value>"
};





if (XPCOMUtils.generateNSGetFactory)
    NSGetFactory = XPCOMUtils.generateNSGetFactory([mySample]);
else
    NSGetModule = XPCOMUtils.generateNSGetModule([mySample]);
