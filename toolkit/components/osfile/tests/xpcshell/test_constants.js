


"use strict";

Components.utils.import("resource://gre/modules/Promise.jsm", this);
Components.utils.import("resource://gre/modules/osfile.jsm");
Components.utils.import("resource://gre/modules/Services.jsm", this);
let isDebugBuild = Components.classes["@mozilla.org/xpcom/debug;1"].getService(Components.interfaces.nsIDebug2).isDebugBuild;

let worker_url = "chrome://workers/content/worker_test_constants.js";
do_load_manifest("data/chrome.manifest");

function run_test() {
  run_next_test();
}


add_task(function* check_definition() {
  do_check_true(OS.Constants!=null);
  do_check_true(!!OS.Constants.Win || !!OS.Constants.libc);
  do_check_true(OS.Constants.Path!=null);
  do_check_true(OS.Constants.Sys!=null);
  
  do_check_eq(OS.Constants.Sys.Name, Services.appinfo.OS);
  
  if (isDebugBuild) {
    do_check_true(OS.Constants.Sys.DEBUG);
  } else {
    do_check_true(typeof(OS.Constants.Sys.DEBUG) == 'undefined');
  }
});

function test_worker(worker) {
  var deferred = Promise.defer();
  worker.onmessage = function (e) {
    
    var data = JSON.parse(e.data);
    try {
      
      do_check_true(data.OS_Constants != null);
      do_check_true(!!data.OS_Constants_Win || !!data.OS_Constants_libc);
      do_check_true(data.OS_Constants_Path != null);
      do_check_true(data.OS_Constants_Sys != null);
      do_check_eq(data.OS_Constants_Sys_Name, Services.appinfo.OS);
      if (isDebugBuild) {
        do_check_true(data.OS_Constants_Sys_DEBUG);
      } else {
        do_check_true(typeof(data.OS_Constants_Sys_DEBUG) == 'undefined');
      }
    }  finally {
       
       deferred.resolve();
    }
  };
  
  worker.postMessage('Start tests');
  return deferred.promise;
}

add_task(function* check_worker() {
  
  return test_worker(new ChromeWorker(worker_url));
});