






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testTooltip.js</p>";

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








exports.setup = function() {
  mockCommands.setup();
};

exports.shutdown = function() {
  mockCommands.shutdown();
};


function type(typed, tests, options) {
  var inputter = options.display.inputter;
  var tooltip = options.display.tooltip;

  inputter.setInput(typed);
  if (tests.cursor) {
    inputter.setCursor({ start: tests.cursor, end: tests.cursor });
  }

  if (!options.isJsdom) {
    if (tests.important) {
      assert.ok(tooltip.field.isImportant, 'Important for ' + typed);
    }
    else {
      assert.ok(!tooltip.field.isImportant, 'Not important for ' + typed);
    }

    if (tests.options) {
      var names = tooltip.field.menu.items.map(function(item) {
        return item.name.textContent ? item.name.textContent : item.name;
      });
      assert.is(tests.options.join('|'), names.join('|'), 'Options for ' + typed);
    }

    if (tests.error) {
      assert.is(tests.error, tooltip.errorEle.textContent, 'Error for ' + typed);
    }
    else {
      assert.is('', tooltip.errorEle.textContent, 'No error for ' + typed);
    }
  }
}

exports.testActivate = function(options) {
  if (!options.display) {
    assert.log('No display. Skipping activate tests');
    return;
  }

  if (options.isJsdom) {
    assert.log('Reduced checks due to JSDom.textContent');
  }

  type(' ', { }, options);

  type('tsb ', {
    important: true,
    options: [ 'false', 'true' ]
  }, options);

  type('tsb t', {
    important: true,
    options: [ 'true' ]
  }, options);

  type('tsb tt', {
    important: true,
    options: [ 'true' ]
  }, options);

  type('wxqy', {
    important: false,
    options: [ ],
    error: 'Can\'t use \'wxqy\'.'
  }, options);

  type('', { }, options);
};



