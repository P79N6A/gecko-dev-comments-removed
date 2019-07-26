


'use strict';

const { Ci } = require('chrome');
const { merge } = require('sdk/util/object');
const windows = require('sdk/windows').browserWindows;
const tabs = require('sdk/tabs');
const winUtils = require('sdk/window/utils');
const { isWindowPrivate } = winUtils;
const { isPrivateBrowsingSupported } = require('sdk/self');
const { is } = require('sdk/system/xul-app');
const { isPrivate } = require('sdk/private-browsing');
const { getOwnerWindow } = require('sdk/private-browsing/window/utils');
const { LoaderWithHookedConsole } = require("sdk/test/loader");
const { getMode, isGlobalPBSupported,
        isWindowPBSupported, isTabPBSupported } = require('sdk/private-browsing/utils');
const { pb } = require('./private-browsing/helper');


if (isGlobalPBSupported) {
  merge(module.exports, require('./private-browsing/global'));

  exports.testGlobalOnlyOnFirefox = function(test) {
    test.assert(is("Firefox"), "isGlobalPBSupported is only true on Firefox");
  }
}
else if (isWindowPBSupported) {
  merge(module.exports, require('./private-browsing/windows'));

  exports.testPWOnlyOnFirefox = function(test) {
    test.assert(is("Firefox"), "isWindowPBSupported is only true on Firefox");
  }
}

else if (isTabPBSupported) {
  merge(module.exports, require('./private-browsing/tabs'));

  exports.testPTOnlyOnFennec = function(test) {
    test.assert(is("Fennec"), "isTabPBSupported is only true on Fennec");
  }
}

exports.testIsPrivateDefaults = function(test) {
  test.assertEqual(isPrivate(), false, 'undefined is not private');
  test.assertEqual(isPrivate('test'), false, 'strings are not private');
  test.assertEqual(isPrivate({}), false, 'random objects are not private');
  test.assertEqual(isPrivate(4), false, 'numbers are not private');
  test.assertEqual(isPrivate(/abc/), false, 'regex are not private');
  test.assertEqual(isPrivate(function() {}), false, 'functions are not private');
};

exports.testWindowDefaults = function(test) {
  
  let { loader, messages } = LoaderWithHookedConsole(module);
  let windows = loader.require("sdk/windows").browserWindows;
  test.assertEqual(windows.activeWindow.isPrivateBrowsing, false,
                   'window is not private browsing by default');
  test.assertMatches(messages[0].msg, /DEPRECATED.+isPrivateBrowsing/,
                     'isPrivateBrowsing is deprecated');

  let chromeWin = winUtils.getMostRecentBrowserWindow();
  test.assertEqual(getMode(chromeWin), false);
  test.assertEqual(isWindowPrivate(chromeWin), false);
}


exports.testIsActiveDefault = function(test) {
  test.assertEqual(pb.isActive, false,
                   'pb.isActive returns false when private browsing isn\'t supported');
};

exports.testIsPrivateBrowsingFalseDefault = function(test) {
  test.assertEqual(isPrivateBrowsingSupported, false,
  	               'isPrivateBrowsingSupported property is false by default');
};

exports.testGetOwnerWindow = function(test) {
  test.waitUntilDone();

  let window = windows.activeWindow;
  let chromeWindow = getOwnerWindow(window);
  test.assert(chromeWindow instanceof Ci.nsIDOMWindow, 'associated window is found');

  tabs.open({
    url: 'about:blank',
    isPrivate: true,
    onOpen: function(tab) {
      
      if (is('Fennec')) {
        test.assertNotStrictEqual(chromeWindow, getOwnerWindow(tab)); 
        test.assert(getOwnerWindow(tab) instanceof Ci.nsIDOMWindow); 
      }
      else {
        test.assertStrictEqual(chromeWindow, getOwnerWindow(tab), 'associated window is the same for window and window\'s tab');
      }

      
      
      test.assert(!isPrivate(tab));
      test.assert(!isPrivate(getOwnerWindow(tab)));

      tab.close(function() test.done());
    }
  });
};
