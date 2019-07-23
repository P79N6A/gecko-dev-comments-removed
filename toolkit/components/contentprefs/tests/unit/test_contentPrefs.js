



































function run_test() {
  var cps = Cc["@mozilla.org/content-pref/service;1"].
            getService(Ci.nsIContentPrefService);
  var uri = ContentPrefTest.getURI("http://www.example.com/");
  
  
  

  do_check_eq(cps.getPref(uri, "test.nonexistent.getPref"), undefined);
  do_check_eq(cps.setPref(uri, "test.nonexistent.setPref", 5), undefined);
  do_check_false(cps.hasPref(uri, "test.nonexistent.hasPref"));
  do_check_eq(cps.removePref(uri, "test.nonexistent.removePref"), undefined);


  
  

  cps.setPref(uri, "test.existing", 5);

  
  do_check_eq(cps.getPref(uri, "test.existing"), 5);

  
  do_check_eq(cps.setPref(uri, "test.existing", 6), undefined);
  do_check_eq(cps.getPref(uri, "test.existing"), 6);

  
  do_check_true(cps.hasPref(uri, "test.existing"));

  
  do_check_eq(cps.removePref(uri, "test.existing"), undefined);
  do_check_false(cps.hasPref(uri, "test.existing"));


  
  

  

  cps.setPref(uri, "test.data-integrity.integer", 5);
  do_check_eq(cps.getPref(uri, "test.data-integrity.integer"), 5);

  cps.setPref(uri, "test.data-integrity.float", 5.5);
  do_check_eq(cps.getPref(uri, "test.data-integrity.float"), 5.5);

  cps.setPref(uri, "test.data-integrity.boolean", true);
  do_check_eq(cps.getPref(uri, "test.data-integrity.boolean"), true);

  cps.setPref(uri, "test.data-integrity.string", "test");
  do_check_eq(cps.getPref(uri, "test.data-integrity.string"), "test");

  cps.setPref(uri, "test.data-integrity.null", null);
  do_check_eq(cps.getPref(uri, "test.data-integrity.null"), null);

  

  

  do_check_true(cps.hasPref(uri, "test.data-integrity.integer"));
  do_check_true(cps.hasPref(uri, "test.data-integrity.float"));
  do_check_true(cps.hasPref(uri, "test.data-integrity.boolean"));
  do_check_true(cps.hasPref(uri, "test.data-integrity.string"));
  do_check_true(cps.hasPref(uri, "test.data-integrity.null"));

  do_check_eq(cps.removePref(uri, "test.data-integrity.integer"), undefined);
  do_check_eq(cps.removePref(uri, "test.data-integrity.float"), undefined);
  do_check_eq(cps.removePref(uri, "test.data-integrity.boolean"), undefined);
  do_check_eq(cps.removePref(uri, "test.data-integrity.string"), undefined);
  do_check_eq(cps.removePref(uri, "test.data-integrity.null"), undefined);

  do_check_false(cps.hasPref(uri, "test.data-integrity.integer"));
  do_check_false(cps.hasPref(uri, "test.data-integrity.float"));
  do_check_false(cps.hasPref(uri, "test.data-integrity.boolean"));
  do_check_false(cps.hasPref(uri, "test.data-integrity.string"));
  do_check_false(cps.hasPref(uri, "test.data-integrity.null"));


  
  
  
  cps.setPref(uri, "test.getPrefs.a", 1);
  cps.setPref(uri, "test.getPrefs.b", 2);
  cps.setPref(uri, "test.getPrefs.c", 3);

  var prefs = cps.getPrefs(uri);
  do_check_true(prefs.hasKey("test.getPrefs.a"));
  do_check_eq(prefs.get("test.getPrefs.a"), 1);
  do_check_true(prefs.hasKey("test.getPrefs.b"));
  do_check_eq(prefs.get("test.getPrefs.b"), 2);
  do_check_true(prefs.hasKey("test.getPrefs.c"));
  do_check_eq(prefs.get("test.getPrefs.c"), 3);


  
  

  
  
  var uri1 = ContentPrefTest.getURI("http://www.domain1.com/");
  var uri2 = ContentPrefTest.getURI("http://foo.domain1.com/");
  var uri3 = ContentPrefTest.getURI("http://domain1.com/");
  var uri4 = ContentPrefTest.getURI("http://www.domain2.com/");

  cps.setPref(uri1, "test.site-specificity.uri1", 5);
  do_check_false(cps.hasPref(uri2, "test.site-specificity.uri1"));
  do_check_false(cps.hasPref(uri3, "test.site-specificity.uri1"));
  do_check_false(cps.hasPref(uri4, "test.site-specificity.uri1"));

  cps.setPref(uri2, "test.site-specificity.uri2", 5);
  do_check_false(cps.hasPref(uri1, "test.site-specificity.uri2"));
  do_check_false(cps.hasPref(uri3, "test.site-specificity.uri2"));
  do_check_false(cps.hasPref(uri4, "test.site-specificity.uri2"));

  cps.setPref(uri3, "test.site-specificity.uri3", 5);
  do_check_false(cps.hasPref(uri1, "test.site-specificity.uri3"));
  do_check_false(cps.hasPref(uri2, "test.site-specificity.uri3"));
  do_check_false(cps.hasPref(uri4, "test.site-specificity.uri3"));

  cps.setPref(uri4, "test.site-specificity.uri4", 5);
  do_check_false(cps.hasPref(uri1, "test.site-specificity.uri4"));
  do_check_false(cps.hasPref(uri2, "test.site-specificity.uri4"));
  do_check_false(cps.hasPref(uri3, "test.site-specificity.uri4"));


  
  

  var specificObserver = {
    interfaces: [Ci.nsIContentPrefObserver, Ci.nsISupports],
  
    QueryInterface: function ContentPrefTest_QueryInterface(iid) {
      if (!this.interfaces.some( function(v) { return iid.equals(v) } ))
        throw Cr.NS_ERROR_NO_INTERFACE;
      return this;
    },

    numTimesSetCalled: 0,
    onContentPrefSet: function specificObserver_onContentPrefSet(group, name, value) {
      ++this.numTimesSetCalled;
      do_check_eq(group, "www.example.com");
      do_check_eq(name, "test.observer.1");
      do_check_eq(value, "test value");
    },

    numTimesRemovedCalled: 0,
    onContentPrefRemoved: function specificObserver_onContentPrefRemoved(group, name) {
      ++this.numTimesRemovedCalled;
      do_check_eq(group, "www.example.com");
      do_check_eq(name, "test.observer.1");
    }

  };

  var genericObserver = {
    interfaces: [Ci.nsIContentPrefObserver, Ci.nsISupports],
  
    QueryInterface: function ContentPrefTest_QueryInterface(iid) {
      if (!this.interfaces.some( function(v) { return iid.equals(v) } ))
        throw Cr.NS_ERROR_NO_INTERFACE;
      return this;
    },

    numTimesSetCalled: 0,
    onContentPrefSet: function genericObserver_onContentPrefSet(group, name, value) {
      ++this.numTimesSetCalled;
      do_check_eq(group, "www.example.com");
      if (name != "test.observer.1" && name != "test.observer.2")
        do_throw("genericObserver.onContentPrefSet: " +
                 "name not in (test.observer.1, test.observer.2)");
      do_check_eq(value, "test value");
    },

    numTimesRemovedCalled: 0,
    onContentPrefRemoved: function genericObserver_onContentPrefRemoved(group, name) {
      ++this.numTimesRemovedCalled;
      do_check_eq(group, "www.example.com");
      if (name != "test.observer.1" && name != "test.observer.2")
        do_throw("genericObserver.onContentPrefSet: " +
                 "name not in (test.observer.1, test.observer.2)");
    }

  };

  
  
  
  cps.addObserver("test.observer.1", specificObserver);
  cps.addObserver(null, genericObserver);
  cps.setPref(uri, "test.observer.1", "test value");
  cps.setPref(uri, "test.observer.2", "test value");
  cps.removePref(uri, "test.observer.1");
  cps.removePref(uri, "test.observer.2");
  do_check_eq(specificObserver.numTimesSetCalled, 1);
  do_check_eq(genericObserver.numTimesSetCalled, 2);
  do_check_eq(specificObserver.numTimesRemovedCalled, 1);
  do_check_eq(genericObserver.numTimesRemovedCalled, 2);

  
  
  cps.removeObserver("test.observer.1", specificObserver);
  cps.removeObserver(null, genericObserver);
  cps.setPref(uri, "test.observer.1", "test value");
  cps.removePref(uri, "test.observer.1", "test value");
  do_check_eq(specificObserver.numTimesSetCalled, 1);
  do_check_eq(genericObserver.numTimesSetCalled, 2);
  do_check_eq(specificObserver.numTimesRemovedCalled, 1);
  do_check_eq(genericObserver.numTimesRemovedCalled, 2);
}
