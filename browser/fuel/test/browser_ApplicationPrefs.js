
var testdata = {
  missing: "fuel.fuel-test-missing",
  dummy: "fuel.fuel-test",
  string: "browser.active_color",
  integer: "permissions.default.image",
  boolean: "browser.blink_allowed"
};

function test() {
  
  var itemValue = Application.prefs.getValue(testdata.missing, "default");
  is(itemValue, "default", "Check 'Application.prefs.getValue' for non-existing item");
  
  is(Application.prefs.get(testdata.missing), null, "Check 'Application.prefs.get' for non-existing item");
  
  
  Application.prefs.setValue(testdata.dummy, "dummy");
  itemValue = Application.prefs.getValue(testdata.dummy, "default");
  is(itemValue, "dummy", "Check 'Application.prefs.getValue' for existing item");

  
  Application.prefs.setValue(testdata.dummy, "smarty");
  itemValue = Application.prefs.getValue(testdata.dummy, "default");
  is(itemValue, "smarty", "Check 'Application.prefs.getValue' for overwritten item");
  
  
  Application.prefs.get(testdata.dummy).value = "dummy2";
  itemValue = Application.prefs.get(testdata.dummy).value;
  is(itemValue, "dummy2", "Check 'Application.prefs.get().value' for existing item");

  
  Application.prefs.get(testdata.dummy).reset();
  var itemValue = Application.prefs.getValue(testdata.dummy, "default");
  is(itemValue, "default", "Check 'Application.prefs.getValue' for reset pref");
  
  
  ok(!Application.prefs.has(testdata.dummy), "Check non-existant property for existance");
  
  
  
  
  ok(Application.prefs.has(testdata.string), "Check existing string property for existance");
  
  
  var val = Application.prefs.getValue(testdata.dummy, "default");
  is(val, "default", "Check non-existant string property for expected value");
  
  
  var val = Application.prefs.getValue(testdata.string, "default");
  is(val, "#EE0000", "Check existing string property for expected value");
  
  
  Application.prefs.setValue(testdata.string, "#EF0000");
  var val = Application.prefs.getValue(testdata.string, "default");
  is(val, "#EF0000", "Set existing string property");
  
  
  Application.prefs.get(testdata.string).reset();
  var val = Application.prefs.getValue(testdata.string, "default");
  is(val, "#EE0000", "Reset existing string property");
  
  
  
  
  ok(Application.prefs.has(testdata.integer), "Check existing integer property for existance");
  
  
  var val = Application.prefs.getValue(testdata.dummy, 0);
  is(val, 0, "Check non-existant integer property for expected value");
  
  
  var val = Application.prefs.getValue(testdata.integer, 0);
  is(val, 1, "Check existing integer property for expected value");

  
  Application.prefs.setValue(testdata.integer, 0);
  var val = Application.prefs.getValue(testdata.integer, 1);
  is(val, 0, "Set existing integer property");
  
  
  Application.prefs.get(testdata.integer).reset();
  var val = Application.prefs.getValue(testdata.integer, 0);
  is(val, 1, "Reset existing integer property");

  
  
  
  ok(Application.prefs.has(testdata.boolean), "Check existing boolean property for existance");
  
  
  var val = Application.prefs.getValue(testdata.dummy, true);
  ok(val, "Check non-existant boolean property for expected value");
  
  
  var val = Application.prefs.getValue(testdata.boolean, false);
  ok(val, "Check existing boolean property for expected value");
  
  
  Application.prefs.setValue(testdata.boolean, false);
  var val = Application.prefs.getValue(testdata.boolean, true);
  ok(!val, "Set existing boolean property");
  
  
  Application.prefs.get(testdata.boolean).reset();
  var val = Application.prefs.getValue(testdata.boolean, false);
  ok(val, "Reset existing string property for expected value");
  
  
  var allPrefs = Application.prefs.all;
  ok(allPrefs.length >= 800, "Check 'Application.prefs.all' for the right number of preferences");
  is(allPrefs[0].name, "capability.policy.default.Window.parent.get", "Check 'Application.prefs.all' for the right starting preference");

  
  is(Application.prefs.root, "", "Check the Application preference root");
  
  
  ok(Application.prefs.get("browser.shell.checkDefaultBrowser").modified, "A single preference is marked as modified.");
  ok(!Application.prefs.get(testdata.string).modified, "A single preference is marked as not modified.");
  
  
  var pref = Application.prefs.get(testdata.string);
  ok(!pref.locked, "A single preference should not be locked.");
  
  pref.locked = true;
  ok(pref.locked, "A single preference should be locked.");
  
  try {
    prev.value = "test value";
    
    ok(false, "A locked preference should not be able to be modified.");
  } catch(e){
    ok(true, "A locked preference should not be able to be modified.");
  }
  
  pref.locked = false;
  ok(!pref.locked, "A single preference should not be locked.");
  
  
  waitForExplicitFinish();
  Application.prefs.events.addListener("change", onPrefChange);
  Application.prefs.setValue("fuel.fuel-test", "change event");
}

function onPrefChange(evt) {
  is(evt.data, testdata.dummy, "Check 'Application.prefs.set' fired a change event");
  Application.prefs.events.removeListener("change", onPrefChange);

  Application.prefs.get("fuel.fuel-test").events.addListener("change", onPrefChange2);
  Application.prefs.setValue("fuel.fuel-test", "change event2");
}

function onPrefChange2(evt) {
  is(evt.data, testdata.dummy, "Check 'Application.prefs.set' fired a change event for a single preference");
  Application.prefs.events.removeListener("change", onPrefChange2);
  
  finish();
}
