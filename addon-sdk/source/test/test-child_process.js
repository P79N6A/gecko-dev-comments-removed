



'use strict';

const { spawn, exec, execFile, fork } = require('sdk/system/child_process');
const { env, platform, pathFor } = require('sdk/system');
const { isNumber } = require('sdk/lang/type');
const { after } = require('sdk/test/utils');
const { emit } = require('sdk/event/core');
const PROFILE_DIR= pathFor('ProfD');
const isWindows = platform.toLowerCase().indexOf('win') === 0;
const { getScript, cleanUp } = require('./fixtures/child-process-scripts');



const CAT_PATH = isWindows ? 'C:\\Windows\\System32\\more.com' : '/bin/cat';

exports.testExecCallbackSuccess = function (assert, done) {
  exec(isWindows ? 'DIR /A-D' : 'ls -al', {
    cwd: PROFILE_DIR
  }, function (err, stdout, stderr) {
    assert.ok(!err, 'no errors found');
    assert.equal(stderr, '', 'stderr is empty');
    assert.ok(/extensions\.ini/.test(stdout), 'stdout output of `ls -al` finds files');

    if (isWindows) {
      
      assert.ok(!/<DIR>/.test(stdout),
        'passing arguments in `exec` works');
    }
    else {
      
      assert.ok(/d(r[-|w][-|x]){3}/.test(stdout),
        'passing arguments in `exec` works');
    }
    done();
  });
};

