


"use strict";

let {OS: {File, Path, Constants}} = Components.utils.import("resource://gre/modules/osfile.jsm", {});
Components.utils.import("resource://gre/modules/Task.jsm");














function clearFiles() {
  return Task.spawn(function () {
    let files = ["test_backupTo_option_with_tmpPath.tmp",
                  "test_backupTo_option_without_tmpPath.tmp",
                  "test_non_backupTo_option.tmp",
                  "test_backupTo_option_without_destination_file.tmp",
                  "test_backupTo_option_with_backup_file.tmp"];
    for (let file of files) {
      let path = Path.join(Constants.Path.tmpDir, file);
      yield File.remove(path);
      yield File.remove(path + ".backup");
    }
  });
}

function run_test() {
  run_next_test();
}

add_task(function* init() {
  yield clearFiles();
});








add_task(function* test_backupTo_option_with_tmpPath() {
  let DEFAULT_CONTENTS = "default contents" + Math.random();
  let WRITE_CONTENTS = "abc" + Math.random();
  let path = Path.join(Constants.Path.tmpDir,
                       "test_backupTo_option_with_tmpPath.tmp");
  yield File.writeAtomic(path, DEFAULT_CONTENTS);
  yield File.writeAtomic(path, WRITE_CONTENTS, { tmpPath: path + ".tmp",
                                        backupTo: path + ".backup" });
  do_check_true((yield File.exists(path + ".backup")));
  let contents = yield File.read(path + ".backup");
  do_check_eq(DEFAULT_CONTENTS, (new TextDecoder()).decode(contents));
});








add_task(function* test_backupTo_option_without_tmpPath() {
  let DEFAULT_CONTENTS = "default contents" + Math.random();
  let WRITE_CONTENTS = "abc" + Math.random();
  let path = Path.join(Constants.Path.tmpDir,
                       "test_backupTo_option_without_tmpPath.tmp");
  yield File.writeAtomic(path, DEFAULT_CONTENTS);
  yield File.writeAtomic(path, WRITE_CONTENTS, { backupTo: path + ".backup" });
  do_check_true((yield File.exists(path + ".backup")));
  let contents = yield File.read(path + ".backup");
  do_check_eq(DEFAULT_CONTENTS, (new TextDecoder()).decode(contents));
});








add_task(function* test_non_backupTo_option() {
  let DEFAULT_CONTENTS = "default contents" + Math.random();
  let WRITE_CONTENTS = "abc" + Math.random();
  let path = Path.join(Constants.Path.tmpDir,
                       "test_non_backupTo_option.tmp");
  yield File.writeAtomic(path, DEFAULT_CONTENTS);
  yield File.writeAtomic(path, WRITE_CONTENTS);
  do_check_false((yield File.exists(path + ".backup")));
});








add_task(function* test_backupTo_option_without_destination_file() {
  let DEFAULT_CONTENTS = "default contents" + Math.random();
  let WRITE_CONTENTS = "abc" + Math.random();
  let path = Path.join(Constants.Path.tmpDir,
                       "test_backupTo_option_without_destination_file.tmp");
  yield File.remove(path);
  yield File.writeAtomic(path, WRITE_CONTENTS, { backupTo: path + ".backup" });
  do_check_false((yield File.exists(path + ".backup")));
});









add_task(function* test_backupTo_option_with_backup_file() {
  let DEFAULT_CONTENTS = "default contents" + Math.random();
  let WRITE_CONTENTS = "abc" + Math.random();
  let path = Path.join(Constants.Path.tmpDir,
                       "test_backupTo_option_with_backup_file.tmp");
  yield File.writeAtomic(path, DEFAULT_CONTENTS);

  yield File.writeAtomic(path + ".backup", new Uint8Array(1000));

  yield File.writeAtomic(path, WRITE_CONTENTS, { backupTo: path + ".backup" });
  do_check_true((yield File.exists(path + ".backup")));
  let contents = yield File.read(path + ".backup");
  do_check_eq(DEFAULT_CONTENTS, (new TextDecoder()).decode(contents));
});

add_task(function* cleanup() {
  yield clearFiles();
});
