






Components.utils.import('resource://test/Utils.jsm');






define("test/source-map/test-base64", ["require", "exports", "module"], function (require, exports, module) {

  var base64 = require('source-map/base64');

  exports['test out of range encoding'] = function (assert, util) {
    assert.throws(function () {
      base64.encode(-1);
    });
    assert.throws(function () {
      base64.encode(64);
    });
  };

  exports['test out of range decoding'] = function (assert, util) {
    assert.throws(function () {
      base64.decode('=');
    });
  };

  exports['test normal encoding and decoding'] = function (assert, util) {
    for (var i = 0; i < 64; i++) {
      assert.equal(base64.decode(base64.encode(i)), i);
    }
  };

});
function run_test() {
  runSourceMapTests('test/source-map/test-base64', do_throw);
}
