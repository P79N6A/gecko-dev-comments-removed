



"use strict";






const { exec } = require("sdk/system/child_process");
const { platform, pathFor } = require("sdk/system");
const PROFILE_DIR = pathFor("ProfD");
const isWindows = platform.toLowerCase().indexOf("win") === 0;
const app = require("sdk/system/xul-app");



if (app.is("Firefox")) {
  exports["test child_process in an addon"] = (assert, done) => {
    exec(isWindows ? "DIR /A-D" : "ls -al", {
      cwd: PROFILE_DIR
    }, (err, stdout, stderr) => {
      assert.ok(!err, "no errors");
      assert.equal(stderr, "", "stderr is empty");
      assert.ok(/extensions\.ini/.test(stdout), "stdout output of `ls -al` finds files");

      if (isWindows)
        assert.ok(!/<DIR>/.test(stdout), "passing args works");
      else
        assert.ok(/d(r[-|w][-|x]){3}/.test(stdout), "passing args works");
      done();
    });
  };
} else {
  exports["test unsupported"] = (assert) => assert.pass("This application is unsupported.");
}
require("sdk/test/runner").runTestsFromModule(module);
