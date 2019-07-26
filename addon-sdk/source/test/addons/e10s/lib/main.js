


'use strict';

const { get: getPref } = require('sdk/preferences/service');
const { getMostRecentBrowserWindow } = require('sdk/window/utils');
const { openTab, closeTab, getBrowserForTab } = require('sdk/tabs/utils');

exports.testRemotePrefIsSet = function(assert) {
  assert.ok(getPref('browser.tabs.remote.autostart'), 
            "Electrolysis remote tabs pref should be set");
}

exports.testTabIsRemote = function(assert, done) {
  const url = 'data:text/html,test-tab-is-remote';
  let tab = openTab(getMostRecentBrowserWindow(), url);
  assert.ok(tab.getAttribute('remote'), "The new tab should be remote");

  
  let mm = getBrowserForTab(tab).messageManager;
  mm.addMessageListener(7, function() {
    closeTab(tab);
    done();
  })
  mm.loadFrameScript('data:,sendAsyncMessage(7)', false);
}

require('sdk/test/runner').runTestsFromModule(module);
