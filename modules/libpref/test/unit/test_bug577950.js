



































function run_test() {
  var ps = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService);

  var pb2= Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefBranch2);

  var pb = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefBranch);

  var observer = {
    QueryInterface: function QueryInterface(aIID) {
      if (aIID.equals(Ci.nsIObserver) ||
          aIID.equals(Ci.nsISupports))
         return this;
      throw Components.results.NS_NOINTERFACE;
    },

    observe: function observe(aSubject, aTopic, aState) {
      
    }
  }

  
  pb.addObserver("UserPref.nonexistent.setIntPref", observer, false);
  pb.addObserver("UserPref.nonexistent.setIntPref", observer, false);
}
