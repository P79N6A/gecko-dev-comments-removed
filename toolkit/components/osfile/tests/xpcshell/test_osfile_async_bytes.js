"use strict";

Components.utils.import("resource://gre/modules/osfile.jsm");
Components.utils.import("resource://gre/modules/Task.jsm");

function run_test() {
  do_test_pending();
  run_next_test();
}





add_task(function* test_bytes() {
  let path = OS.Path.join(OS.Constants.Path.tmpDir,
                          "test_osfile_async_bytes.tmp");
  let file = yield OS.File.open(path, {trunc: true, read: true, write: true});
  try {
    try {
      
      
      yield file.write(new Uint8Array(2048), {bytes: 1024});
      do_check_eq((yield file.stat()).size, 1024);

      
      yield file.setPosition(0, OS.File.POS_START);
      let read = yield file.readTo(new Uint8Array(1024), {bytes: 512});
      do_check_eq(read, 512);

      
      yield file.setPosition(0, OS.File.POS_END);
      yield file.write(new Uint8Array(1024), null);
      yield file.write(new Uint8Array(1024), undefined);
      do_check_eq((yield file.stat()).size, 3072);
    } finally {
      yield file.close();
    }
  } finally {
    yield OS.File.remove(path);
  }
});

add_task(do_test_finished);
