




































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function SimpleTest() {
}

SimpleTest.prototype = {
  classID: Components.ID("{4177e257-a0dc-49b9-a774-522a000a49fa}"),

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsISimpleTest) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  add: function(a, b) {
    dump("add(" + a + "," + b + ") from JS\n");
    return a + b;
  }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([SimpleTest]);
