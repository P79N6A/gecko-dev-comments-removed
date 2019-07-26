


'use strict';

const { PrefsTarget } = require('sdk/preferences/event-target');
const { get, set, reset } = require('sdk/preferences/service');
const { Loader } = require('sdk/test/loader');
const { setTimeout } = require('sdk/timers');

const root = PrefsTarget();

exports.testPrefsTarget = function(test) {
  test.waitUntilDone();

  let loader = Loader(module);
  let pt = loader.require('sdk/preferences/event-target').PrefsTarget({});
  let name = 'test';

  test.assertEqual(get(name, ''), '', 'test pref is blank');

  pt.once(name, function() {
    test.assertEqual(pt.prefs[name], 2, 'test pref is 2');

    pt.once(name, function() {
      test.fail('should not have heard a pref change');
    });
    loader.unload();
    root.once(name, function() {
      test.pass('test pref was changed');
      reset(name);

      
      
      
      setTimeout(function() test.done());
    });
    set(name, 3);
  });

  pt.prefs[name] = 2;
};
