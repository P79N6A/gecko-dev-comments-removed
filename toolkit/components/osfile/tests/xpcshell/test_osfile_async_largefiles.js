


"use strict";

Components.utils.import("resource://gre/modules/ctypes.jsm");
Components.utils.import("resource://gre/modules/osfile.jsm");
Components.utils.import("resource://gre/modules/Task.jsm");







function test_setPosition(forward, current, backward) {
  let path = OS.Path.join(OS.Constants.Path.tmpDir,
                          "test_osfile_async_largefiles.tmp");

  
  try {
    yield OS.File.remove(path);
  } catch (ex if ex.becauseNoSuchFile) {
    
  }

  try {
    let file = yield OS.File.open(path, {write:true, append:false});
    try {
      let pos = 0;

      
      do_print("Moving forward: " + forward);
      yield file.setPosition(forward, OS.File.POS_START);
      pos += forward;
      do_check_eq((yield file.getPosition()), pos);

      
      do_print("Moving current: " + current);
      yield file.setPosition(current, OS.File.POS_CURRENT);
      pos += current;
      do_check_eq((yield file.getPosition()), pos);

      
      do_print("Moving current backward: " + backward);
      yield file.setPosition(-backward, OS.File.POS_CURRENT);
      pos -= backward;
      do_check_eq((yield file.getPosition()), pos);

    } finally {
      yield file.setPosition(0, OS.File.POS_START);
      yield file.close();
    }
  } catch(ex) {
    try {
      yield OS.File.remove(path);
    } catch (ex if ex.becauseNoSuchFile) {
      
    }
    do_throw(ex);
  }
}


function test_setPosition_failures() {
  let path = OS.Path.join(OS.Constants.Path.tmpDir,
                          "test_osfile_async_largefiles.tmp");

  
  try {
    yield OS.File.remove(path);
  } catch (ex if ex.becauseNoSuchFile) {
    
  }

  try {
    let file = yield OS.File.open(path, {write:true, append:false});
    try {
      let pos = 0;

      
      try {
        yield file.setPosition(0.5, OS.File.POS_START);
        do_throw("Shouldn't have succeeded");
      } catch (ex) {
        do_check_true(ex.toString().includes("can't pass"));
      }
      
      
      do_check_eq((yield file.getPosition()), 0);

      
      try {
        yield file.setPosition(0xffffffff + 0.5, OS.File.POS_START);
        do_throw("Shouldn't have succeeded");
      } catch (ex) {
        do_check_true(ex.toString().includes("can't pass"));
      }
      
      
      do_check_eq((yield file.getPosition()), 0);

      
      try {
        
        
        yield file.setPosition(9007199254740992, OS.File.POS_START);
        yield file.setPosition(1, OS.File.POS_CURRENT);
        do_throw("Shouldn't have succeeded");
      } catch (ex) {
        do_print(ex.toString());
        do_check_true(!!ex);
      }

    } finally {
      yield file.setPosition(0, OS.File.POS_START);
      yield file.close();
      try {
        yield OS.File.remove(path);
      } catch (ex if ex.becauseNoSuchFile) {
        
      }
    }
  } catch(ex) {
    do_throw(ex);
  }
}

function run_test() {
  
  add_task(test_setPosition.bind(null, 0, 100, 50));
  add_task(test_setPosition.bind(null, 1000, 100, 50));
  add_task(test_setPosition.bind(null, 1000, -100, -50));

  if (OS.Constants.Win || ctypes.off_t.size >= 8)Â {
    
    
    add_task(test_setPosition.bind(null, 0x7fffffff, 0x7fffffff, 0));
    
    
    
    add_task(test_setPosition.bind(null, 0, 0xffffffff, 0xffffffff));
    
    add_task(test_setPosition.bind(null, 0xffffffff, 0xffffffff, 0xffffffff));
    
    add_task(test_setPosition.bind(null, 0xffffffff, -0x7fffffff, 0x7fffffff));

    
    add_task(test_setPosition_failures);
  }

  run_next_test();
}
