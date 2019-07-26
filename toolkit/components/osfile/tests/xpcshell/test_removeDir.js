



"use strict";

Components.utils.import("resource://gre/modules/osfile.jsm");
Components.utils.import("resource://gre/modules/Task.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

do_register_cleanup(function() {
  Services.prefs.setBoolPref("toolkit.osfile.log", false);
});

function run_test() {
  Services.prefs.setBoolPref("toolkit.osfile.log", true);

  run_next_test();
}

add_task(function() {
  
  
  do_get_profile();

  let file = OS.Path.join(OS.Constants.Path.profileDir, "file");
  let dir = OS.Path.join(OS.Constants.Path.profileDir, "directory");
  let file1 = OS.Path.join(dir, "file1");
  let file2 = OS.Path.join(dir, "file2");
  let subDir = OS.Path.join(dir, "subdir");
  let fileInSubDir = OS.Path.join(subDir, "file");

  
  do_check_false((yield OS.File.exists(dir)));

  
  let exception = null;
  try {
    yield OS.File.removeDir(dir);
  } catch (ex) {
    exception = ex;
  }

  do_check_true(!!exception);
  do_check_true(exception instanceof OS.File.Error);

  
  yield OS.File.removeDir(dir, {ignoreAbsent: true});

  
  yield OS.File.writeAtomic(file, "content", { tmpPath: file + ".tmp" });
  exception = null;
  try {
    yield OS.File.removeDir(file);
  } catch (ex) {
    exception = ex;
  }

  do_check_true(!!exception);
  do_check_true(exception instanceof OS.File.Error);

  
  yield OS.File.makeDir(dir);
  yield OS.File.removeDir(dir);
  do_check_false((yield OS.File.exists(dir)));

  
  yield OS.File.makeDir(dir);
  yield OS.File.writeAtomic(file1, "content", { tmpPath: file1 + ".tmp" });
  
  yield OS.File.removeDir(dir)
  do_check_false((yield OS.File.exists(dir)));

  
  yield OS.File.makeDir(dir);
  yield OS.File.writeAtomic(file1, "content", { tmpPath: file1 + ".tmp" });
  yield OS.File.writeAtomic(file2, "content", { tmpPath: file2 + ".tmp" });
  yield OS.File.removeDir(dir)
  do_check_false((yield OS.File.exists(dir)));

  
  yield OS.File.makeDir(dir);
  yield OS.File.writeAtomic(file1, "content", { tmpPath: file1 + ".tmp" });
  yield OS.File.makeDir(subDir);
  yield OS.File.writeAtomic(fileInSubDir, "content", { tmpPath: fileInSubDir + ".tmp" });
  yield OS.File.removeDir(dir);
  do_check_false((yield OS.File.exists(dir)));
});
