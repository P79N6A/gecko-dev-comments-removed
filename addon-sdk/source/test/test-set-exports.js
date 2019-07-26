



let four = require("./modules/exportsEquals");
exports.testExportsEquals = function(assert) {
  assert.equal(four, 4);
};



















exports.testModule = function(assert) {
  
  
  
  
  var found = /test-set-exports$/.test(module.id);
  assert.equal(found, true, module.id+" ends with test-set-exports.js");
};

require('sdk/test').run(exports);
