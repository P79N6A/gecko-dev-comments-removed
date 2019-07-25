






Components.utils.import('resource://test/Utils.jsm');






define("test/source-map/test-dog-fooding", ["require", "exports", "module"], function (require, exports, module) {

  var SourceMapConsumer = require('source-map/source-map-consumer').SourceMapConsumer;
  var SourceMapGenerator = require('source-map/source-map-generator').SourceMapGenerator;

  exports['test eating our own dog food'] = function (assert, util) {
    var smg = new SourceMapGenerator({
      file: 'testing.js',
      sourceRoot: '/wu/tang'
    });

    smg.addMapping({
      source: 'gza.coffee',
      original: { line: 1, column: 0 },
      generated: { line: 2, column: 2 }
    });

    smg.addMapping({
      source: 'gza.coffee',
      original: { line: 2, column: 0 },
      generated: { line: 3, column: 2 }
    });

    smg.addMapping({
      source: 'gza.coffee',
      original: { line: 3, column: 0 },
      generated: { line: 4, column: 2 }
    });

    smg.addMapping({
      source: 'gza.coffee',
      original: { line: 4, column: 0 },
      generated: { line: 5, column: 2 }
    });

    var smc = new SourceMapConsumer(smg.toString());

    
    util.assertMapping(2, 2, '/wu/tang/gza.coffee', 1, 0, null, smc, assert);
    util.assertMapping(3, 2, '/wu/tang/gza.coffee', 2, 0, null, smc, assert);
    util.assertMapping(4, 2, '/wu/tang/gza.coffee', 3, 0, null, smc, assert);
    util.assertMapping(5, 2, '/wu/tang/gza.coffee', 4, 0, null, smc, assert);

    
    util.assertMapping(2, 0, null, null, null, null, smc, assert);
    util.assertMapping(2, 9, '/wu/tang/gza.coffee', 1, 0, null, smc, assert);
    util.assertMapping(3, 0, '/wu/tang/gza.coffee', 1, 0, null, smc, assert);
    util.assertMapping(3, 9, '/wu/tang/gza.coffee', 2, 0, null, smc, assert);
    util.assertMapping(4, 0, '/wu/tang/gza.coffee', 2, 0, null, smc, assert);
    util.assertMapping(4, 9, '/wu/tang/gza.coffee', 3, 0, null, smc, assert);
    util.assertMapping(5, 0, '/wu/tang/gza.coffee', 3, 0, null, smc, assert);
    util.assertMapping(5, 9, '/wu/tang/gza.coffee', 4, 0, null, smc, assert);
  };

});
function run_test() {
  runSourceMapTests('test/source-map/test-dog-fooding', do_throw);
}
