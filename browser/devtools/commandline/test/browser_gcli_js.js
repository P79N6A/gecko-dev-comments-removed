






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testJs.js</p>";

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




var Requisition = require('gcli/cli').Requisition;
var Status = require('gcli/types').Status;
var javascript = require('gcli/types/javascript');
var canon = require('gcli/canon');



var debug = false;
var requ;

var assign;
var status;
var statuses;
var tempWindow;


exports.setup = function(options) {
  tempWindow = javascript.getGlobalObject();
  javascript.setGlobalObject(options.window);

  Object.defineProperty(options.window, 'donteval', {
    get: function() {
      assert.ok(false, 'donteval should not be used');
      return { cant: '', touch: '', 'this': '' };
    },
    enumerable: true,
    configurable : true
  });
};

exports.shutdown = function(options) {
  delete options.window.donteval;

  javascript.setGlobalObject(tempWindow);
  tempWindow = undefined;
};

function input(typed) {
  if (!requ) {
    requ = new Requisition();
  }
  var cursor = { start: typed.length, end: typed.length };
  requ.update(typed);

  if (debug) {
    console.log('####### TEST: typed="' + typed +
        '" cur=' + cursor.start +
        ' cli=', requ);
  }

  status = requ.getStatus();
  statuses = requ.getInputStatusMarkup(cursor.start).map(function(s) {
    return Array(s.string.length + 1).join(s.status.toString()[0]);
  }).join('');

  if (requ.commandAssignment.value) {
    assign = requ.getAssignment(0);
  }
  else {
    assign = undefined;
  }
}

function predictionsHas(name) {
  return assign.getPredictions().some(function(prediction) {
    return name === prediction.name;
  }, this);
}

function check(expStatuses, expStatus, expAssign, expPredict) {
  assert.is('{', requ.commandAssignment.value.name, 'is exec');

  assert.is(expStatuses, statuses, 'unexpected status markup');
  assert.is(expStatus.toString(), status.toString(), 'unexpected status');
  assert.is(expAssign, assign.value, 'unexpected assignment');

  if (expPredict != null) {
    var contains;
    if (Array.isArray(expPredict)) {
      expPredict.forEach(function(p) {
        contains = predictionsHas(p);
        assert.ok(contains, 'missing prediction ' + p);
      });
    }
    else if (typeof expPredict === 'number') {
      contains = true;
      assert.is(assign.getPredictions().length, expPredict, 'prediction count');
      if (assign.getPredictions().length !== expPredict) {
        assign.getPredictions().forEach(function(prediction) {
          assert.log('actual prediction: ', prediction);
        });
      }
    }
    else {
      contains = predictionsHas(expPredict);
      assert.ok(contains, 'missing prediction ' + expPredict);
    }

    if (!contains) {
      assert.log('Predictions: ' + assign.getPredictions().map(function(p) {
        return p.name;
      }).join(', '));
    }
  }
}

exports.testBasic = function(options) {
  if (!canon.getCommand('{')) {
    assert.log('Skipping exec tests because { is not registered');
    return;
  }

  input('{');
  check('V', Status.ERROR, undefined);

  input('{ ');
  check('VV', Status.ERROR, undefined);

  input('{ w');
  check('VVI', Status.ERROR, 'w', 'window');

  input('{ windo');
  check('VVIIIII', Status.ERROR, 'windo', 'window');

  input('{ window');
  check('VVVVVVVV', Status.VALID, 'window');

  input('{ window.d');
  check('VVIIIIIIII', Status.ERROR, 'window.d', 'window.document');

  input('{ window.document.title');
  check('VVVVVVVVVVVVVVVVVVVVVVV', Status.VALID, 'window.document.title', 0);

  input('{ d');
  check('VVI', Status.ERROR, 'd', 'document');

  input('{ document.title');
  check('VVVVVVVVVVVVVVVV', Status.VALID, 'document.title', 0);

  if (!options.isJsdom) {
    
    assert.ok('donteval' in options.window, 'donteval exists');
  }

  input('{ don');
  check('VVIII', Status.ERROR, 'don', 'donteval');

  input('{ donteval');
  check('VVVVVVVVVV', Status.VALID, 'donteval', 0);

  








  input('{ donteval.cant');
  check('VVVVVVVVVVVVVVV', Status.VALID, 'donteval.cant', 0);

  input('{ donteval.xxx');
  check('VVVVVVVVVVVVVV', Status.VALID, 'donteval.xxx', 0);
};



