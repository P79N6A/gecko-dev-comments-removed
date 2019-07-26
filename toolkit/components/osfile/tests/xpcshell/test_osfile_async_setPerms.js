


"use strict";
















function format_mode(mode) {
  if (mode <= 0o777) {
    return ("0000" + mode.toString(8)).slice(-4);
  } else {
    return "0" + mode.toString(8);
  }
}





function do_check_modes_eq(left, right, text) {
  text = text + ": " + format_mode(left) + " === " + format_mode(right);
  do_report_result(left === right, text, Components.stack.caller, false);
}

const _umask = OS.Constants.Sys.umask;
do_print("umask: " + format_mode(_umask));





function apply_umask(mode) {
  return mode & ~_umask;
}


add_task(function*() {
  let path = OS.Path.join(OS.Constants.Path.tmpDir,
                          "test_osfile_async_setPerms_nonproto.tmp");
  yield OS.File.writeAtomic(path, new Uint8Array(1));

  try {
    let stat;

    yield OS.File.setPermissions(path, {unixMode: 0o4777});
    stat = yield OS.File.stat(path);
    do_check_modes_eq(stat.unixMode, 0o4777,
                      "setPermissions(path, 04777)");

    yield OS.File.setPermissions(path, {unixMode: 0o4777,
                                        unixHonorUmask: true});
    stat = yield OS.File.stat(path);
    do_check_modes_eq(stat.unixMode, apply_umask(0o4777),
                      "setPermissions(path, 04777&~umask)");

    yield OS.File.setPermissions(path);
    stat = yield OS.File.stat(path);
    do_check_modes_eq(stat.unixMode, apply_umask(0o666),
                      "setPermissions(path, {})");

    yield OS.File.setPermissions(path, {unixMode: 0});
    stat = yield OS.File.stat(path);
    do_check_modes_eq(stat.unixMode, 0,
                      "setPermissions(path, 0000)");

  } finally {
    yield OS.File.remove(path);
  }
});


add_task(function*() {
  
  let path = OS.Path.join(OS.Constants.Path.tmpDir,
                              "test_osfile_async_setDates_proto.tmp");
  yield OS.File.writeAtomic(path, new Uint8Array(1));

  try {
    let fd = yield OS.File.open(path, {write: true});
    let stat;

    yield fd.setPermissions({unixMode: 0o4777});
    stat = yield fd.stat();
    do_check_modes_eq(stat.unixMode, 0o4777,
                      "fd.setPermissions(04777)");

    yield fd.setPermissions({unixMode: 0o4777, unixHonorUmask: true});
    stat = yield fd.stat();
    do_check_modes_eq(stat.unixMode, apply_umask(0o4777),
                      "fd.setPermissions(04777&~umask)");

    yield fd.setPermissions();
    stat = yield fd.stat();
    do_check_modes_eq(stat.unixMode, apply_umask(0o666),
                      "fd.setPermissions({})");

    yield fd.setPermissions({unixMode: 0});
    stat = yield fd.stat();
    do_check_modes_eq(stat.unixMode, 0,
                      "fd.setPermissions(0000)");

    yield fd.close();
  } finally {
    yield OS.File.remove(path);
  }
});

function run_test() {
  run_next_test();
}
