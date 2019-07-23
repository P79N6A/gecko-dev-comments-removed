



































function run_test() {
  
  

  
  
  

  
  {
    ContentPrefTest.deleteDatabase();

    
    let cps = Cc["@mozilla.org/content-pref/service;1"].
              createInstance(Ci.nsIContentPrefService);
    do_check_true(cps.DBConnection.connectionReady);
    cps.DBConnection.close();
  }

  
  {
    let dbFile = ContentPrefTest.deleteDatabase();

    let cps = Cc["@mozilla.org/content-pref/service;1"].
               createInstance(Ci.nsIContentPrefService);
    cps.DBConnection.close();
    do_check_true(dbFile.exists());

    
    cps = Cc["@mozilla.org/content-pref/service;1"].
          createInstance(Ci.nsIContentPrefService);
    do_check_true(cps.DBConnection.connectionReady);
    cps.DBConnection.close();
  }

  
  {
    let dbFile = ContentPrefTest.deleteDatabase();

    
    let dbService = Cc["@mozilla.org/storage/service;1"].
                    getService(Ci.mozIStorageService);
    let dbConnection = dbService.openDatabase(dbFile);
    do_check_eq(dbConnection.schemaVersion, 0);
    dbConnection.close();
    do_check_true(dbFile.exists());

    
    let cps = Cc["@mozilla.org/content-pref/service;1"].
              createInstance(Ci.nsIContentPrefService);
    do_check_neq(cps.DBConnection.schemaVersion, 0);
    cps.DBConnection.close();
  }

  
  {
    let dbFile = ContentPrefTest.deleteDatabase();
    let backupDBFile = ContentPrefTest.deleteBackupDatabase();

    
    let foStream = Cc["@mozilla.org/network/file-output-stream;1"].
                   createInstance(Ci.nsIFileOutputStream);
    foStream.init(dbFile, 0x02 | 0x08 | 0x20, 0666, 0);
    let garbageData = "garbage that makes SQLite think the file is corrupted";
    foStream.write(garbageData, garbageData.length);
    foStream.close();

    
    let cps = Cc["@mozilla.org/content-pref/service;1"].
              createInstance(Ci.nsIContentPrefService);
    do_check_true(backupDBFile.exists());
    do_check_true(cps.DBConnection.connectionReady);

    cps.DBConnection.close();
  }

  
  {
    let dbFile = ContentPrefTest.deleteDatabase();
    let backupDBFile = ContentPrefTest.deleteBackupDatabase();

    
    
    let dbService = Cc["@mozilla.org/storage/service;1"].
                    getService(Ci.mozIStorageService);
    let dbConnection = dbService.openDatabase(dbFile);
    dbConnection.schemaVersion = -1;
    dbConnection.close();
    do_check_true(dbFile.exists());

    
    let cps = Cc["@mozilla.org/content-pref/service;1"].
              createInstance(Ci.nsIContentPrefService);
    do_check_true(backupDBFile.exists());
    do_check_true(cps.DBConnection.connectionReady);

    cps.DBConnection.close();
  }


  
  var cps = Cc["@mozilla.org/content-pref/service;1"].
            getService(Ci.nsIContentPrefService);

  var uri = ContentPrefTest.getURI("http://www.example.com/");

  
  var statement = cps.DBConnection.createStatement("PRAGMA synchronous");
  statement.executeStep();
  do_check_eq(0, statement.getInt32(0));

  
  

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


  
  
  
  {
    var anObserver = {
      interfaces: [Ci.nsIContentPrefObserver, Ci.nsISupports],

      QueryInterface: function ContentPrefTest_QueryInterface(iid) {
        if (!this.interfaces.some( function(v) { return iid.equals(v) } ))
          throw Cr.NS_ERROR_NO_INTERFACE;
        return this;
      },

      onContentPrefSet: function anObserver_onContentPrefSet(group, name, value) {
      },

      expectedDomains: [],
      numTimesRemovedCalled: 0,
      onContentPrefRemoved: function anObserver_onContentPrefRemoved(group, name) {
        ++this.numTimesRemovedCalled;

        
        var index = this.expectedDomains.indexOf(group);
        do_check_true(index >= 0);
        this.expectedDomains.splice(index, 1);
      }
    };

    var uri1 = ContentPrefTest.getURI("http://www.domain1.com/");
    var uri2 = ContentPrefTest.getURI("http://foo.domain1.com/");
    var uri3 = ContentPrefTest.getURI("http://domain1.com/");
    var uri4 = ContentPrefTest.getURI("http://www.domain2.com/");

    cps.setPref(uri1, "test.byname.1", 1);
    cps.setPref(uri1, "test.byname.2", 2);
    cps.setPref(uri2, "test.byname.1", 4);
    cps.setPref(uri3, "test.byname.3", 8);
    cps.setPref(uri4, "test.byname.1", 16);
    cps.setPref(null, "test.byname.1", 32);
    cps.setPref(null, "test.byname.2", false);
    
    function enumerateAndCheck(testName, expectedSum, expectedDomains) {
      var prefsByName = cps.getPrefsByName(testName);
      var enumerator = prefsByName.enumerator;
      var sum = 0;
      while (enumerator.hasMoreElements()) {
        var property = enumerator.getNext().QueryInterface(Components.interfaces.nsIProperty);
        sum += parseInt(property.value);

        
        var index = expectedDomains.indexOf(property.name);
        do_check_true(index >= 0);
        expectedDomains.splice(index, 1);
      }
      do_check_eq(sum, expectedSum);
      
      do_check_eq(expectedDomains.length, 0);
    }
    
    enumerateAndCheck("test.byname.1", 53,
      ["foo.domain1.com", null, "www.domain1.com", "www.domain2.com"]);
    enumerateAndCheck("test.byname.2", 2, ["www.domain1.com", null]);
    enumerateAndCheck("test.byname.3", 8, ["domain1.com"]);

    cps.addObserver("test.byname.1", anObserver);
    anObserver.expectedDomains = ["foo.domain1.com", null, "www.domain1.com", "www.domain2.com"];

    cps.removePrefsByName("test.byname.1");
    do_check_false(cps.hasPref(uri1, "test.byname.1"));
    do_check_false(cps.hasPref(uri2, "test.byname.1"));
    do_check_false(cps.hasPref(uri3, "test.byname.1"));
    do_check_false(cps.hasPref(uri4, "test.byname.1"));
    do_check_false(cps.hasPref(null, "test.byname.1"));
    do_check_true(cps.hasPref(uri1, "test.byname.2"));
    do_check_true(cps.hasPref(uri3, "test.byname.3"));

    do_check_eq(anObserver.numTimesRemovedCalled, 4);
    do_check_eq(anObserver.expectedDomains.length, 0);
 
    cps.removeObserver("test.byname.1", anObserver);
    
    
    cps.removePref(uri1, "test.byname.2");
    cps.removePref(uri3, "test.byname.3");
    cps.removePref(null, "test.byname.2");
  }


  
  

  {
    let uri1 = ContentPrefTest.getURI("http://www.domain1.com/");
    let uri2 = ContentPrefTest.getURI("http://www.domain2.com/");
    let uri3 = ContentPrefTest.getURI("http://www.domain3.com/");

    let dbConnection = cps.DBConnection;

    let prefCount = dbConnection.createStatement("SELECT COUNT(*) AS count FROM prefs");

    let groupCount = dbConnection.createStatement("SELECT COUNT(*) AS count FROM groups");

    
    cps.setPref(uri1, "test.removeAllGroups", 1);
    cps.setPref(uri2, "test.removeAllGroups", 2);
    cps.setPref(uri3, "test.removeAllGroups", 3);

    
    cps.setPref(null, "test.removeAllGroups", 1);

    
    prefCount.executeStep();
    do_check_true(prefCount.row.count > 0);
    prefCount.reset();
    groupCount.executeStep();
    do_check_true(groupCount.row.count > 0);
    groupCount.reset();

    
    
    cps.removeGroupedPrefs();

    
    
    prefCount.executeStep();
    do_check_true(prefCount.row.count == 1);
    prefCount.reset();
    groupCount.executeStep();
    do_check_true(groupCount.row.count == 0);
    groupCount.reset();
    let globalPref = dbConnection.createStatement("SELECT groupID FROM prefs");
    globalPref.executeStep();
    do_check_true(globalPref.row.groupID == null);
    globalPref.reset();
  }
}
