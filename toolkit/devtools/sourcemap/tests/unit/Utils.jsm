













Components.utils.import('resource://gre/modules/devtools/Require.jsm');
Components.utils.import('resource://gre/modules/devtools/SourceMap.jsm');

this.EXPORTED_SYMBOLS = [ "define", "runSourceMapTests" ];






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






define('test/source-map/util', ['require', 'exports', 'module' ,  'lib/source-map/util'], function(require, exports, module) {

  var util = require('source-map/util');

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  exports.testGeneratedCode = " ONE.foo=function(a){return baz(a);};\n"+
                              " TWO.inc=function(a){return a+1;};";
  exports.testMap = {
    version: 3,
    file: 'min.js',
    names: ['bar', 'baz', 'n'],
    sources: ['one.js', 'two.js'],
    sourceRoot: '/the/root',
    mappings: 'CAAC,IAAI,IAAM,SAAUA,GAClB,OAAOC,IAAID;CCDb,IAAI,IAAM,SAAUE,GAClB,OAAOA'
  };
  exports.testMapWithSourcesContent = {
    version: 3,
    file: 'min.js',
    names: ['bar', 'baz', 'n'],
    sources: ['one.js', 'two.js'],
    sourcesContent: [
      ' ONE.foo = function (bar) {\n' +
      '   return baz(bar);\n' +
      ' };',
      ' TWO.inc = function (n) {\n' +
      '   return n + 1;\n' +
      ' };'
    ],
    sourceRoot: '/the/root',
    mappings: 'CAAC,IAAI,IAAM,SAAUA,GAClB,OAAOC,IAAID;CCDb,IAAI,IAAM,SAAUE,GAClB,OAAOA'
  };

  function assertMapping(generatedLine, generatedColumn, originalSource,
                         originalLine, originalColumn, name, map, assert,
                         dontTestGenerated, dontTestOriginal) {
    if (!dontTestOriginal) {
      var origMapping = map.originalPositionFor({
        line: generatedLine,
        column: generatedColumn
      });
      assert.equal(origMapping.name, name,
                   'Incorrect name, expected ' + JSON.stringify(name)
                   + ', got ' + JSON.stringify(origMapping.name));
      assert.equal(origMapping.line, originalLine,
                   'Incorrect line, expected ' + JSON.stringify(originalLine)
                   + ', got ' + JSON.stringify(origMapping.line));
      assert.equal(origMapping.column, originalColumn,
                   'Incorrect column, expected ' + JSON.stringify(originalColumn)
                   + ', got ' + JSON.stringify(origMapping.column));
      assert.equal(origMapping.source,
                   originalSource ? util.join(map._sourceRoot, originalSource) : null,
                   'Incorrect source, expected ' + JSON.stringify(originalSource)
                   + ', got ' + JSON.stringify(origMapping.source));
    }

    if (!dontTestGenerated) {
      var genMapping = map.generatedPositionFor({
        source: originalSource,
        line: originalLine,
        column: originalColumn
      });
      assert.equal(genMapping.line, generatedLine,
                   'Incorrect line, expected ' + JSON.stringify(generatedLine)
                   + ', got ' + JSON.stringify(genMapping.line));
      assert.equal(genMapping.column, generatedColumn,
                   'Incorrect column, expected ' + JSON.stringify(generatedColumn)
                   + ', got ' + JSON.stringify(genMapping.column));
    }
  }
  exports.assertMapping = assertMapping;

  function assertEqualMaps(assert, actualMap, expectedMap) {
    assert.equal(actualMap.version, expectedMap.version, "version mismatch");
    assert.equal(actualMap.file, expectedMap.file, "file mismatch");
    assert.equal(actualMap.names.length,
                 expectedMap.names.length,
                 "names length mismatch: " +
                   actualMap.names.join(", ") + " != " + expectedMap.names.join(", "));
    for (var i = 0; i < actualMap.names.length; i++) {
      assert.equal(actualMap.names[i],
                   expectedMap.names[i],
                   "names[" + i + "] mismatch: " +
                     actualMap.names.join(", ") + " != " + expectedMap.names.join(", "));
    }
    assert.equal(actualMap.sources.length,
                 expectedMap.sources.length,
                 "sources length mismatch: " +
                   actualMap.sources.join(", ") + " != " + expectedMap.sources.join(", "));
    for (var i = 0; i < actualMap.sources.length; i++) {
      assert.equal(actualMap.sources[i],
                   expectedMap.sources[i],
                   "sources[" + i + "] length mismatch: " +
                   actualMap.sources.join(", ") + " != " + expectedMap.sources.join(", "));
    }
    assert.equal(actualMap.sourceRoot,
                 expectedMap.sourceRoot,
                 "sourceRoot mismatch: " +
                   actualMap.sourceRoot + " != " + expectedMap.sourceRoot);
    assert.equal(actualMap.mappings, expectedMap.mappings, "mappings mismatch");
    if (actualMap.sourcesContent) {
      assert.equal(actualMap.sourcesContent.length,
                   expectedMap.sourcesContent.length,
                   "sourcesContent length mismatch");
      for (var i = 0; i < actualMap.sourcesContent.length; i++) {
        assert.equal(actualMap.sourcesContent[i],
                     expectedMap.sourcesContent[i],
                     "sourcesContent[" + i + "] mismatch");
      }
    }
  }
  exports.assertEqualMaps = assertEqualMaps;

});






define('lib/source-map/util', ['require', 'exports', 'module' , ], function(require, exports, module) {

  









  function getArg(aArgs, aName, aDefaultValue) {
    if (aName in aArgs) {
      return aArgs[aName];
    } else if (arguments.length === 3) {
      return aDefaultValue;
    } else {
      throw new Error('"' + aName + '" is a required argument.');
    }
  }
  exports.getArg = getArg;

  function join(aRoot, aPath) {
    return aPath.charAt(0) === '/'
      ? aPath
      : aRoot.replace(/\/$/, '') + '/' + aPath;
  }
  exports.join = join;

  








  function toSetString(aStr) {
    return '$' + aStr;
  }
  exports.toSetString = toSetString;

  function fromSetString(aStr) {
    return aStr.substr(1);
  }
  exports.fromSetString = fromSetString;

  function relative(aRoot, aPath) {
    aRoot = aRoot.replace(/\/$/, '');
    return aPath.indexOf(aRoot + '/') === 0
      ? aPath.substr(aRoot.length + 1)
      : aPath;
  }
  exports.relative = relative;

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
this.runSourceMapTests = runSourceMapTests;
