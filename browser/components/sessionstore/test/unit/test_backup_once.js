


let toplevel = this;
Cu.import("resource://gre/modules/osfile.jsm");

function run_test() {
  let profd = do_get_profile();
  Cu.import("resource:///modules/sessionstore/SessionFile.jsm", toplevel);
  decoder = new TextDecoder();
  pathStore = OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.js");
  pathBackup = OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.bak");
  let source = do_get_file("data/sessionstore_valid.js");
  source.copyTo(profd, "sessionstore.js");
  run_next_test();
}

let pathStore;
let pathBackup;
let decoder;


add_task(function test_first_write_backup() {
  let content = "test_1";
  let initial_content = decoder.decode(yield OS.File.read(pathStore));

  do_check_true(!(yield OS.File.exists(pathBackup)));
  yield SessionFile.write(content);
  do_check_true(yield OS.File.exists(pathBackup));

  let backup_content = decoder.decode(yield OS.File.read(pathBackup));
  do_check_eq(initial_content, backup_content);
});


add_task(function test_second_write_no_backup() {
  let content = "test_2";
  let initial_content = decoder.decode(yield OS.File.read(pathStore));
  let initial_backup_content = decoder.decode(yield OS.File.read(pathBackup));

  yield SessionFile.write(content);

  let written_content = decoder.decode(yield OS.File.read(pathStore));
  do_check_eq(content, written_content);

  let backup_content = decoder.decode(yield OS.File.read(pathBackup));
  do_check_eq(initial_backup_content, backup_content);
});
