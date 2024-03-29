






Components.utils.import('resource://test/Utils.jsm');






define("test/source-map/test-base64-vlq", ["require", "exports", "module"], function (require, exports, module) {

  var base64VLQ = require('source-map/base64-vlq');

  exports['test normal encoding and decoding'] = function (assert, util) {
    var result = {};
    for (var i = -255; i < 256; i++) {
      var str = base64VLQ.encode(i);
      base64VLQ.decode(str, 0, result);
      assert.equal(result.value, i);
      assert.equal(result.rest, str.length);
    }
  };

});
function run_test() {
  runSourceMapTests('test/source-map/test-base64-vlq', do_throw);
}
