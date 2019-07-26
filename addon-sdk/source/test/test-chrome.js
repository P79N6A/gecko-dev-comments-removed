




'use strict';

let chrome = require('chrome');

const FIXTURES_URL = module.uri.substr(0, module.uri.lastIndexOf('/') + 1) +
                     'fixtures/chrome-worker/'

exports['test addEventListener'] = function(assert, done) {
  let uri = FIXTURES_URL + 'addEventListener.js';

  let worker = new chrome.ChromeWorker(uri);
  worker.addEventListener('message', function(event) {
    assert.equal(event.data, 'Hello', 'message received');
    worker.terminate();
    done();
  });
};

exports['test onmessage'] = function(assert, done) {
  let uri = FIXTURES_URL + 'onmessage.js';

  let worker = new chrome.ChromeWorker(uri);
  worker.onmessage = function(event) {
    assert.equal(event.data, 'ok', 'message received');
    worker.terminate();
    done();
  };
  worker.postMessage('ok');
};

exports['test setTimeout'] = function(assert, done) {
  let uri = FIXTURES_URL + 'setTimeout.js';

  let worker = new chrome.ChromeWorker(uri);
  worker.onmessage = function(event) {
    assert.equal(event.data, 'ok', 'setTimeout fired');
    worker.terminate();
    done();
  };
};

exports['test jsctypes'] = function(assert, done) {
  let uri = FIXTURES_URL + 'jsctypes.js';

  let worker = new chrome.ChromeWorker(uri);
  worker.onmessage = function(event) {
    assert.equal(event.data, 'function', 'ctypes.open is a function');
    worker.terminate();
    done();
  };
};

exports['test XMLHttpRequest'] = function(assert, done) {
  let uri = FIXTURES_URL + 'xhr.js';

  let worker = new chrome.ChromeWorker(uri);
  worker.onmessage = function(event) {
    assert.equal(event.data, 'ok', 'XMLHttpRequest works');
    worker.terminate();
    done();
  };
};

exports['test onerror'] = function(assert, done) {
  let uri = FIXTURES_URL + 'onerror.js';

  let worker = new chrome.ChromeWorker(uri);
  worker.onerror = function(event) {
    assert.equal(event.filename, uri, 'event reports the correct uri');
    assert.equal(event.lineno, 8, 'event reports the correct line number');
    assert.equal(event.target, worker, 'event reports the correct worker');
    assert.ok(event.message.match(/ok/),
                'event contains the exception message');
    
    event.preventDefault();
    worker.terminate();
    done();
  };
};

require('test').run(exports);
