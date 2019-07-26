



'use strict';

const kServerURL = 'http://localhost:4444';
const kCanonicalSitePath = '/canonicalSite.html';
const kPrefsCanonicalURL = 'captivedetect.canonicalURL';
const kPrefsMaxWaitingTime = 'captivedetect.maxWaitingTime';
const kPrefsPollingTime = 'captivedetect.pollingTime';

function setupPrefs() {
  let prefs = Components.classes["@mozilla.org/preferences-service;1"]
                .getService(Components.interfaces.nsIPrefService)
                .QueryInterface(Components.interfaces.nsIPrefBranch);
  prefs.setCharPref(kPrefsCanonicalURL, kServerURL + kCanonicalSitePath);
  prefs.setIntPref(kPrefsMaxWaitingTime, 0);
  prefs.setIntPref(kPrefsPollingTime, 1);
}

setupPrefs();
