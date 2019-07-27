



'use strict';

let {
  Loader, main, unload, parseStack, generateMap, resolve, nodeResolve
} = require('toolkit/loader');
let { readURI } = require('sdk/net/url');
let { all } = require('sdk/core/promise');
let testOptions = require('@test/options');

let root = module.uri.substr(0, module.uri.lastIndexOf('/'))

const { Cu } = require('chrome');
const { addDebuggerToGlobal } = Cu.import('resource://gre/modules/jsdebugger.jsm', {});
addDebuggerToGlobal(this);


exports['test nodeResolve'] = function (assert) {
  let rootURI = root + '/fixtures/native-addon-test/';
  let manifest = {};
  manifest.dependencies = {};

  
  resolveTest('../package.json', './dir/c.js', './package.json');
  resolveTest('../dir/b.js', './dir/c.js', './dir/b.js');

  resolveTest('./dir/b', './index.js', './dir/b.js');
  resolveTest('../index', './dir/b.js', './index.js');
  resolveTest('../', './dir/b.js', './index.js');
  resolveTest('./dir/a', './index.js', './dir/a.js', 'Precedence dir/a.js over dir/a/');
  resolveTest('../utils', './dir/a.js', './utils/index.js', 'Requiring a directory defaults to dir/index.js');
  resolveTest('../newmodule', './dir/c.js', './newmodule/lib/file.js', 'Uses package.json main in dir to load appropriate "main"');
  resolveTest('test-math', './utils/index.js', './node_modules/test-math/index.js',
    'Dependencies default to their index.js');
  resolveTest('test-custom-main', './utils/index.js', './node_modules/test-custom-main/lib/custom-entry.js',
    'Dependencies use "main" entry');
  resolveTest('test-math/lib/sqrt', './utils/index.js', './node_modules/test-math/lib/sqrt.js',
    'Dependencies\' files can be consumed via "/"');

  resolveTest('sdk/tabs/utils', './index.js', undefined,
    'correctly ignores SDK references in paths');
  resolveTest('fs', './index.js', undefined,
    'correctly ignores built in node modules in paths');

  resolveTest('test-add', './node_modules/test-math/index.js',
    './node_modules/test-math/node_modules/test-add/index.js',
    'Dependencies\' dependencies can be found');


  function resolveTest (id, requirer, expected, msg) {
    let result = nodeResolve(id, requirer, { manifest: manifest, rootURI: rootURI });
    assert.equal(result, expected, 'nodeResolve ' + id + ' from ' + requirer + ' ' +msg);
  }
}








exports['test generateMap()'] = function (assert, done) {
  getJSON('/fixtures/native-addon-test/expectedmap.json').then(expected => {
    generateMap({
      rootURI: root + '/fixtures/native-addon-test/'
    }, map => {
      assert.deepEqual(map, expected, 'generateMap returns expected mappings');
      assert.equal(map['./index.js']['./dir/a'], './dir/a.js',
        'sanity check on correct mappings');
      done();
    });
  }).then(null, (reason) => console.error(reason));
};

exports['test JSM loading'] = function (assert, done) {
  getJSON('/fixtures/jsm-package/package.json').then(manifest => {
    let rootURI = root + '/fixtures/jsm-package/';
    let loader = Loader({
      paths: makePaths(rootURI),
      rootURI: rootURI,
      manifest: manifest,
      isNative: true
    });

    let program = main(loader);
    assert.ok(program.localJSMCached, 'local relative JSMs are cached');
    assert.ok(program.isCachedJSAbsolute , 'absolute resource:// js are cached');
    assert.ok(program.isCachedPath, 'JSMs resolved in paths are cached');
    assert.ok(program.isCachedAbsolute, 'absolute resource:// JSMs are cached');

    assert.ok(program.localJSM, 'able to load local relative JSMs');
    all([
      program.isLoadedPath(10),
      program.isLoadedAbsolute(20),
      program.isLoadedJSAbsolute(30)
    ]).then(([path, absolute, jsabsolute]) => {
      assert.equal(path, 10, 'JSM files resolved from path work');
      assert.equal(absolute, 20, 'JSM files resolved from full resource:// work');
      assert.equal(jsabsolute, 30, 'JS files resolved from full resource:// work');
    }).then(done, console.error);

  }).then(null, console.error);
};

exports['test native Loader with mappings'] = function (assert, done) {
  all([
    getJSON('/fixtures/native-addon-test/expectedmap.json'),
    getJSON('/fixtures/native-addon-test/package.json')
  ]).then(([expectedMap, manifest]) => {

    
    
    expectedMap['./index.js']['./dir/dummy'] = './dir/a.js';

    let rootURI = root + '/fixtures/native-addon-test/';
    let loader = Loader({
      paths: makePaths(rootURI),
      rootURI: rootURI,
      manifest: manifest,
      requireMap: expectedMap,
      isNative: true
    });

    let program = main(loader);
    assert.equal(program.dummyModule, 'dir/a',
      'The lookup uses the information given in the mapping');

    testLoader(program, assert);
    unload(loader);
    done();
  }).then(null, (reason) => console.error(reason));
};

