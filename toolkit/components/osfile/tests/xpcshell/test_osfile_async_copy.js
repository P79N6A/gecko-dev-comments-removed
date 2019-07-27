"use strict";

Components.utils.import("resource://gre/modules/osfile.jsm");
Components.utils.import("resource://gre/modules/FileUtils.jsm");
Components.utils.import("resource://gre/modules/NetUtil.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/Promise.jsm");
Components.utils.import("resource://gre/modules/Task.jsm");

function run_test() {
  do_test_pending();
  run_next_test();
}




let EXISTING_FILE = "test_osfile_async_copy.js";










let reference_fetch_file = function reference_fetch_file(path) {
  let promise = Promise.defer();
  let file = new FileUtils.File(path);
  NetUtil.asyncFetch2(
    file,
    function(stream, status) {
      if (!Components.isSuccessCode(status)) {
        promise.reject(status);
        return;
      }
      let result, reject;
      try {
        result = NetUtil.readInputStreamToString(stream, stream.available());
      } catch (x) {
        reject = x;
      }
      stream.close();
      if (reject) {
        promise.reject(reject);
      } else {
        promise.resolve(result);
      }
    },
    null,      
    Services.scriptSecurityManager.getSystemPrincipal(),
    null,      
    Ci.nsILoadInfo.SEC_NORMAL,
    Ci.nsIContentPolicy.TYPE_OTHER);

  return promise.promise;
};











let reference_compare_files = function reference_compare_files(a, b) {
  let a_contents = yield reference_fetch_file(a);
  let b_contents = yield reference_fetch_file(b);
  
  
  do_check_true(a_contents === b_contents);
};




function test_copymove(options = {}) {
  let source = OS.Path.join((yield OS.File.getCurrentDirectory()),
                            EXISTING_FILE);
  let dest = OS.Path.join(OS.Constants.Path.tmpDir,
                          "test_osfile_async_copy_dest.tmp");
  let dest2 = OS.Path.join(OS.Constants.Path.tmpDir,
                           "test_osfile_async_copy_dest2.tmp");
  try {
    
    yield OS.File.copy(source, dest, options);
    yield reference_compare_files(source, dest);
    
    yield OS.File.move(dest, dest2);
    yield reference_compare_files(source, dest2);
    
    do_check_eq((yield OS.File.exists(dest)), false);
  } finally {
    try {
      yield OS.File.remove(dest);
    } catch (ex if ex.becauseNoSuchFile) {
      
    }
    try {
      yield OS.File.remove(dest2);
    } catch (ex if ex.becauseNoSuchFile) {
      
    }
  }
}


add_task(test_copymove);

add_task(test_copymove.bind(null, {unixUserland: true}));

add_task(do_test_finished);
