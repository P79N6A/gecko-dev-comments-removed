






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testKeyboard2.js</p>";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, exports);
  }).then(finish);
}



'use strict';




exports.setup = function(options) {
  mockCommands.setup();
};

exports.shutdown = function(options) {
  mockCommands.shutdown();
};



exports.testSimple = function(options) {
  return helpers.audit(options, [
    {
      setup: 'tsela<TAB>',
      check: { input: 'tselarr ', cursor: 8 }
    },
    {
      setup: 'tsn di<TAB>',
      check: { input: 'tsn dif ', cursor: 8 }
    },
    {
      setup: 'tsg a<TAB>',
      check: { input: 'tsg aaa ', cursor: 8 }
    }
  ]);
};

exports.testIncr = function(options) {
  return helpers.audit(options, [
    
















    {
      setup: 'tsu -5<UP>',
      check: { input: 'tsu -3' }
    },
    {
      setup: 'tsu -4<UP>',
      check: { input: 'tsu -3' }
    },
    {
      setup: 'tsu -3<UP>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu -2<UP>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu -1<UP>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu 0<UP>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 1<UP>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 2<UP>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 3<UP>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 4<UP>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 5<UP>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 6<UP>',
      check: { input: 'tsu 9' }
    },
    {
      setup: 'tsu 7<UP>',
      check: { input: 'tsu 9' }
    },
    {
      setup: 'tsu 8<UP>',
      check: { input: 'tsu 9' }
    },
    {
      setup: 'tsu 9<UP>',
      check: { input: 'tsu 10' }
    },
    {
      setup: 'tsu 10<UP>',
      check: { input: 'tsu 10' }
    }
    






  ]);
};

exports.testDecr = function(options) {
  return helpers.audit(options, [
    














    {
      setup: 'tsu -5<DOWN>',
      check: { input: 'tsu -5' }
    },
    {
      setup: 'tsu -4<DOWN>',
      check: { input: 'tsu -5' }
    },
    {
      setup: 'tsu -3<DOWN>',
      check: { input: 'tsu -5' }
    },
    {
      setup: 'tsu -2<DOWN>',
      check: { input: 'tsu -3' }
    },
    {
      setup: 'tsu -1<DOWN>',
      check: { input: 'tsu -3' }
    },
    {
      setup: 'tsu 0<DOWN>',
      check: { input: 'tsu -3' }
    },
    {
      setup: 'tsu 1<DOWN>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu 2<DOWN>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu 3<DOWN>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu 4<DOWN>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 5<DOWN>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 6<DOWN>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 7<DOWN>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 8<DOWN>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 9<DOWN>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 10<DOWN>',
      check: { input: 'tsu 9' }
    }
    






  ]);
};

exports.testIncrFloat = function(options) {
  return helpers.audit(options, [
    






    {
      setup: 'tsf -6.5<UP>',
      check: { input: 'tsf -6' }
    },
    {
      setup: 'tsf -6<UP>',
      check: { input: 'tsf -4.5' }
    },
    {
      setup: 'tsf -4.5<UP>',
      check: { input: 'tsf -3' }
    },
    {
      setup: 'tsf -4<UP>',
      check: { input: 'tsf -3' }
    },
    {
      setup: 'tsf -3<UP>',
      check: { input: 'tsf -1.5' }
    },
    {
      setup: 'tsf -1.5<UP>',
      check: { input: 'tsf 0' }
    },
    {
      setup: 'tsf 0<UP>',
      check: { input: 'tsf 1.5' }
    },
    {
      setup: 'tsf 1.5<UP>',
      check: { input: 'tsf 3' }
    },
    {
      setup: 'tsf 2<UP>',
      check: { input: 'tsf 3' }
    },
    {
      setup: 'tsf 3<UP>',
      check: { input: 'tsf 4.5' }
    },
    {
      setup: 'tsf 5<UP>',
      check: { input: 'tsf 6' }
    }
    






  ]);
};

exports.testDecrFloat = function(options) {
  return helpers.audit(options, [
    






    {
      setup: 'tsf -6.5<DOWN>',
      check: { input: 'tsf -6.5' }
    },
    {
      setup: 'tsf -6<DOWN>',
      check: { input: 'tsf -6.5' }
    },
    {
      setup: 'tsf -4.5<DOWN>',
      check: { input: 'tsf -6' }
    },
    {
      setup: 'tsf -4<DOWN>',
      check: { input: 'tsf -4.5' }
    },
    {
      setup: 'tsf -3<DOWN>',
      check: { input: 'tsf -4.5' }
    },
    {
      setup: 'tsf -1.5<DOWN>',
      check: { input: 'tsf -3' }
    },
    {
      setup: 'tsf 0<DOWN>',
      check: { input: 'tsf -1.5' }
    },
    {
      setup: 'tsf 1.5<DOWN>',
      check: { input: 'tsf 0' }
    },
    {
      setup: 'tsf 2<DOWN>',
      check: { input: 'tsf 1.5' }
    },
    {
      setup: 'tsf 3<DOWN>',
      check: { input: 'tsf 1.5' }
    },
    {
      setup: 'tsf 5<DOWN>',
      check: { input: 'tsf 4.5' }
    }
    






  ]);
};

exports.testIncrSelection = function(options) {
  



















};

exports.testDecrSelection = function(options) {
  








};


