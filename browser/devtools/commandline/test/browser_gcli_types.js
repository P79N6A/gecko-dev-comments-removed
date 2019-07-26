















'use strict';





var exports = {};

var TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testTypes.js</p>";

function test() {
  return Task.spawn(function() {
    let options = yield helpers.openTab(TEST_URI);
    yield helpers.openToolbar(options);
    gcli.addItems(mockCommands.items);

    yield helpers.runTests(options, exports);

    gcli.removeItems(mockCommands.items);
    yield helpers.closeToolbar(options);
    yield helpers.closeTab(options);
  }).then(finish, helpers.handleError);
}




var util = require('gcli/util/util');
var promise = require('gcli/util/promise');
var nodetype = require('gcli/types/node');

exports.setup = function(options) {
  if (options.window) {
    nodetype.setDocument(options.window.document);
  }
};

exports.shutdown = function(options) {
  nodetype.unsetDocument();
};

function forEachType(options, typeSpec, callback) {
  var types = options.requisition.types;
  return util.promiseEach(types.getTypeNames(), function(name) {
    typeSpec.name = name;
    typeSpec.requisition = options.requisition;

    
    if (name === 'selection') {
      typeSpec.data = [ 'a', 'b' ];
    }
    else if (name === 'delegate') {
      typeSpec.delegateType = function() {
        return 'string';
      };
    }
    else if (name === 'array') {
      typeSpec.subtype = 'string';
    }
    else if (name === 'remote') {
      return;
    }

    var type = types.createType(typeSpec);
    var reply = callback(type);
    return promise.resolve(reply).then(function(value) {
      
      delete typeSpec.name;
      delete typeSpec.requisition;
      delete typeSpec.data;
      delete typeSpec.delegateType;
      delete typeSpec.subtype;

      return value;
    });
  });
}

exports.testDefault = function(options) {
  if (options.isNoDom) {
    assert.log('Skipping tests due to issues with resource type.');
    return;
  }

  return forEachType(options, {}, function(type) {
    var context = options.requisition.executionContext;
    var blank = type.getBlank(context).value;

    
    if (type.name === 'boolean') {
      assert.is(blank, false, 'blank boolean is false');
    }
    else if (type.name === 'array') {
      assert.ok(Array.isArray(blank), 'blank array is array');
      assert.is(blank.length, 0, 'blank array is empty');
    }
    else if (type.name === 'nodelist') {
      assert.ok(typeof blank.item, 'function', 'blank.item is function');
      assert.is(blank.length, 0, 'blank nodelist is empty');
    }
    else {
      assert.is(blank, undefined, 'default defined for ' + type.name);
    }
  });
};

exports.testNullDefault = function(options) {
  var context = null; 

  return forEachType(options, { defaultValue: null }, function(type) {
    var reply = type.stringify(null, context);
    return promise.resolve(reply).then(function(str) {
      assert.is(str, '', 'stringify(null) for ' + type.name);
    });
  });
};
