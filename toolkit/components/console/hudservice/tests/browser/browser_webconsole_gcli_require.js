





Components.utils.import("resource:///modules/gcli.jsm");

var define, require, console;

function test() {

  define = gcli._internal.define;
  require = gcli._internal.require;
  console = gcli._internal.console;

  define('gclitest/requirable', [], function(require, exports, module) {
    exports.thing1 = 'thing1';
    exports.thing2 = 2;

    let status = 'initial';
    exports.setStatus = function(aStatus) { status = aStatus; };
    exports.getStatus = function() { return status; };
  });

  define('gclitest/unrequirable', [], function(require, exports, module) {
    null.throwNPE();
  });

  define('gclitest/recurse', [], function(require, exports, module) {
    require('gclitest/recurse');
  });

  testWorking();
  testLeakage();
  testMultiImport();
  testRecursive();
  testUncompilable();

  finishTest();

  delete define;
  delete require;
  delete console;
}

function testWorking() {
  
  
  
  let requireable = require('gclitest/requirable');
  ok('thing1' == requireable.thing1, 'thing1 was required');
  ok(2 == requireable.thing2, 'thing2 was required');
  ok(requireable.thing3 === undefined, 'thing3 was not required');
}

function testDomains() {
  let requireable = require('gclitest/requirable');
  ok(requireable.status === undefined, 'requirable has no status');
  requireable.setStatus(null);
  ok(null === requireable.getStatus(), 'requirable.getStatus changed to null');
  ok(requireable.status === undefined, 'requirable still has no status');
  requireable.setStatus('42');
  ok('42' == requireable.getStatus(), 'requirable.getStatus changed to 42');
  ok(requireable.status === undefined, 'requirable *still* has no status');

  let domain = new define.Domain();
  let requireable2 = domain.require('gclitest/requirable');
  ok(requireable2.status === undefined, 'requirable2 has no status');
  ok('initial' === requireable2.getStatus(), 'requirable2.getStatus is initial');
  requireable2.setStatus(999);
  ok(999 === requireable2.getStatus(), 'requirable2.getStatus changed to 999');
  ok(requireable2.status === undefined, 'requirable2 still has no status');

  t.verifyEqual('42', requireable.getStatus());
  ok(requireable.status === undefined, 'requirable has no status (as expected)');
}

function testLeakage() {
  let requireable = require('gclitest/requirable');
  ok(requireable.setup == null, 'leakage of setup');
  ok(requireable.shutdown == null, 'leakage of shutdown');
  ok(requireable.testWorking == null, 'leakage of testWorking');
}

function testMultiImport() {
  let r1 = require('gclitest/requirable');
  let r2 = require('gclitest/requirable');
  ok(r1 === r2, 'double require was strict equal');
}

function testUncompilable() {
  
  
  
  try {
      let unrequireable = require('gclitest/unrequirable');
      fail();
  }
  catch (ex) {
      console.log(ex);
  }
}

function testRecursive() {
  
  
  
}
