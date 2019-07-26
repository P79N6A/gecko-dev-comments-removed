"use strict";

do_print("starting tests");

Components.utils.import("resource://gre/modules/osfile.jsm");
Components.utils.import("resource://gre/modules/Task.jsm");






function setup_mode(mode) {
  
  let realMode = {
    read: true,
    write: true
  };
  for (let k in mode) {
    realMode[k] = mode[k];
  }
  return realMode;
}


function test_append(mode) {
  let path = OS.Path.join(OS.Constants.Path.tmpDir,
                          "test_osfile_async_append.tmp");

  
  try {
    yield OS.File.remove(path);
  } catch (ex if ex.becauseNoSuchFile) {
    
  }

  try {
    mode = setup_mode(mode);
    mode.append = true;
    if (mode.trunc) {
      
      yield OS.File.writeAtomic(path, new Uint8Array(500));
    }
    let file = yield OS.File.open(path, mode);
    try {
      yield file.write(new Uint8Array(1000));
      yield file.setPosition(0, OS.File.POS_START);
      yield file.read(100);
      
      yield file.write(new Uint8Array(100));
      
      let stat = yield file.stat();
      do_check_eq(1100, stat.size);
    } finally {
      yield file.close();
    }
  } catch(ex) {
    try {
      yield OS.File.remove(path);
    } catch (ex if ex.becauseNoSuchFile) {
      
    }
  }
}


function test_no_append(mode) {
  let path = OS.Path.join(OS.Constants.Path.tmpDir,
                          "test_osfile_async_noappend.tmp");

  
  try {
    yield OS.File.remove(path);
  } catch (ex if ex.becauseNoSuchFile) {
    
  }

  try {
    mode = setup_mode(mode);
    mode.append = false;
    if (mode.trunc) {
      
      yield OS.File.writeAtomic(path, new Uint8Array(500));
    }
    let file = yield OS.File.open(path, mode);
    try {
      yield file.write(new Uint8Array(1000));
      yield file.setPosition(0, OS.File.POS_START);
      yield file.read(100);
      
      yield file.write(new Uint8Array(100));
      
      let stat = yield file.stat();
      do_check_eq(1000, stat.size);
    } finally {
      yield file.close();
    }
  } finally {
    try {
      yield OS.File.remove(path);
    } catch (ex if ex.becauseNoSuchFile) {
      
    }
  }
}

let test_flags = [
  {},
  {create:true},
  {trunc:true}
];
function run_test() {
  do_test_pending();

  for (let t of test_flags) {
    add_task(test_append.bind(null, t));
    add_task(test_no_append.bind(null, t));
  }
  add_task(do_test_finished);

  run_next_test();
}
