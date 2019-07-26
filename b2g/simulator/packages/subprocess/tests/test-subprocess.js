




const { Cc, Ci } = require("chrome");
const subprocess = require("subprocess");
const env = Cc["@mozilla.org/process/environment;1"].getService(Ci.nsIEnvironment);


if (env.get('OS') && env.get('OS').match(/Windows/)) {

exports.testWindows = function (test) {
  test.waitUntilDone();
  let envTestValue = "OK";
  let gotStdout = false;

  var p = subprocess.call({
    
    command:     env.get('ComSpec'),
    
    arguments:   ['/C', 'echo %ENV_TEST%'], 
    
    environment: ['ENV_TEST='+envTestValue],

    stdin: function(stdin) {
      
      
      stdin.write("stdin");
      stdin.close();
    },
    stdout: function(data) {
      test.assert(!gotStdout,"don't get stdout twice");
      test.assertEqual(data,envTestValue+"\r\n","stdout contains the environment variable");
      gotStdout = true;
    },
    stderr: function(data) {
      test.fail("shouldn't get stderr");
    },
    done: function() {
      test.assert(gotStdout, "got stdout before finished");
      test.done();
    },
    mergeStderr: false
  });

}

exports.testWindowsStderr = function (test) {
  test.waitUntilDone();
  let gotStderr = false;

  var p = subprocess.call({
    command:     env.get('ComSpec'),
    arguments:   ['/C', 'nonexistent'],

    stdout: function(data) {
      test.fail("shouldn't get stdout");
    },
    stderr: function(data) {
      test.assert(!gotStderr,"don't get stderr twice");
      test.assertEqual(
        data,
        "'nonexistent' is not recognized as an internal or external command,\r\n" +
        "operable program or batch file.\r\n",
        "stderr contains the error message"
      );
      gotStderr = true;
    },
    done: function() {
      test.assert(gotStderr, "got stderr before finished");
      test.done();
    },
    mergeStderr: false
  });

}

}

if (env.get('USER') && env.get('SHELL')) {

exports.testUnix = function (test) {
  test.waitUntilDone();
  let envTestValue = "OK";
  let gotStdout = false;

  var p = subprocess.call({
    command:     '/bin/sh',
    
    
    environment: ['ENV_TEST='+envTestValue],

    stdin: function(stdin) {
      stdin.write("echo $ENV_TEST");
      stdin.close();
    },
    stdout: function(data) {
      test.assert(!gotStdout,"don't get stdout twice");
      test.assertEqual(data,envTestValue+"\n","stdout contains the environment variable");
      gotStdout = true;
    },
    stderr: function(data) {
      test.fail("shouldn't get stderr");
    },
    done: function() {
      test.assert(gotStdout, "got stdout before finished");
      test.done();
    },
    mergeStderr: false
  });
}

exports.testUnixStderr = function (test) {
  test.waitUntilDone();
  let gotStderr = false;

  var p = subprocess.call({
    
    command:     '/bin/sh',
    arguments:   ['nonexistent'],

    stdout: function(data) {
      test.fail("shouldn't get stdout");
    },
    stderr: function(data) {
      test.assert(!gotStderr,"don't get stderr twice");
      
      if (data == "/bin/sh: 0: Can't open nonexistent\n")
        test.pass("stderr containes the expected error message");
      else
        test.assertEqual(data, "/bin/sh: nonexistent: No such file or directory\n",
                         "stderr contains the error message");
      gotStderr = true;
    },
    done: function() {
      test.assert(gotStderr, "got stderr before finished");
      test.done();
    },
    mergeStderr: false
  });
}

}
