




var prefObserver = {
    setCalledNum: 0,
    onContentPrefSet: function(aGroup, aName, aValue) {
        this.setCalledNum++;
    },
    removedCalledNum: 0,
    onContentPrefRemoved: function(aGroup, aName) {
        this.removedCalledNum++;
    }
};

function run_test() {
  let loadContext = { get usePrivateBrowsing() { return gInPrivateBrowsing; } };

  var cps = new ContentPrefInstance(loadContext);
  cps.removeGroupedPrefs();

  var uri = ContentPrefTest.getURI("http://www.example.com/");
  var group = cps.grouper.group(uri);

  
  cps.setPref(uri, "value", "foo");
  cps.setPref(null, "value-global", "foo-global");

  var num;
  cps.addObserver("value", prefObserver);
  cps.addObserver("value-global", prefObserver);

  enterPBMode();

  
  num = prefObserver.setCalledNum;
  cps.setPref(uri, "value", "foo-private-browsing");
  do_check_eq(cps.hasPref(uri, "value"), true);
  do_check_eq(cps.getPref(uri, "value"), "foo-private-browsing");
  do_check_eq(prefObserver.setCalledNum, num + 1);

  num = prefObserver.setCalledNum;
  cps.setPref(null, "value-global", "foo-private-browsing-global");
  do_check_eq(cps.hasPref(null, "value-global"), true);
  do_check_eq(cps.getPref(null, "value-global"), "foo-private-browsing-global");
  do_check_eq(prefObserver.setCalledNum, num + 1);

  
  num = prefObserver.removedCalledNum;
  cps.removePref(uri, "value");
  do_check_eq(cps.hasPref(uri, "value"), true);
  
  do_check_eq(cps.getPref(uri, "value"), "foo"); 
  do_check_eq(prefObserver.removedCalledNum, num + 1);

  num = prefObserver.removedCalledNum;
  cps.removePref(null, "value-global");
  do_check_eq(cps.hasPref(null, "value-global"), true);
  
  do_check_eq(cps.getPref(null, "value-global"), "foo-global") ;
  do_check_eq(prefObserver.removedCalledNum, num + 1);

  
  cps.setPref(uri, "value", "foo-private-browsing");
  cps.removeGroupedPrefs();
  do_check_eq(cps.hasPref(uri, "value"), false);
  do_check_eq(cps.getPref(uri, "value"), undefined);

  cps.setPref(null, "value-global", "foo-private-browsing-global");
  cps.removeGroupedPrefs();
  do_check_eq(cps.hasPref(null, "value-global"), true);
  do_check_eq(cps.getPref(null, "value-global"), "foo-private-browsing-global");

  
  num = prefObserver.removedCalledNum;
  cps.setPref(uri, "value", "foo-private-browsing");
  cps.removePrefsByName("value");
  do_check_eq(cps.hasPref(uri, "value"), false);
  do_check_eq(cps.getPref(uri, "value"), undefined);
  do_check_true(prefObserver.removedCalledNum > num);

  num = prefObserver.removedCalledNum;
  cps.setPref(null, "value-global", "foo-private-browsing");
  cps.removePrefsByName("value-global");
  do_check_eq(cps.hasPref(null, "value-global"), false);
  do_check_eq(cps.getPref(null, "value-global"), undefined);
  do_check_true(prefObserver.removedCalledNum > num);

  
  cps.setPref(uri, "value", "foo-private-browsing");
  do_check_eq(cps.getPrefs(uri).getProperty("value"), "foo-private-browsing");

  cps.setPref(null, "value-global", "foo-private-browsing-global");
  do_check_eq(cps.getPrefs(null).getProperty("value-global"), "foo-private-browsing-global");

  
  do_check_eq(cps.getPrefsByName("value").getProperty(group), "foo-private-browsing");
  do_check_eq(cps.getPrefsByName("value-global").getProperty(null), "foo-private-browsing-global");

  cps.removeObserver("value", prefObserver);
  cps.removeObserver("value-global", prefObserver);
}
