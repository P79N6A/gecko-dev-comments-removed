


"use strict";

Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource:///modules/experiments/Experiments.jsm");

const FILE_CACHE               = "experiments.json";
const PREF_EXPERIMENTS_ENABLED = "experiments.enabled";
const PREF_LOGGING_LEVEL       = "experiments.logging.level";
const PREF_LOGGING_DUMP        = "experiments.logging.dump";
const PREF_MANIFEST_URI        = "experiments.manifest.uri";


let gProfileDir = null;
let gHttpServer = null;
let gHttpRoot   = null;
let gPolicy     = new Experiments.Policy();

function removeCacheFile() {
  let path = OS.Path.join(OS.Constants.Path.profileDir, FILE_CACHE);
  return OS.File.remove(path);
}

function run_test() {
  createAppInfo();
  gProfileDir = do_get_profile();

  gHttpServer = new HttpServer();
  gHttpServer.start(-1);
  let port = gHttpServer.identity.primaryPort;
  gHttpRoot = "http://localhost:" + port + "/";
  gHttpServer.registerDirectory("/", do_get_cwd());
  do_register_cleanup(() => gHttpServer.stop(() => {}));

  disableCertificateChecks();

  Services.prefs.setBoolPref(PREF_EXPERIMENTS_ENABLED, true);
  Services.prefs.setIntPref(PREF_LOGGING_LEVEL, 0);
  Services.prefs.setBoolPref(PREF_LOGGING_DUMP, true);

  patchPolicy(gPolicy, {
    updatechannel: () => "nightly",
  });

  run_next_test();
}

add_task(function* test_fetchAndCache() {
  Services.prefs.setCharPref(PREF_MANIFEST_URI, gHttpRoot + "experiments_1.manifest");
  let ex = new Experiments.Experiments(gPolicy);

  Assert.equal(ex._experiments.size, 0, "There should be no cached experiments yet.");
  yield ex.updateManifest();
  Assert.notEqual(ex._experiments.size, 0, "There should be cached experiments now.");

  yield ex.uninit();
});

add_task(function* test_checkCache() {
  let ex = new Experiments.Experiments(gPolicy);
  Assert.equal(ex._experiments.size, 0, "There should be no cached experiments yet.");

  yield ex._loadFromCache();
  Assert.notEqual(ex._experiments.size, 0, "There should be cached experiments now.");

  yield ex.uninit();
});

add_task(function* test_fetchInvalid() {
  Services.prefs.setCharPref(PREF_MANIFEST_URI, gHttpRoot + "invalid.manifest");
  yield removeCacheFile();

  let ex = new Experiments.Experiments(gPolicy);
  Assert.equal(ex._experiments.size, 0, "There should be no cached experiments yet.");

  let error;
  yield ex.updateManifest().then(() => error = false, () => error = true);
  Assert.ok(error, "Updating the manifest should not have succeeded.");
  Assert.equal(ex._experiments.size, 0, "There should still be no cached experiments.");

  yield ex.uninit();
});