exports.testExecCallbackError = function (assert, done) {
  exec('not-real-command', { cwd: PROFILE_DIR }, function (err, stdout, stderr) {
    assert.ok(/not-real-command/.test(err.toString()),
      'error contains error message');
    assert.ok(err.lineNumber >= 0, 'error contains lineNumber');
    assert.ok(/resource:\/\//.test(err.fileName), 'error contains fileName');
    assert.ok(err.code && isNumber(err.code), 'non-zero error code property on error');
    assert.equal(err.signal, null,
      'null signal property when not manually terminated');
    assert.equal(stdout, '', 'stdout is empty');
    assert.ok(/not-real-command/.test(stderr), 'stderr contains error message');
    done();
  });
};

exports.testExecOptionsEnvironment = function (assert, done) {
  getScript('check-env').then(envScript => {
    exec(envScript, {
      cwd: PROFILE_DIR,
      env: { CHILD_PROCESS_ENV_TEST: 'my-value-test' }
    }, function (err, stdout, stderr) {
      assert.equal(stderr, '', 'stderr is empty');
      assert.ok(!err, 'received `cwd` option');
      assert.ok(/my-value-test/.test(stdout),
        'receives environment option');
      done();
    });
  }).then(null, assert.fail);
};

exports.testExecOptionsTimeout = function (assert, done) {
  let count = 0;
  getScript('wait').then(script => {
    let child = exec(script, { timeout: 100 }, (err, stdout, stderr) => {
      assert.equal(err.killed, true, 'error has `killed` property as true');
      assert.equal(err.code, null, 'error has `code` as null');
      assert.equal(err.signal, 'SIGTERM',
        'error has `signal` as SIGTERM by default');
      assert.equal(stdout, '', 'stdout is empty');
      assert.equal(stderr, '', 'stderr is empty');
      if (++count === 3) complete();
    });

    function exitHandler (code, signal) {
      assert.equal(code, null, 'error has `code` as null');
      assert.equal(signal, 'SIGTERM',
        'error has `signal` as SIGTERM by default');
      if (++count === 3) complete();
    }

    function closeHandler (code, signal) {
      assert.equal(code, null, 'error has `code` as null');
      assert.equal(signal, 'SIGTERM',
        'error has `signal` as SIGTERM by default');
      if (++count === 3) complete();
    }

    child.on('exit', exitHandler);
    child.on('close', closeHandler);

    function complete () {
      child.off('exit', exitHandler);
      child.off('close', closeHandler);
      done();
    }
  }).then(null, assert.fail);
};

exports.testExecFileCallbackSuccess = function (assert, done) {
  getScript('args').then(script => {
    execFile(script, ['--myargs', '-j', '-s'], { cwd: PROFILE_DIR }, function (err, stdout, stderr) {
      assert.ok(!err, 'no errors found');
      assert.equal(stderr, '', 'stderr is empty');
      
      assert.equal(stdout.trim(), '--myargs -j -s'.trim(), 'passes in correct arguments');
      done();
    });
  }).then(null, assert.fail);
};

exports.testExecFileCallbackError = function (assert, done) {
  execFile('not-real-command', { cwd: PROFILE_DIR }, function (err, stdout, stderr) {
    assert.ok(/NS_ERROR_FILE_UNRECOGNIZED_PATH/.test(err.message),
      'error contains error message');
    assert.ok(err.lineNumber >= 0, 'error contains lineNumber');
    assert.ok(/resource:\/\//.test(err.fileName), 'error contains fileName');
    assert.equal(stdout, '', 'stdout is empty');
    assert.equal(stderr, '', 'stdout is empty');
    done();
  });
};

exports.testExecFileOptionsEnvironment = function (assert, done) {
  getScript('check-env').then(script => {
    execFile(script, {
      cwd: PROFILE_DIR,
      env: { CHILD_PROCESS_ENV_TEST: 'my-value-test' }
    }, function (err, stdout, stderr) {
      assert.equal(stderr, '', 'stderr is empty');
      assert.ok(!err, 'received `cwd` option');
      assert.ok(/my-value-test/.test(stdout),
        'receives environment option');
      done();
    });
  }).then(null, assert.fail);
};

exports.testExecFileOptionsTimeout = function (assert, done) {
  let count = 0;
  getScript('wait').then(script => {
    let child = execFile(script, { timeout: 100 }, (err, stdout, stderr) => {
      assert.equal(err.killed, true, 'error has `killed` property as true');
      assert.equal(err.code, null, 'error has `code` as null');
      assert.equal(err.signal, 'SIGTERM',
        'error has `signal` as SIGTERM by default');
      assert.equal(stdout, '', 'stdout is empty');
      assert.equal(stderr, '', 'stderr is empty');
      if (++count === 3) complete();
    });

    function exitHandler (code, signal) {
      assert.equal(code, null, 'error has `code` as null');
      assert.equal(signal, 'SIGTERM',
        'error has `signal` as SIGTERM by default');
      if (++count === 3) complete();
    }

    function closeHandler (code, signal) {
      assert.equal(code, null, 'error has `code` as null');
      assert.equal(signal, 'SIGTERM',
        'error has `signal` as SIGTERM by default');
      if (++count === 3) complete();
    }

    child.on('exit', exitHandler);
    child.on('close', closeHandler);

    function complete () {
      child.off('exit', exitHandler);
      child.off('close', closeHandler);
      done();
    }
  }).then(null, assert.fail);
};






exports.testExecFileOptionsMaxBufferLargeStdOut = function (assert, done) {
  let count = 0;
  let stdoutChild;

  
  getScript('large-out').then(script => {
    stdoutChild = execFile(script, ['10000'], { maxBuffer: 50 }, (err, stdout, stderr) => {
      assert.ok(/stdout maxBuffer exceeded/.test(err.toString()),
        'error contains stdout maxBuffer exceeded message');
      assert.ok(stdout.length >= 50, 'stdout has full buffer');
      assert.equal(stderr, '', 'stderr is empty');
      if (++count === 3) complete();
    });
    stdoutChild.on('exit', exitHandler);
    stdoutChild.on('close', closeHandler);
  }).then(null, assert.fail);

  function exitHandler (code, signal) {
    assert.equal(code, null, 'Exit code is null in exit handler');
    assert.equal(signal, 'SIGTERM', 'Signal is SIGTERM in exit handler');
    if (++count === 3) complete();
  }

  function closeHandler (code, signal) {
    assert.equal(code, null, 'Exit code is null in close handler');
    assert.equal(signal, 'SIGTERM', 'Signal is SIGTERM in close handler');
    if (++count === 3) complete();
  }

  function complete () {
    stdoutChild.off('exit', exitHandler);
    stdoutChild.off('close', closeHandler);
    done();
  }
};

exports.testExecFileOptionsMaxBufferLargeStdOErr = function (assert, done) {
  let count = 0;
  let stderrChild;
  
  getScript('large-err').then(script => {
    stderrChild = execFile(script, ['10000'], { maxBuffer: 50 }, (err, stdout, stderr) => {
      assert.ok(/stderr maxBuffer exceeded/.test(err.toString()),
        'error contains stderr maxBuffer exceeded message');
      assert.ok(stderr.length >= 50, 'stderr has full buffer');
      assert.equal(stdout, '', 'stdout is empty');
      if (++count === 3) complete();
    });
    stderrChild.on('exit', exitHandler);
    stderrChild.on('close', closeHandler);
  }).then(null, assert.fail);

  function exitHandler (code, signal) {
    assert.equal(code, null, 'Exit code is null in exit handler');
    assert.equal(signal, 'SIGTERM', 'Signal is SIGTERM in exit handler');
    if (++count === 3) complete();
  }

  function closeHandler (code, signal) {
    assert.equal(code, null, 'Exit code is null in close handler');
    assert.equal(signal, 'SIGTERM', 'Signal is SIGTERM in close handler');
    if (++count === 3) complete();
  }

  function complete () {
    stderrChild.off('exit', exitHandler);
    stderrChild.off('close', closeHandler);
    done();
  }
};






exports.testExecFileOptionsMaxBufferSmallStdOut = function (assert, done) {
  let count = 0;
  let stdoutChild;

  
  getScript('large-out').then(script => {
    stdoutChild = execFile(script, ['60'], { maxBuffer: 50 }, (err, stdout, stderr) => {
      assert.ok(/stdout maxBuffer exceeded/.test(err.toString()),
        'error contains stdout maxBuffer exceeded message');
      assert.ok(stdout.length >= 50, 'stdout has full buffer');
      assert.equal(stderr, '', 'stderr is empty');
      if (++count === 3) complete();
    });
    stdoutChild.on('exit', exitHandler);
    stdoutChild.on('close', closeHandler);
  }).then(null, assert.fail);

  function exitHandler (code, signal) {
    
    
    if (code === null) {
      assert.equal(code, null, 'Exit code is null in exit handler');
      assert.equal(signal, 'SIGTERM', 'Signal is SIGTERM in exit handler');
    }
    else {
      assert.equal(code, 0, 'Exit code is 0 in exit handler');
      assert.equal(signal, null, 'Signal is null in exit handler');
    }
    if (++count === 3) complete();
  }

  function closeHandler (code, signal) {
    
    
    if (code === null) {
      assert.equal(code, null, 'Exit code is null in close handler');
      assert.equal(signal, 'SIGTERM', 'Signal is SIGTERM in close handler');
    }
    else {
      assert.equal(code, 0, 'Exit code is 0 in close handler');
      assert.equal(signal, null, 'Signal is null in close handler');
    }
    if (++count === 3) complete();
  }

  function complete () {
    stdoutChild.off('exit', exitHandler);
    stdoutChild.off('close', closeHandler);
    done();
  }
};

exports.testExecFileOptionsMaxBufferSmallStdErr = function (assert, done) {
  let count = 0;
  let stderrChild;
  
  getScript('large-err').then(script => {
    stderrChild = execFile(script, ['60'], { maxBuffer: 50 }, (err, stdout, stderr) => {
      assert.ok(/stderr maxBuffer exceeded/.test(err.toString()),
        'error contains stderr maxBuffer exceeded message');
      assert.ok(stderr.length >= 50, 'stderr has full buffer');
      assert.equal(stdout, '', 'stdout is empty');
      if (++count === 3) complete();
    });
    stderrChild.on('exit', exitHandler);
    stderrChild.on('close', closeHandler);
  }).then(null, assert.fail);

  function exitHandler (code, signal) {
    
    
    if (code === null) {
      assert.equal(code, null, 'Exit code is null in exit handler');
      assert.equal(signal, 'SIGTERM', 'Signal is SIGTERM in exit handler');
    }
    else {
      assert.equal(code, 0, 'Exit code is 0 in exit handler');
      assert.equal(signal, null, 'Signal is null in exit handler');
    }
    if (++count === 3) complete();
  }

  function closeHandler (code, signal) {
    
    
    if (code === null) {
      assert.equal(code, null, 'Exit code is null in close handler');
      assert.equal(signal, 'SIGTERM', 'Signal is SIGTERM in close handler');
    }
    else {
      assert.equal(code, 0, 'Exit code is 0 in close handler');
      assert.equal(signal, null, 'Signal is null in close handler');
    }
    if (++count === 3) complete();
  }

  function complete () {
    stderrChild.off('exit', exitHandler);
    stderrChild.off('close', closeHandler);
    done();
  }
};

exports.testChildExecFileKillSignal = function (assert, done) {
  getScript('wait').then(script => {
    execFile(script, {
      killSignal: 'beepbeep',
      timeout: 10
    }, function (err, stdout, stderr) {
      assert.equal(err.signal, 'beepbeep', 'correctly used custom killSignal');
      done();
    });
  }).then(null, assert.fail);
};

exports.testChildProperties = function (assert, done) {
  getScript('check-env').then(script => {
    let child = spawn(script, {
      env: { CHILD_PROCESS_ENV_TEST: 'my-value-test' }
    });

    if (isWindows)
      assert.ok(true, 'Windows environment does not have `pid`');
    else
      assert.ok(child.pid > 0, 'Child has a pid');
  }).then(done, assert.fail);
};

exports.testChildStdinStreamLarge = function (assert, done) {
  let REPEAT = 2000;
  let allData = '';
  
  
  let child = spawn(CAT_PATH);

  child.stdout.on('data', onData);
  child.on('close', onClose);

  for (let i = 0; i < REPEAT; i++)
    emit(child.stdin, 'data', '12345\n');

  emit(child.stdin, 'end');

  function onData (data) {
    allData += data;
  }

  function onClose (code, signal) {
    child.stdout.off('data', onData);
    child.off('close', onClose);
    assert.equal(code, 0, 'exited succesfully');
    assert.equal(signal, null, 'no kill signal given');
    assert.equal(allData.replace(/\W/g, '').length, '12345'.length * REPEAT,
      'all data processed from stdin');
    done();
  }
};

exports.testChildStdinStreamSmall = function (assert, done) {
  let allData = '';
  let child = spawn(CAT_PATH);
  child.stdout.on('data', onData);
  child.on('close', onClose);

  emit(child.stdin, 'data', '12345');
  emit(child.stdin, 'end');

  function onData (data) {
    allData += data;
  }

  function onClose (code, signal) {
    child.stdout.off('data', onData);
    child.off('close', onClose);
    assert.equal(code, 0, 'exited succesfully');
    assert.equal(signal, null, 'no kill signal given');
    assert.equal(allData.trim(), '12345', 'all data processed from stdin');
    done();
  }
};




exports.testChildEventsSpawningError= function (assert, done) {
  let handlersCalled = 0;
  let child = execFile('i-do-not-exist', (err, stdout, stderr) => {
    assert.ok(err, 'error was passed into callback');
    assert.equal(stdout, '', 'stdout is empty')
    assert.equal(stderr, '', 'stderr is empty');
    if (++handlersCalled === 3) complete();
  });

  child.on('error', handleError);
  child.on('exit', handleExit);
  child.on('close', handleClose);

  function handleError (e) {
    assert.ok(e, 'error passed into error handler');
    if (++handlersCalled === 3) complete();
  }

  function handleClose (code, signal) {
    assert.equal(code, -1,
      'process was never spawned, therefore exit code is -1');
    assert.equal(signal, null, 'signal should be null');
    if (++handlersCalled === 3) complete();
  }

  function handleExit (code, signal) {
    assert.fail('Close event should not be called on init failure');
  }

  function complete () {
    child.off('error', handleError);
    child.off('exit', handleExit);
    child.off('close', handleClose);
    done();
  }
};

exports.testSpawnOptions = function (assert, done) {
  let count = 0;
  let envStdout = '';
  let cwdStdout = '';
  let checkEnv, checkPwd, envChild, cwdChild;
  getScript('check-env').then(script => {
    checkEnv = script;
    return getScript('check-pwd');
  }).then(script => {
    checkPwd = script;

    envChild = spawn(checkEnv, {
      env: { CHILD_PROCESS_ENV_TEST: 'my-value-test' }
    });
    cwdChild = spawn(checkPwd, { cwd: PROFILE_DIR });

    
    envChild.stdout.on('data', data => envStdout += data);
    cwdChild.stdout.on('data', data => cwdStdout += data);

    envChild.on('close', envClose);
    cwdChild.on('close', cwdClose);
  }).then(null, assert.fail);

  function envClose () {
    assert.equal(envStdout.trim(), 'my-value-test', 'spawn correctly passed in ENV');
    if (++count === 2) complete();
  }

  function cwdClose () {
    
    
    
    let isCorrectPath = ~cwdStdout.trim().indexOf(PROFILE_DIR);
    assert.ok(isCorrectPath, 'spawn correctly passed in cwd');
    if (++count === 2) complete();
  }

  function complete () {
    envChild.off('close', envClose);
    cwdChild.off('close', cwdClose);
    done();
  }
};

exports.testFork = function (assert) {
  assert.throws(function () {
    fork();
  }, /not currently supported/, 'fork() correctly throws an unsupported error');
};

after(exports, cleanUp);

require("sdk/test").run(exports);


module.exports = {};
