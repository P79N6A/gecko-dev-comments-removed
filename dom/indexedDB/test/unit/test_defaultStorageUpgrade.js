




var testGenerator = testSteps();

function testSteps()
{
  const openParams = [
    
    { url: "http://localhost", dbName: "dbA", dbVersion: 1 },

    
    { url: "http://www.mozilla.org", dbName: "dbB", dbVersion: 1 },

    
    { url: "http://www.mozilla.org:8080", dbName: "dbC", dbVersion: 1 },

    
    { url: "https://www.mozilla.org", dbName: "dbD", dbVersion: 1 },

    
    { url: "https://www.mozilla.org:8080", dbName: "dbE", dbVersion: 1 },

    
    { url: "indexeddb://fx-devtools", dbName: "dbF",
      dbOptions: { version: 1, storage: "persistent" } },

    
    { url: "moz-safe-about:home", dbName: "dbG",
      dbOptions: { version: 1, storage: "persistent" } },

    
    { url: "file:///Users/joe/", dbName: "dbH", dbVersion: 1 },

    
    { url: "file:///Users/joe/index.html", dbName: "dbI", dbVersion: 1 },

    
    { url: "file:///c:/Users/joe/", dbName: "dbJ", dbVersion: 1 },

    
    { url: "file:///c:/Users/joe/index.html", dbName: "dbK", dbVersion: 1 },

    
    { dbName: "dbL", dbVersion: 1 },

    
    { appId: 1007, inMozBrowser: false, url: "app://system.gaiamobile.org",
      dbName: "dbM", dbVersion: 1 },

    
    { appId: 1007, inMozBrowser: true, url: "https://developer.cdn.mozilla.net",
      dbName: "dbN", dbVersion: 1 },

    
    { url: "http://127.0.0.1", dbName: "dbO", dbVersion: 1 },

    
    { url: "file:///", dbName: "dbP", dbVersion: 1 },

    
    { url: "file:///c:/", dbName: "dbQ", dbVersion: 1 },

    
    { url: "file:///Users/joe/c++/index.html", dbName: "dbR", dbVersion: 1 },

    
    { url: "file:///Users/joe/c///index.html", dbName: "dbR", dbVersion: 1 },

    
    { url: "file:///+/index.html", dbName: "dbS", dbVersion: 1 },

    
    { url: "file://///index.html", dbName: "dbS", dbVersion: 1 },

    
    { url: "http://localhost", dbName: "dbZ",
      dbOptions: { version: 1, storage: "temporary" } }
  ];

  let ios = SpecialPowers.Cc["@mozilla.org/network/io-service;1"]
                         .getService(SpecialPowers.Ci.nsIIOService);

  let ssm = SpecialPowers.Cc["@mozilla.org/scriptsecuritymanager;1"]
                         .getService(SpecialPowers.Ci.nsIScriptSecurityManager);

  function openDatabase(params) {
    let request;
    if ("url" in params) {
      let uri = ios.newURI(params.url, null, null);
      let principal;
      if ("appId" in params) {
        principal = ssm.getAppCodebasePrincipal(uri, params.appId,
                                                params.inMozBrowser);
      } else {
        principal = ssm.getNoAppCodebasePrincipal(uri);
      }
      if ("dbVersion" in params) {
        request = indexedDB.openForPrincipal(principal, params.dbName,
                                             params.dbVersion);
      } else {
        request = indexedDB.openForPrincipal(principal, params.dbName,
                                             params.dbOptions);
      }
    } else {
      if ("dbVersion" in params) {
        request = indexedDB.open(params.dbName, params.dbVersion);
      } else {
        request = indexedDB.open(params.dbName, params.dbOptions);
      }
    }
    return request;
  }

  clearAllDatabases(continueToNextStepSync);
  yield undefined;

  installPackagedProfile("defaultStorageUpgrade_profile");

  for (let params of openParams) {
    let request = openDatabase(params);
    request.onerror = errorHandler;
    request.onupgradeneeded = unexpectedSuccessHandler;
    request.onsuccess = grabEventAndContinueHandler;
    let event = yield undefined;

    is(event.type, "success", "Correct event type");
  }

  resetAllDatabases(continueToNextStepSync);
  yield undefined;

  for (let params of openParams) {
    let request = openDatabase(params);
    request.onerror = errorHandler;
    request.onupgradeneeded = unexpectedSuccessHandler;
    request.onsuccess = grabEventAndContinueHandler;
    let event = yield undefined;

    is(event.type, "success", "Correct event type");
  }

  finishTest();
  yield undefined;
}
