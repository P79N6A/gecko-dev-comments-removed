


"use strict"

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");


{
  let commonFile = do_get_file("../head_common.js", false);
  let uri = Services.io.newFileURI(commonFile);
  Services.scriptloader.loadSubScript(uri.spec, this);
}



const DB_FILENAME = "places.sqlite";










let setupPlacesDatabase = Task.async(function* (aFileName) {
  let currentDir = yield OS.File.getCurrentDirectory();

  let src = OS.Path.join(currentDir, aFileName);
  Assert.ok((yield OS.File.exists(src)), "Database file found");

  
  let dest = OS.Path.join(OS.Constants.Path.profileDir, DB_FILENAME);
  Assert.ok(!(yield OS.File.exists(dest)), "Database file should not exist yet");

  yield OS.File.copy(src, dest);
});


function run_test() {
  run_next_test();
}
