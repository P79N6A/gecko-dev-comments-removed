


"use strict";

const { Loader } = require("sdk/test/loader");
const { setTimeout } = require("sdk/timers");
const { notify } = require("sdk/deprecated/observer-service");
const { id } = require("sdk/self");
const simplePrefs = require("sdk/simple-prefs");
const { prefs: sp } = simplePrefs;

const specialChars = "!@#$%^&*()_-=+[]{}~`\'\"<>,./?;:";

exports.testIterations = function(assert) {
  sp["test"] = true;
  sp["test.test"] = true;
  let prefAry = [];
  for (var name in sp ) {
    prefAry.push(name);
  }
  assert.ok("test" in sp);
  assert.ok(!sp.getPropertyDescriptor);
  assert.ok(Object.prototype.hasOwnProperty.call(sp, "test"));
  assert.equal(["test", "test.test"].toString(), prefAry.sort().toString(), "for (x in y) part 1/2 works");
  assert.equal(["test", "test.test"].toString(), Object.keys(sp).sort().toString(), "Object.keys works");

  delete sp["test"];
  delete sp["test.test"];
  let prefAry = [];
  for (var name in sp ) {
    prefAry.push(name);
  }
  assert.equal([].toString(), prefAry.toString(), "for (x in y) part 2/2 works");
}

exports.testSetGetBool = function(assert) {
  assert.equal(sp.test, undefined, "Value should not exist");
  sp.test = true;
  assert.ok(sp.test, "Value read should be the value previously set");
};


exports.testSpecialChars = function(assert) {
  let chars = specialChars.split("");
  let len = chars.length;

  let count = 0;
  chars.forEach(function(char) {
    let rand = Math.random() + "";
    simplePrefs.on(char, function onPrefChanged() {
      simplePrefs.removeListener(char, onPrefChanged);
      assert.equal(sp[char], rand, "setting pref with a name that is a special char, " + char + ", worked!");

      
      if (++count == len)
        test.done();
    })
    sp[char] = rand;
  });
};

exports.testSetGetInt = function(assert) {
  assert.equal(sp["test-int"], undefined, "Value should not exist");
  sp["test-int"] = 1;
  assert.equal(sp["test-int"], 1, "Value read should be the value previously set");
};

exports.testSetComplex = function(assert) {
  try {
    sp["test-complex"] = {test: true};
    assert.fail("Complex values are not allowed");
  }
  catch (e) {
    assert.pass("Complex values are not allowed");
  }
};

exports.testSetGetString = function(assert) {
  assert.equal(sp["test-string"], undefined, "Value should not exist");
  sp["test-string"] = "test";
  assert.equal(sp["test-string"], "test", "Value read should be the value previously set");
};

exports.testHasAndRemove = function(assert) {
  sp.test = true;
  assert.ok(("test" in sp), "Value exists");
  delete sp.test;
  assert.equal(sp.test, undefined, "Value should be undefined");
};

exports.testPrefListener = function(assert, done) {
  let listener = function(prefName) {
    simplePrefs.removeListener('test-listener', listener);
    assert.equal(prefName, "test-listen", "The prefs listener heard the right event");
    done();
  };

  simplePrefs.on("test-listen", listener);

  sp["test-listen"] = true;

  
  let toSet = ['wildcard1','wildcard.pref2','wildcard.even.longer.test'];
  let observed = [];

  let wildlistener = function(prefName) {
    if (toSet.indexOf(prefName) > -1) observed.push(prefName);
  };

  simplePrefs.on('',wildlistener);

  toSet.forEach(function(pref) {
    sp[pref] = true;
  });

  assert.ok((observed.length == 3 && toSet.length == 3),
      "Wildcard lengths inconsistent" + JSON.stringify([observed.length, toSet.length]));

  toSet.forEach(function(pref,ii) {
    assert.equal(observed[ii], pref, "Wildcard observed " + pref);
  });

  simplePrefs.removeListener('',wildlistener);

};

exports.testBtnListener = function(assert, done) {
  let name = "test-btn-listen";
  simplePrefs.on(name, function listener() {
    simplePrefs.removeListener(name, listener);
    assert.pass("Button press event was heard");
    done();
  });
  notify((id + "-cmdPressed"), "", name);
};

exports.testPrefRemoveListener = function(assert, done) {
  let counter = 0;

  let listener = function() {
    assert.pass("The prefs listener was not removed yet");

    if (++counter > 1)
      assert.fail("The prefs listener was not removed");

    simplePrefs.removeListener("test-listen2", listener);

    sp["test-listen2"] = false;

    setTimeout(function() {
      assert.pass("The prefs listener was removed");
      done();
    }, 250);
  };

  simplePrefs.on("test-listen2", listener);

  
  sp["test-listen2"] = true;
};


exports.testPrefUnloadListener = function(assert, done) {
  let loader = Loader(module);
  let sp = loader.require("sdk/simple-prefs");
  let counter = 0;

  let listener = function() {
    assert.equal(++counter, 1, "This listener should only be called once");

    loader.unload();

    
    sp.prefs["test-listen3"] = false;
    
    require("sdk/simple-prefs").prefs["test-listen3"] = false;

    done();
  };

  sp.on("test-listen3", listener);

  
  sp.prefs["test-listen3"] = true;
};



exports.testPrefUnloadWildcardListener = function(assert, done) {
  let testpref = "test-wildcard-unload-listener";
  let loader = Loader(module);
  let sp = loader.require("sdk/simple-prefs");
  let counter = 0;

  let listener = function() {
    assert.equal(++counter, 1, "This listener should only be called once");

    loader.unload();

    
    sp.prefs[testpref] = false;
    
    require("sdk/simple-prefs").prefs[testpref] = false;

    done();
  };

  sp.on("", listener);
  
  sp.prefs[testpref] = true;
};



exports.testPrefJSONStringification = function(assert) {
  var sp = require("sdk/simple-prefs").prefs;
  assert.equal(
      Object.keys(sp).join(),
      Object.keys(JSON.parse(JSON.stringify(sp))).join(),
      "JSON stringification should work.");
};

require('sdk/test').run(exports);
