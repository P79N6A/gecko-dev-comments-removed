



'use strict';

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://testing-common/httpd.js');

XPCOMUtils.defineLazyServiceGetter(this, 'gCaptivePortalDetector',
                                   '@mozilla.org/toolkit/captive-detector;1',
                                   'nsICaptivePortalDetector');

const kServerURL = 'http://localhost:4444';
const kCanonicalSitePath = '/canonicalSite.html';
const kCanonicalSiteContent = 'true';
const kPrefsCanonicalURL = 'captivedetect.canonicalURL';
const kPrefsCanonicalContent = 'captivedetect.canonicalContent';
const kPrefsMaxWaitingTime = 'captivedetect.maxWaitingTime';
const kPrefsPollingTime = 'captivedetect.pollingTime';

function setupPrefs() {
  let prefs = Components.classes["@mozilla.org/preferences-service;1"]
                .getService(Components.interfaces.nsIPrefService)
                .QueryInterface(Components.interfaces.nsIPrefBranch);
  prefs.setCharPref(kPrefsCanonicalURL, kServerURL + kCanonicalSitePath);
  prefs.setCharPref(kPrefsCanonicalContent, kCanonicalSiteContent);
  prefs.setIntPref(kPrefsMaxWaitingTime, 0);
  prefs.setIntPref(kPrefsPollingTime, 1);
}

setupPrefs();
