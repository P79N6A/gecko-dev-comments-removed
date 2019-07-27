


'use strict';

const { Loader } = require('sdk/test/loader');
const { show, hide } = require('sdk/ui/sidebar/actions');
const { isShowing } = require('sdk/ui/sidebar/utils');
const { getMostRecentBrowserWindow, isWindowPrivate } = require('sdk/window/utils');
const { open, close, focus, promise: windowPromise } = require('sdk/window/helpers');
const { setTimeout } = require('sdk/timers');
const { isPrivate } = require('sdk/private-browsing');
const { data } = require('sdk/self');
const { URL } = require('sdk/url');

const { BUILTIN_SIDEBAR_MENUITEMS, isSidebarShowing,
        getSidebarMenuitems, getExtraSidebarMenuitems, makeID, simulateCommand,
        simulateClick, isChecked } = require('./sidebar/utils');

exports.testSideBarIsInNewPrivateWindows = function(assert, done) {
  const { Sidebar } = require('sdk/ui/sidebar');
  let testName = 'testSideBarIsInNewPrivateWindows';
  let sidebar = Sidebar({
    id: testName,
    title: testName,
    url: 'data:text/html;charset=utf-8,'+testName
  });

  let startWindow = getMostRecentBrowserWindow();
  let ele = startWindow.document.getElementById(makeID(testName));
  assert.ok(ele, 'sidebar element was added');

  open(null, { features: { private: true } }).then(function(window) {
      let ele = window.document.getElementById(makeID(testName));
      assert.ok(isPrivate(window), 'the new window is private');
      assert.ok(!!ele, 'sidebar element was added');

      sidebar.destroy();
      assert.ok(!window.document.getElementById(makeID(testName)), 'sidebar id DNE');
      assert.ok(!startWindow.document.getElementById(makeID(testName)), 'sidebar id DNE');

      return close(window);
  }).then(done).then(null, assert.fail);
}















































exports.testDestroyEdgeCaseBugWithPrivateWindow = function(assert, done) {
  const { Sidebar } = require('sdk/ui/sidebar');
  let testName = 'testDestroyEdgeCaseBug';
  let window = getMostRecentBrowserWindow();
  let sidebar = Sidebar({
    id: testName,
    title: testName,
    url: 'data:text/html;charset=utf-8,'+testName
  });

  
  
  sidebar.show();

  assert.equal(isPrivate(window), false, 'the new window is not private');
  assert.equal(isSidebarShowing(window), true, 'the sidebar is showing');

  

  open(null, { features: { private: true } }).then(focus).then(function(window2) {
    assert.equal(isPrivate(window2), true, 'the new window is private');
    assert.equal(isSidebarShowing(window2), false, 'the sidebar is not showing');
    assert.equal(isShowing(sidebar), false, 'the sidebar is not showing');

    sidebar.destroy();
    assert.pass('destroying the sidebar');

    close(window2).then(function() {
      let loader = Loader(module);

      assert.equal(isPrivate(window), false, 'the current window is not private');

      let sidebar = loader.require('sdk/ui/sidebar').Sidebar({
        id: testName,
        title: testName,
        url:  'data:text/html;charset=utf-8,'+ testName,
        onShow: function() {
          assert.pass('onShow works for Sidebar');
          loader.unload();

          for (let mi of getSidebarMenuitems()) {
            assert.ok(BUILTIN_SIDEBAR_MENUITEMS.indexOf(mi.getAttribute('id')) >= 0, 'the menuitem is for a built-in sidebar')
            assert.ok(!isChecked(mi), 'no sidebar menuitem is checked');
          }
          assert.ok(!window.document.getElementById(makeID(testName)), 'sidebar id DNE');
          assert.equal(isSidebarShowing(window), false, 'the sidebar is not showing');

          done();
        }
      })

      sidebar.show();
      assert.pass('showing the sidebar');
    }).then(null, assert.fail);
  }).then(null, assert.fail);
}

exports.testShowInPrivateWindow = function(assert, done) {
  const { Sidebar } = require('sdk/ui/sidebar');
  let testName = 'testShowInPrivateWindow';
  let window1 = getMostRecentBrowserWindow();
  let url = 'data:text/html;charset=utf-8,'+testName;

  let sidebar1 = Sidebar({
    id: testName,
    title: testName,
    url: url
  });
  let menuitemID = makeID(sidebar1.id);

  assert.equal(sidebar1.url, url, 'url getter works');
  assert.equal(isShowing(sidebar1), false, 'the sidebar is not showing');
  assert.ok(!isChecked(window1.document.getElementById(menuitemID)),
               'the menuitem is not checked');
  assert.equal(isSidebarShowing(window1), false, 'the new window sidebar is not showing');

  windowPromise(window1.OpenBrowserWindow({ private: true }), 'load').then(function(window) {
    let { document } = window;
    assert.equal(isWindowPrivate(window), true, 'new window is private');
    assert.equal(isPrivate(window), true, 'new window is private');

    sidebar1.show().then(
      function good() {
        assert.equal(isShowing(sidebar1), true, 'the sidebar is showing');
        assert.ok(!!document.getElementById(menuitemID),
                  'the menuitem exists on the private window');
        assert.equal(isSidebarShowing(window), true, 'the new window sidebar is showing');

        sidebar1.destroy();
        assert.equal(isSidebarShowing(window), false, 'the new window sidebar is showing');
        assert.ok(!window1.document.getElementById(menuitemID),
                  'the menuitem on the new window dne');

        
        assert.equal(isSidebarShowing(window1), false, 'the old window sidebar is not showing');
        assert.equal(window1.document.getElementById(menuitemID),
                     null,
                     'the menuitem on the old window dne');

        close(window).then(done).then(null, assert.fail);
      },
      function bad() {
        assert.fail('a successful show should not happen here..');
      });
  }).then(null, assert.fail);
}




try {
  require('sdk/ui/sidebar');
}
catch (err) {
  if (!/^Unsupported Application/.test(err.message))
    throw err;

  module.exports = {
    'test Unsupported Application': assert => assert.pass(err.message)
  }
}
