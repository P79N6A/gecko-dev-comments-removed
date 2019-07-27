




const Ci = Components.interfaces;
Components.utils.import("resource://gre/modules/Services.jsm");




function MockPlugin(name, version, enabledState) {
  this.name = name;
  this.version = version;
  this.enabledState = enabledState;
}

MockPlugin.prototype = {
  get blocklisted() {
    let bls = Services.blocklist;
    return bls.getPluginBlocklistState(this) == bls.STATE_BLOCKED;
  },

  get disabled() {
    return this.enabledState == Ci.nsIPluginTag.STATE_DISABLED;
  }
};


const PLUGINS = [
  new MockPlugin('test_with_infoURL', '5', Ci.nsIPluginTag.STATE_ENABLED),
  new MockPlugin('test_with_altInfoURL', '5', Ci.nsIPluginTag.STATE_ENABLED),
  new MockPlugin('test_no_infoURL', '5', Ci.nsIPluginTag.STATE_ENABLED),
  new MockPlugin('test_newVersion', '1', Ci.nsIPluginTag.STATE_ENABLED),
  new MockPlugin('test_newVersion', '3', Ci.nsIPluginTag.STATE_ENABLED)
];





function run_test() {
  copyBlocklistToProfile(do_get_file('data/pluginInfoURL_block.xml'));

  createAppInfo('xpcshell@tests.mozilla.org', 'XPCShell', '3', '8');
  startupManager();

  run_next_test();
}





add_task(function* test_infoURL() {
  
  
  let testInfoURL = 'http://test.url.com/';

  Assert.strictEqual(Services.blocklist.getPluginInfoURL(PLUGINS[0]),
    testInfoURL, 'Should be the provided url when an infoURL tag is available');
});





add_task(function* test_altInfoURL() {
  let altTestInfoURL = 'http://alt.test.url.com/';

  Assert.strictEqual(Services.blocklist.getPluginInfoURL(PLUGINS[1]),
    altTestInfoURL, 'Should be the alternative infoURL');
});





add_task(function* test_infoURL_missing() {
  Assert.strictEqual(Services.blocklist.getPluginInfoURL(PLUGINS[2]), null,
    'Should be null when no infoURL tag is available.');
});

add_task(function* test_intoURL_newVersion() {
  let testInfoURL = 'http://test.url2.com/';
  Assert.strictEqual(Services.blocklist.getPluginInfoURL(PLUGINS[3]),
    testInfoURL, 'Old plugin should match');
  Assert.strictEqual(Services.blocklist.getPluginInfoURL(PLUGINS[4]),
    null, 'New plugin should not match');
});
