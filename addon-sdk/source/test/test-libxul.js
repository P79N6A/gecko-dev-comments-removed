





const {Cu} = require("chrome");
const {ctypes} = Cu.import("resource://gre/modules/ctypes.jsm", {});
const {OS} = Cu.import("resource://gre/modules/osfile.jsm", {});

exports.test = function(assert) {
  let path = OS.Constants.Path.libxul;
  assert.pass("libxul is at " + path);
  let lib = ctypes.open(path);
  assert.ok(lib != null, "linked to libxul successfully");
};

require('sdk/test').run(exports);
