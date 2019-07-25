














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

        throw Components.results.NS_ERROR_NO_INTERFACE;
    },

    val: "<default value>"
};

const kMyCID = Components.ID("{dea98e50-1dd1-11b2-9344-8902b4805a2e}");

const kMyFactory = {
    





 createInstance: function (outer, iid) {
   debug("CI: " + iid + "\n");
   if (outer != null)
     throw Components.results.NS_ERROR_NO_AGGREGATION;

   return (new mySample()).QueryInterface(iid);
  }
};

function NSGetFactory(cid)
{
    if (cid.equals(kMyCID))
        return kMyFactory;
        
    throw Components.results.NS_ERROR_FACTORY_NOT_REGISTERED;
}
