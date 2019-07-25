













Components.utils.import('resource:///modules/devtools/Require.jsm');
Components.utils.import('resource:///modules/devtools/SourceMap.jsm');

let EXPORTED_SYMBOLS = [ "define", "runSourceMapTests" ];






define('test/source-map/assert', ['exports'], function (exports) {

  let do_throw = function (msg) {
    throw new Error(msg);
  };

  exports.init = function (throw_fn) {
    do_throw = throw_fn;
  };

  exports.doesNotThrow = function (fn) {
    try {
      fn();
    }
    catch (e) {
      do_throw(e.message);
    }
  };

  exports.equal = function (actual, expected, msg) {
    msg = msg || String(actual) + ' != ' + String(expected);
    if (actual != expected) {
      do_throw(msg);
    }
  };

  exports.ok = function (val, msg) {
    msg = msg || String(val) + ' is falsey';
    if (!Boolean(val)) {
      do_throw(msg);
    }
  };

  exports.strictEqual = function (actual, expected, msg) {
    msg = msg || String(actual) + ' !== ' + String(expected);
    if (actual !== expected) {
      do_throw(msg);
    }
  };

  exports.throws = function (fn) {
    try {
      fn();
      do_throw('Expected an error to be thrown, but it wasn\'t.');
    }
    catch (e) {
    }
  };

});






define('test/source-map/util', ['require', 'exports', 'module' ], function(require, exports, module) {

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  exports.testMap = {
    version: 3,
    file: 'min.js',
    names: ['bar', 'baz', 'n'],
    sources: ['one.js', 'two.js'],
    sourceRoot: '/the/root',
    mappings: 'CAAC,IAAI,IAAM,SAAUA,GAClB,OAAOC,IAAID;CCDb,IAAI,IAAM,SAAUE,GAClB,OAAOA'
  };

  function assertMapping(generatedLine, generatedColumn, originalSource,
                         originalLine, originalColumn, name, map, assert) {
    var mapping = map.originalPositionFor({
      line: generatedLine,
      column: generatedColumn
    });
    assert.equal(mapping.name, name,
                 'Incorrect name, expected ' + JSON.stringify(name)
                 + ', got ' + JSON.stringify(mapping.name));
    assert.equal(mapping.line, originalLine,
                 'Incorrect line, expected ' + JSON.stringify(originalLine)
                 + ', got ' + JSON.stringify(mapping.line));
    assert.equal(mapping.column, originalColumn,
                 'Incorrect column, expected ' + JSON.stringify(originalColumn)
                 + ', got ' + JSON.stringify(mapping.column));
    assert.equal(mapping.source, originalSource,
                 'Incorrect source, expected ' + JSON.stringify(originalSource)
                 + ', got ' + JSON.stringify(mapping.source));
  }
  exports.assertMapping = assertMapping;

});






function runSourceMapTests(modName, do_throw) {
  let mod = require(modName);
  let assert = require('test/source-map/assert');
  let util = require('test/source-map/util');

  assert.init(do_throw);

  for (let k in mod) {
    if (/^test/.test(k)) {
      mod[k](assert, util);
    }
  }

}
