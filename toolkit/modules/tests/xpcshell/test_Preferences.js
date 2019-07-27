


const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu, manager: Cm} = Components;

Cu.import("resource://gre/modules/Preferences.jsm");

function run_test() {
  run_next_test();
}

add_test(function test_set_get_pref() {
  Preferences.set("test_set_get_pref.integer", 1);
  do_check_eq(Preferences.get("test_set_get_pref.integer"), 1);

  Preferences.set("test_set_get_pref.string", "foo");
  do_check_eq(Preferences.get("test_set_get_pref.string"), "foo");

  Preferences.set("test_set_get_pref.boolean", true);
  do_check_eq(Preferences.get("test_set_get_pref.boolean"), true);

  
  Preferences.resetBranch("test_set_get_pref.");

  run_next_test();
});

add_test(function test_set_get_branch_pref() {
  let prefs = new Preferences("test_set_get_branch_pref.");

  prefs.set("something", 1);
  do_check_eq(prefs.get("something"), 1);
  do_check_false(Preferences.has("something"));

  
  prefs.reset("something");

  run_next_test();
});

add_test(function test_set_get_multiple_prefs() {
  Preferences.set({ "test_set_get_multiple_prefs.integer":  1,
                    "test_set_get_multiple_prefs.string":   "foo",
                    "test_set_get_multiple_prefs.boolean":  true });

  let [i, s, b] = Preferences.get(["test_set_get_multiple_prefs.integer",
                                   "test_set_get_multiple_prefs.string",
                                   "test_set_get_multiple_prefs.boolean"]);

  do_check_eq(i, 1);
  do_check_eq(s, "foo");
  do_check_eq(b, true);

  
  Preferences.resetBranch("test_set_get_multiple_prefs.");

  run_next_test();
});

add_test(function test_get_multiple_prefs_with_default_value() {
  Preferences.set({ "test_get_multiple_prefs_with_default_value.a":  1,
                    "test_get_multiple_prefs_with_default_value.b":  2 });

  let [a, b, c] = Preferences.get(["test_get_multiple_prefs_with_default_value.a",
                                   "test_get_multiple_prefs_with_default_value.b",
                                   "test_get_multiple_prefs_with_default_value.c"],
                                  0);

  do_check_eq(a, 1);
  do_check_eq(b, 2);
  do_check_eq(c, 0);

  
  Preferences.resetBranch("test_get_multiple_prefs_with_default_value.");

  run_next_test();
});

add_test(function test_set_get_unicode_pref() {
  Preferences.set("test_set_get_unicode_pref", String.fromCharCode(960));
  do_check_eq(Preferences.get("test_set_get_unicode_pref"), String.fromCharCode(960));

  
  Preferences.reset("test_set_get_unicode_pref");

  run_next_test();
});

add_test(function test_set_null_pref() {
  try {
    Preferences.set("test_set_null_pref", null);
    
    do_check_true(false);
  }
  catch(ex) {}

  run_next_test();
});

add_test(function test_set_undefined_pref() {
  try {
    Preferences.set("test_set_undefined_pref");
    
    do_check_true(false);
  }
  catch(ex) {}

  run_next_test();
});

add_test(function test_set_unsupported_pref() {
  try {
    Preferences.set("test_set_unsupported_pref", new Array());
    
    do_check_true(false);
  }
  catch(ex) {}

  run_next_test();
});




add_test(function test_get_string_pref() {
  let svc = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService).
            getBranch("");
  svc.setCharPref("test_get_string_pref", "a normal string");
  do_check_eq(Preferences.get("test_get_string_pref"), "a normal string");

  
  Preferences.reset("test_get_string_pref");

  run_next_test();
});

add_test(function test_get_localized_string_pref() {
  let svc = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService).
            getBranch("");
  let prefName = "test_get_localized_string_pref";
  let localizedString = Cc["@mozilla.org/pref-localizedstring;1"]
    .createInstance(Ci.nsIPrefLocalizedString);
  localizedString.data = "a localized string";
  svc.setComplexValue(prefName, Ci.nsIPrefLocalizedString, localizedString);
  do_check_eq(Preferences.get(prefName, null, Ci.nsIPrefLocalizedString),
    "a localized string");

  
  Preferences.reset(prefName);

  run_next_test();
});

add_test(function test_set_get_number_pref() {
  Preferences.set("test_set_get_number_pref", 5);
  do_check_eq(Preferences.get("test_set_get_number_pref"), 5);

  
  Preferences.set("test_set_get_number_pref", 3.14159);
  do_check_eq(Preferences.get("test_set_get_number_pref"), 3);

  
  try {
    Preferences.set("test_set_get_number_pref", Math.pow(2, 31));
    
    do_check_true(false);
  }
  catch(ex) {}

  
  Preferences.reset("test_set_get_number_pref");

  run_next_test();
});

