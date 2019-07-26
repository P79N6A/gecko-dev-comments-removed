


"use strict";

function run_test() {
  let uuid = CommonUtils.generateUUID();
  do_check_eq(uuid.length, 36);
  do_check_eq(uuid[8], "-");

  run_next_test();
}
