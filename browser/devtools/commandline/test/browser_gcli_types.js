






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testTypes.js</p>";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, exports);
  }).then(finish);
}



'use strict';


var types = require('gcli/types');

function forEachType(options, typeSpec, callback) {
  types.getTypeNames().forEach(function(name) {
    typeSpec.name = name;
    typeSpec.requisition = options.display.requisition;

    
    if (name === 'selection') {
      typeSpec.data = [ 'a', 'b' ];
    }
    else if (name === 'delegate') {
      typeSpec.delegateType = function() {
        return types.getType('string');
      };
    }
    else if (name === 'array') {
      typeSpec.subtype = 'string';
    }

    var type = types.getType(typeSpec);
    callback(type);
  });
}

exports.testDefault = function(options) {
  if (options.isJsdom) {
    assert.log('Skipping tests due to issues with resource type.');
    return;
  }

  forEachType(options, {}, function(type) {
    var blank = type.getBlank().value;

    
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
  forEachType(options, { defaultValue: null }, function(type) {
    assert.is(type.stringify(null, null), '', 'stringify(null) for ' + type.name);
  });
};