add_test(function test_reset_pref() {
  Preferences.set("test_reset_pref", 1);
  Preferences.reset("test_reset_pref");
  do_check_eq(Preferences.get("test_reset_pref"), undefined);

  run_next_test();
});

add_test(function test_reset_pref_branch() {
  Preferences.set("test_reset_pref_branch.foo", 1);
  Preferences.set("test_reset_pref_branch.bar", 2);
  Preferences.resetBranch("test_reset_pref_branch.");
  do_check_eq(Preferences.get("test_reset_pref_branch.foo"), undefined);
  do_check_eq(Preferences.get("test_reset_pref_branch.bar"), undefined);

  run_next_test();
});



add_test(function test_reset_nonexistent_pref() {
  Preferences.reset("test_reset_nonexistent_pref");

  run_next_test();
});



add_test(function test_reset_nonexistent_pref_branch() {
  Preferences.resetBranch("test_reset_nonexistent_pref_branch.");

  run_next_test();
});

add_test(function test_observe_prefs_function() {
  let observed = false;
  let observer = function() { observed = !observed };

  Preferences.observe("test_observe_prefs_function", observer);
  Preferences.set("test_observe_prefs_function", "something");
  do_check_true(observed);

  Preferences.ignore("test_observe_prefs_function", observer);
  Preferences.set("test_observe_prefs_function", "something else");
  do_check_true(observed);

  
  Preferences.reset("test_observe_prefs_function");

  run_next_test();
});

add_test(function test_observe_prefs_object() {
  let observer = {
    observed: false,
    observe: function() {
      this.observed = !this.observed;
    }
  };

  Preferences.observe("test_observe_prefs_object", observer.observe, observer);
  Preferences.set("test_observe_prefs_object", "something");
  do_check_true(observer.observed);

  Preferences.ignore("test_observe_prefs_object", observer.observe, observer);
  Preferences.set("test_observe_prefs_object", "something else");
  do_check_true(observer.observed);

  
  Preferences.reset("test_observe_prefs_object");

  run_next_test();
});

add_test(function test_observe_prefs_nsIObserver() {
  let observer = {
    observed: false,
    observe: function(subject, topic, data) {
      this.observed = !this.observed;
      do_check_true(subject instanceof Ci.nsIPrefBranch);
      do_check_eq(topic, "nsPref:changed");
      do_check_eq(data, "test_observe_prefs_nsIObserver");
    }
  };

  Preferences.observe("test_observe_prefs_nsIObserver", observer);
  Preferences.set("test_observe_prefs_nsIObserver", "something");
  do_check_true(observer.observed);

  Preferences.ignore("test_observe_prefs_nsIObserver", observer);
  Preferences.set("test_observe_prefs_nsIObserver", "something else");
  do_check_true(observer.observed);

  
  Preferences.reset("test_observe_prefs_nsIObserver");

  run_next_test();
});


















add_test(function test_observe_value_of_set_pref() {
  let observer = function(newVal) { do_check_eq(newVal, "something") };

  Preferences.observe("test_observe_value_of_set_pref", observer);
  Preferences.set("test_observe_value_of_set_pref", "something");

  
  Preferences.ignore("test_observe_value_of_set_pref", observer);
  Preferences.reset("test_observe_value_of_set_pref");

  run_next_test();
});

add_test(function test_observe_value_of_reset_pref() {
  let observer = function(newVal) { do_check_true(typeof newVal == "undefined") };

  Preferences.set("test_observe_value_of_reset_pref", "something");
  Preferences.observe("test_observe_value_of_reset_pref", observer);
  Preferences.reset("test_observe_value_of_reset_pref");

  
  Preferences.ignore("test_observe_value_of_reset_pref", observer);

  run_next_test();
});

add_test(function test_has_pref() {
  do_check_false(Preferences.has("test_has_pref"));
  Preferences.set("test_has_pref", "foo");
  do_check_true(Preferences.has("test_has_pref"));

  Preferences.set("test_has_pref.foo", "foo");
  Preferences.set("test_has_pref.bar", "bar");
  let [hasFoo, hasBar, hasBaz] = Preferences.has(["test_has_pref.foo",
                                                  "test_has_pref.bar",
                                                  "test_has_pref.baz"]);
  do_check_true(hasFoo);
  do_check_true(hasBar);
  do_check_false(hasBaz);

  
  Preferences.resetBranch("test_has_pref");

  run_next_test();
});

add_test(function test_isSet_pref() {
  
  
  
  do_check_false(Preferences.isSet("toolkit.defaultChromeURI"));
  Preferences.set("toolkit.defaultChromeURI", "foo");
  do_check_true(Preferences.isSet("toolkit.defaultChromeURI"));

  
  Preferences.reset("toolkit.defaultChromeURI");

  run_next_test();
});


























