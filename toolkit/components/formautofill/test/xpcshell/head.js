








"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);



let Output = {
  print: do_print,
};

let executeSoon = do_execute_soon;
let setTimeout = (fn, delay) => do_timeout(delay, fn);

function run_test() {
  do_get_profile();
  run_next_test();
}
