








"use strict";
Cu.import("resource://gre/modules/osfile.jsm", this);

const Telemetry = Services.telemetry;
const Path = OS.Path;
const HistogramId = "FX_SESSION_RESTORE_ALL_FILES_CORRUPT";


let profd = do_get_profile();
Cu.import("resource:///modules/sessionstore/SessionFile.jsm", this);





function reset_session(backups = {}) {

  
  Telemetry.getHistogramById(HistogramId).clear();

  
  OS.File.makeDir(SessionFile.Paths.backups);
  for (let key of SessionFile.Paths.loadOrder) {
    if (backups.hasOwnProperty(key)) {
      OS.File.copy(backups[key], SessionFile.Paths[key]);
    } else {
      OS.File.remove(SessionFile.Paths[key]);
    }
  }
}






add_task(function* test_ensure_histogram_exists_and_empty() {
  let s = Telemetry.getHistogramById(HistogramId).snapshot();
  Assert.equal(s.sum, 0, "Initially, the sum of probes is 0");
});





add_task(function* test_no_files_exist() {
  
  reset_session();

  yield SessionFile.read();
  
  let h = Telemetry.getHistogramById(HistogramId);
  let s = h.snapshot();
  Assert.equal(s.counts[0], 1, "One probe for the 'false' bucket.");
  Assert.equal(s.counts[1], 0, "No probes in the 'true' bucket.");
});





add_task(function* test_one_file_valid() {
  
  let invalidSession = "data/sessionstore_invalid.js";
  let validSession = "data/sessionstore_valid.js";
  reset_session({
    clean : invalidSession,
    cleanBackup: validSession,
    recovery: invalidSession,
    recoveryBackup: invalidSession
  });

  yield SessionFile.read();
  
  let h = Telemetry.getHistogramById(HistogramId);
  let s = h.snapshot();
  Assert.equal(s.counts[0], 1, "One probe for the 'false' bucket.");
  Assert.equal(s.counts[1], 0, "No probes in the 'true' bucket.");
});





add_task(function* test_all_files_corrupt() {
  
  let invalidSession = "data/sessionstore_invalid.js";
  reset_session({
    clean : invalidSession,
    cleanBackup: invalidSession,
    recovery: invalidSession,
    recoveryBackup: invalidSession
  });

  yield SessionFile.read();
  
  let h = Telemetry.getHistogramById(HistogramId);
  let s = h.snapshot();
  Assert.equal(s.counts[1], 1, "One probe for the 'true' bucket.");
  Assert.equal(s.counts[0], 0, "No probes in the 'false' bucket.");
});

function run_test() {
  run_next_test();
}
