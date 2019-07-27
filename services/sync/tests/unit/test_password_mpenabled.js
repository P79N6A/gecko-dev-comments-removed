


Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-sync/stages/enginesync.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/engines/passwords.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://testing-common/services/sync/utils.js");

function run_test() {
  initTestLogging("Trace");
  run_next_test();
}

add_test(function test_simple() {
  ensureLegacyIdentityManager();
  
  let xpcs = Cc["@mozilla.org/weave/service;1"]
             .getService(Components.interfaces.nsISupports)
             .wrappedJSObject;
  let fxaEnabledGetter = xpcs.__lookupGetter__("fxAccountsEnabled");
  xpcs.__defineGetter__("fxAccountsEnabled", () => true);

  
  let mpEnabledF = Utils.mpEnabled;
  let mpEnabled = false;
  Utils.mpEnabled = function() mpEnabled;

  let manager = Service.engineManager;

  Service.engineManager.register(PasswordEngine);
  let engine = Service.engineManager.get("passwords");
  let wipeCount = 0;
  let engineWipeServerF = engine.wipeServer;
  engine.wipeServer = function() {
    ++wipeCount;
  }

  
  let server  = new SyncServer();
  let johndoe = server.registerUser("johndoe", "password");
  johndoe.createContents({
    meta: {global: {engines: {passwords: {version: engine.version,
                                          syncID: engine.syncID}}}},
    crypto: {},
    clients: {}
  });
  server.start();
  setBasicCredentials("johndoe", "password", "abcdeabcdeabcdeabcdeabcdea");
  Service.serverURL = server.baseURI;
  Service.clusterURL = server.baseURI;

  let engineSync = new EngineSynchronizer(Service);
  engineSync._log.level = Log.Level.Trace;

  function assertEnabled(expected, message) {
    Assert.strictEqual(engine.enabled, expected, message);
    
    Assert.strictEqual(Svc.Prefs.get("engine." + engine.prefName), expected,
                       message + " (pref should match enabled state)");
  }

  try {
    assertEnabled(true, "password engine should be enabled by default")
    let engineMeta = Service.recordManager.get(engine.metaURL);
    
    Assert.notStrictEqual(engineMeta.payload.engines[engine.name], undefined,
                          "The engine should appear in the metadata");
    Assert.ok(!engineMeta.changed, "the metadata for the password engine hasn't changed");

    
    mpEnabled = true;
    
    assertEnabled(false, "if mp is locked the engine should be disabled");
    
    Assert.ok(!manager.isDeclined("passwords"), "password engine is not declined");
    
    
    engineSync._updateEnabledEngines();
    
    engineMeta = Service.recordManager.get(engine.metaURL);
    Assert.strictEqual(engineMeta.payload.engines[engine.name], undefined,
                       "The engine should have vanished");
    
    Assert.strictEqual(wipeCount, 1, "wipeServer should have been called");

    
    
    
    
    let meta = {
      payload: {
        engines: {
          "passwords": {"version":1,"syncID":"yfBi2v7PpFO2"},
        },
      },
    };
    engineSync._updateEnabledFromMeta(meta, 3, manager);
    Assert.strictEqual(wipeCount, 2, "wipeServer should have been called");
    Assert.ok(!manager.isDeclined("passwords"), "password engine is not declined");
    assertEnabled(false, "engine still not enabled locally");

    
    mpEnabled = false;
    
    assertEnabled(false, "engine still not enabled locally");
    
    
    meta = {
      payload: {
        engines: {
          "passwords": 1,
        },
      },
    };
    engineSync._updateEnabledFromMeta(meta, 3, manager);
    Assert.strictEqual(wipeCount, 2, "wipeServer should *not* have been called again");
    Assert.ok(!manager.isDeclined("passwords"), "password engine is not declined");
    
    assertEnabled(true, "engine now enabled locally");
    
    engine._syncStartup();
    
    engineMeta = Service.recordManager.get(engine.metaURL);
    Assert.equal(engineMeta.payload.engines[engine.name].version, engine.version,
                 "The engine should re-appear in the metadata");
  } finally {
    
    engine.wipeServer = engineWipeServerF;
    engine._store.wipe();
    
    Utils.mpEnabled = mpEnabledF;
    xpcs.__defineGetter__("fxAccountsEnabled", fxaEnabledGetter);
    server.stop(run_next_test);
  }
});
