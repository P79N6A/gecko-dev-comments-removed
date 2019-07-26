



Object.defineProperty(this, "global", { value: this });

exports.testGlobals = function(test) {
  
  
  test.assertObject(module, "have 'module', good");
  test.assertObject(exports, "have 'exports', good");
  test.assertFunction(require, "have 'require', good");
  test.assertFunction(dump, "have 'dump', good");
  test.assertObject(console, "have 'console', good");

  
  test.assert(!('packaging' in global), "no 'packaging', good");
  test.assert(!('memory' in global), "no 'memory', good");

  test.assertMatches(module.uri, /test-globals\.js$/,
                     'should contain filename');
};
