



let four = require("./modules/exportsEquals");
exports.testExportsEquals = function(test) {
  test.assertEqual(four, 4);
}



















exports.testModule = function(test) {
  
  
  
  
  var found = /test-set-exports$/.test(module.id);
  test.assertEqual(found, true, module.id+" ends with test-set-exports.js");
}
