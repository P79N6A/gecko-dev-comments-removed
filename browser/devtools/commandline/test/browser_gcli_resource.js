






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testResource.js</p>";

function test() {
  var tests = Object.keys(exports);
  
  tests.sort(function(t1, t2) {
    if (t1 == "setup" || t2 == "shutdown") return -1;
    if (t2 == "setup" || t1 == "shutdown") return 1;
    return 0;
  });
  info("Running tests: " + tests.join(", "))
  tests = tests.map(function(test) { return exports[test]; });
  DeveloperToolbarTest.test(TEST_URI, tests, true);
}




var resource = require('gcli/types/resource');
var types = require('gcli/types');
var Status = require('gcli/types').Status;



var tempDocument;

exports.setup = function(options) {
  tempDocument = resource.getDocument();
  resource.setDocument(options.window.document);
};

exports.shutdown = function(options) {
  resource.setDocument(tempDocument);
  tempDocument = undefined;
};

exports.testPredictions = function(options) {
  if (options.isJsdom) {
    assert.log('Skipping checks due to jsdom document.stylsheets support.');
    return;
  }

  var resource1 = types.getType('resource');
  var options1 = resource1.getLookup();
  
  if (!options.isFirefox) {
    assert.ok(options1.length > 1, 'have all resources');
  }
  else {
    assert.log('Skipping checks due to firefox document.stylsheets support.');
  }
  options1.forEach(function(prediction) {
    checkPrediction(resource1, prediction);
  });

  var resource2 = types.getType({ name: 'resource', include: 'text/javascript' });
  var options2 = resource2.getLookup();
  
  if (!options.isFirefox) {
    assert.ok(options2.length > 1, 'have js resources');
  }
  else {
    assert.log('Skipping checks due to firefox document.stylsheets support.');
  }
  options2.forEach(function(prediction) {
    checkPrediction(resource2, prediction);
  });

  var resource3 = types.getType({ name: 'resource', include: 'text/css' });
  var options3 = resource3.getLookup();
  
  if (!options.isJsdom && !options.isFirefox) {
    assert.ok(options3.length >= 1, 'have css resources');
  }
  else {
    assert.log('Skipping checks due to firefox document.stylsheets support.');
  }
  options3.forEach(function(prediction) {
    checkPrediction(resource3, prediction);
  });

  var resource4 = types.getType({ name: 'resource' });
  var options4 = resource4.getLookup();

  assert.is(options1.length, options4.length, 'type spec');
  assert.is(options2.length + options3.length, options4.length, 'split');
};

function checkPrediction(res, prediction) {
  var name = prediction.name;
  var value = prediction.value;

  var conversion = res.parseString(name);
  assert.is(conversion.getStatus(), Status.VALID, 'status VALID for ' + name);
  assert.is(conversion.value, value, 'value for ' + name);

  var strung = res.stringify(value);
  assert.is(strung, name, 'stringify for ' + name);

  assert.is(typeof value.loadContents, 'function', 'resource for ' + name);
  assert.is(typeof value.element, 'object', 'resource for ' + name);
}


