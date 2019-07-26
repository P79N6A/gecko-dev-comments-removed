



'use strict';

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