exports['test native Loader without mappings'] = function (assert, done) {
  getJSON('/fixtures/native-addon-test/package.json').then(manifest => {
    let rootURI = root + '/fixtures/native-addon-test/';
    let loader = Loader({
      paths: makePaths(rootURI),
      rootURI: rootURI,
      manifest: manifest,
      isNative: true
    });

    let program = main(loader);
    testLoader(program, assert);
    unload(loader);
    done();
  }).then(null, (reason) => console.error(reason));
};

exports["test require#resolve with relative, dependencies"] = function(assert, done) {
  getJSON('/fixtures/native-addon-test/package.json').then(manifest => {
    let rootURI = root + '/fixtures/native-addon-test/';
    let loader = Loader({
      paths: makePaths(rootURI),
      rootURI: rootURI,
      manifest: manifest,
      isNative: true
    });

    let program = main(loader);
    let fixtureRoot = program.require.resolve("./").replace(/native-addon-test\/(.*)/, "") + "native-addon-test/";

    assert.ok(/^resource:\/\/[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}-at-jetpack\/addon-sdk\/tests\/fixtures\/native-addon-test\/$/.test(fixtureRoot),
      "correct resolution root");
    assert.equal(program.require.resolve("test-math"), fixtureRoot + "node_modules/test-math/index.js", "works with node_modules");
    assert.equal(program.require.resolve("./newmodule"), fixtureRoot + "newmodule/lib/file.js", "works with directory mains");
    assert.equal(program.require.resolve("./dir/a"), fixtureRoot + "dir/a.js", "works with normal relative module lookups");
    assert.equal(program.require.resolve("modules/Promise.jsm"), "resource://gre/modules/Promise.jsm", "works with path lookups");

    
    
    

    unload(loader);
    done();
  }).then(null, (reason) => console.error(reason));
};

function testLoader (program, assert) {
  
  
  assert.equal(program.customMainModule, 'custom entry file',
    'a node_module dependency correctly uses its `main` entry in manifest');
  
  assert.equal(program.customMainModuleRelative, 'custom entry file relative',
    'a node_module dependency correctly uses its `main` entry in manifest with relative ./');
  
  assert.equal(program.defaultMain, 'default main',
    'a node_module dependency correctly defautls to index.js for main');

  
  assert.equal(program.directoryDefaults, 'utils',
    '`require`ing a directory defaults to dir/index.js');
  assert.equal(program.directoryMain, 'main from new module',
    '`require`ing a directory correctly loads the `main` entry and not index.js');
  assert.equal(program.resolvesJSoverDir, 'dir/a',
    '`require`ing "a" resolves "a.js" over "a/index.js"');

  
  assert.ok(program.math.add,
    'correctly defaults to index.js of a module');
  assert.equal(program.math.add(10, 5), 15,
    'node dependencies correctly include their own dependencies');
  assert.equal(program.math.subtract(10, 5), 5,
    'node dependencies correctly include their own dependencies');
  assert.equal(program.mathInRelative.subtract(10, 5), 5,
    'relative modules can also include node dependencies');

  
  assert.ok(program.promise.defer, 'main entry can include SDK modules with no deps');
  assert.ok(program.promise.resolve, 'main entry can include SDK modules with no deps');
  assert.ok(program.eventCore.on, 'main entry can include SDK modules that have dependencies');
  assert.ok(program.eventCore.off, 'main entry can include SDK modules that have dependencies');

  
  assert.ok(program.promisejsm.defer, 'can require JSM files in path');
  assert.equal(program.localJSM.test, 'this is a jsm',
    'can require relative JSM files');

  
  assert.equal(program.areModulesCached, true,
    'modules are correctly cached');
  assert.equal(program.testJSON.dependencies['test-math'], '*',
    'correctly requires JSON files');
}

function getJSON (uri) {
  return readURI(root + uri).then(manifest => JSON.parse(manifest));
}

function makePaths (uri) {
  
  let sdkPaths = testOptions.paths ? testOptions.paths[''] : 'resource://gre/modules/commonjs/';
  return {
    './': uri,
    'sdk/': sdkPaths + 'sdk/',
    'toolkit/': sdkPaths + 'toolkit/',
    'modules/': 'resource://gre/modules/'
  };
}

function loadAddon (uri, map) {
  let rootURI = root + uri;
  getJSON(uri + '/package.json').then(manifest => {
    let loader = Loader({
      paths: makePaths(rootURI),
      rootURI: rootURI,
      manifest: manifest,
      isNative: true,
      modules: {
        '@test/options': testOptions
      }
    });
    let program = main(loader);
  }).then(null, console.error);
}

require('test').run(exports);
