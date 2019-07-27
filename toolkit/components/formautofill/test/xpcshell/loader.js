











"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);

Services.scriptloader.loadSubScript(
  Services.io.newFileURI(do_get_file("loader_common.js")).spec, this);


let Output = {
  print: do_print,
};

let executeSoon = do_execute_soon;
let setTimeout = (fn, delay) => do_timeout(delay, fn);


let add_task_in_parent_process = add_task;
let add_task_in_child_process = function () {};
let add_task_in_both_processes = add_task;

Services.scriptloader.loadSubScript(
  Services.io.newFileURI(do_get_file("head_common.js")).spec, this);


function run_test() {
  do_get_profile();
  run_next_test();
}



