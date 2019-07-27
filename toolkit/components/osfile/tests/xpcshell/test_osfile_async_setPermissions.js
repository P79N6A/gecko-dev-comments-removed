


"use strict";
















function format_mode(mode) {
  if (mode <= 0o777) {
    return ("0000" + mode.toString(8)).slice(-4);
  } else {
    return "0" + mode.toString(8);
  }
}

const _umask = OS.Constants.Sys.umask;
do_print("umask: " + format_mode(_umask));





function apply_umask(mode) {
  return mode & ~_umask;
}



let testSequence = [
  [null,                                        apply_umask(0o600)],
  [{ unixMode: 0o4777 },                        apply_umask(0o4777)],
  [{ unixMode: 0o4777, unixHonorUmask: false }, 0o4777],
  [{ unixMode: 0o4777, unixHonorUmask: true },  apply_umask(0o4777)],
  [undefined,                                   apply_umask(0o600)],
  [{ unixMode: 0o666 },                         apply_umask(0o666)],
  [{ unixMode: 0o600 },                         apply_umask(0o600)],
  [{ unixMode: 0 },                             0],
  [{},                                          apply_umask(0o600)],
];


add_task(function* test_path_setPermissions() {
  let path = OS.Path.join(OS.Constants.Path.tmpDir,
                          "test_osfile_async_setPermissions_path.tmp");
  yield OS.File.writeAtomic(path, new Uint8Array(1));

  try {
    for (let [options, expectedMode] of testSequence) {
      if (options !== null) {
        do_print("Setting permissions to " + JSON.stringify(options));
        yield OS.File.setPermissions(path, options);
      }

      let stat = yield OS.File.stat(path);
      do_check_eq(format_mode(stat.unixMode), format_mode(expectedMode));
    }
  } finally {
    yield OS.File.remove(path);
  }
});


add_task(function* test_file_setPermissions() {
  let path = OS.Path.join(OS.Constants.Path.tmpDir,
                              "test_osfile_async_setPermissions_file.tmp");
  yield OS.File.writeAtomic(path, new Uint8Array(1));

  try {
    let fd = yield OS.File.open(path, { write: true });
    try {
      for (let [options, expectedMode] of testSequence) {
        if (options !== null) {
          do_print("Setting permissions to " + JSON.stringify(options));
          yield fd.setPermissions(options);
        }

        let stat = yield fd.stat();
        do_check_eq(format_mode(stat.unixMode), format_mode(expectedMode));
      }
    } finally {
      yield fd.close();
    }
  } finally {
    yield OS.File.remove(path);
  }
});

function run_test() {
  run_next_test();
}
