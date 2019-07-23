














function mySample() {  }


mySample.prototype = {
    





    get value()       { return this.val; },
    set value(newval) { return this.val = newval; },

    writeValue: function (aPrefix) {
        debug("mySample::writeValue => " + aPrefix + this.val + "\n");
    },
    poke: function (aValue) { this.val = aValue; },

    QueryInterface: function (iid) {
        if (iid.equals(Components.interfaces.nsISample) ||
            iid.equals(Components.interfaces.nsISupports))
            return this;

        Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
        return null;
    },

    val: "<default value>"
}

var myModule = {
    firstTime: true,

    







    registerSelf: function (compMgr, fileSpec, location, type) {
        if (this.firstTime) {
            debug("*** Deferring registration of sample JS components\n");
            this.firstTime = false;
            throw Components.results.NS_ERROR_FACTORY_REGISTER_AGAIN;
        }
        debug("*** Registering sample JS components\n");
        compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
        compMgr.registerFactoryLocation(this.myCID,
                                        "Sample JS Component",
                                        this.myProgID,
                                        fileSpec,
                                        location,
                                        type);
    },

    


    getClassObject: function (compMgr, cid, iid) {
        if (!cid.equals(this.myCID))
            throw Components.results.NS_ERROR_NO_INTERFACE;

        if (!iid.equals(Components.interfaces.nsIFactory))
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        return this.myFactory;
    },

    
    myCID: Components.ID("{dea98e50-1dd1-11b2-9344-8902b4805a2e}"),

    
    myProgID: "@mozilla.org/jssample;1",

    
    myFactory: {
        





        createInstance: function (outer, iid) {
            debug("CI: " + iid + "\n");
            if (outer != null)
                throw Components.results.NS_ERROR_NO_AGGREGATION;

            return (new mySample()).QueryInterface(iid);
        }
    },

    










    canUnload: function(compMgr) {
        debug("*** Unloading sample JS components\n");
        return true;
    }
};

function NSGetModule(compMgr, fileSpec) {
    return myModule;
}


