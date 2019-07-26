



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
    yield OS.File.removeDir(dir, {ignoreAbsent: false});
  } catch (ex) {
    exception = ex;
  }

  do_check_true(!!exception);
  do_check_true(exception instanceof OS.File.Error);

  
  yield OS.File.removeDir(dir, {ignoreAbsent: true});
  yield OS.File.removeDir(dir);

  
  yield OS.File.writeAtomic(file, "content", { tmpPath: file + ".tmp" });
  exception = null;
  try {
    yield OS.File.removeDir(file, {ignoreAbsent: false});
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
  yield OS.File.removeDir(dir);
  do_check_false((yield OS.File.exists(dir)));

  
  yield OS.File.makeDir(dir);
  yield OS.File.writeAtomic(file1, "content", { tmpPath: file1 + ".tmp" });
  yield OS.File.writeAtomic(file2, "content", { tmpPath: file2 + ".tmp" });
  yield OS.File.removeDir(dir);
  do_check_false((yield OS.File.exists(dir)));

  
  yield OS.File.makeDir(dir);
  yield OS.File.writeAtomic(file1, "content", { tmpPath: file1 + ".tmp" });
  yield OS.File.makeDir(subDir);
  yield OS.File.writeAtomic(fileInSubDir, "content", { tmpPath: fileInSubDir + ".tmp" });
  yield OS.File.removeDir(dir);
  do_check_false((yield OS.File.exists(dir)));
});

add_task(function* test_unix_symlink() {
  
  if (OS.Constants.Win) {
    return;
  }

  let file = OS.Path.join(OS.Constants.Path.profileDir, "file");
  let dir = OS.Path.join(OS.Constants.Path.profileDir, "directory");
  let file1 = OS.Path.join(dir, "file1");

  
  
  
  
  
  
  
  
  

  
  do_check_false((yield OS.File.exists(dir)));

  yield OS.File.writeAtomic(file, "content", { tmpPath: file + ".tmp" });
  do_check_true((yield OS.File.exists(file)));
  let info = yield OS.File.stat(file, {unixNoFollowingLinks: true});
  do_check_false(info.isDir);
  do_check_false(info.isSymLink);

  yield OS.File.unixSymLink(file, file + ".link");
  do_check_true((yield OS.File.exists(file + ".link")));
  info = yield OS.File.stat(file + ".link", {unixNoFollowingLinks: true});
  do_check_false(info.isDir);
  do_check_true(info.isSymLink);
  info = yield OS.File.stat(file + ".link");
  do_check_false(info.isDir);
  do_check_false(info.isSymLink);
  yield OS.File.remove(file + ".link");
  do_check_false((yield OS.File.exists(file + ".link")));

  yield OS.File.makeDir(dir);
  do_check_true((yield OS.File.exists(dir)));
  info = yield OS.File.stat(dir, {unixNoFollowingLinks: true});
  do_check_true(info.isDir);
  do_check_false(info.isSymLink);

  let link = OS.Path.join(OS.Constants.Path.profileDir, "linkdir");

  yield OS.File.unixSymLink(dir, link);
  do_check_true((yield OS.File.exists(link)));
  info = yield OS.File.stat(link, {unixNoFollowingLinks: true});
  do_check_false(info.isDir);
  do_check_true(info.isSymLink);
  info = yield OS.File.stat(link);
  do_check_true(info.isDir);
  do_check_false(info.isSymLink);

  let dir3 = OS.Path.join(OS.Constants.Path.profileDir, "directory3");
  let file3 = OS.Path.join(dir3, "file3");
  let link2 = OS.Path.join(dir, "link2");

  yield OS.File.writeAtomic(file1, "content", { tmpPath: file1 + ".tmp" });
  do_check_true((yield OS.File.exists(file1)));
  yield OS.File.makeDir(dir3);
  do_check_true((yield OS.File.exists(dir3)));
  yield OS.File.writeAtomic(file3, "content", { tmpPath: file3 + ".tmp" });
  do_check_true((yield OS.File.exists(file3)));
  yield OS.File.unixSymLink("../directory3", link2);
  do_check_true((yield OS.File.exists(link2)));

  yield OS.File.removeDir(link);
  do_check_false((yield OS.File.exists(link)));
  do_check_true((yield OS.File.exists(file1)));
  yield OS.File.removeDir(dir);
  do_check_false((yield OS.File.exists(dir)));
  do_check_true((yield OS.File.exists(file3)));
  yield OS.File.removeDir(dir3);
  do_check_false((yield OS.File.exists(dir3)));

  
  
  
});
