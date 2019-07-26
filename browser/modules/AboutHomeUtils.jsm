



"use strict";

this.EXPORTED_SYMBOLS = [ "AboutHomeUtils" ];

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");


const SNIPPETS_URL_PREF = "browser.aboutHomeSnippets.updateUrl";


const STARTPAGE_VERSION = 4;

this.AboutHomeUtils = {
  get snippetsVersion() STARTPAGE_VERSION,

  







  get showKnowYourRights() {
    
    
    try {
      return !Services.prefs.getBoolPref("browser.rights.override");
    } catch (e) { }
    
    try {
      return !Services.prefs.getBoolPref("browser.EULA.override");
    } catch (e) { }

#ifndef MOZILLA_OFFICIAL
    
    return false;
#endif

    
    var currentVersion = Services.prefs.getIntPref("browser.rights.version");
    try {
      return !Services.prefs.getBoolPref("browser.rights." + currentVersion + ".shown");
    } catch (e) { }

    
    
    try {
      return !Services.prefs.getBoolPref("browser.EULA." + currentVersion + ".accepted");
    } catch (e) { }

    
    return true;
  }
};





XPCOMUtils.defineLazyGetter(AboutHomeUtils, "defaultSearchEngine", function() {
  let defaultEngine = Services.search.originalDefaultEngine;
  let submission = defaultEngine.getSubmission("_searchTerms_", null, "homepage");
  if (submission.postData) {
    throw new Error("Home page does not support POST search engines.");
  }

  return Object.freeze({
    name: defaultEngine.name,
    searchURL: submission.uri.spec
  });
});




XPCOMUtils.defineLazyGetter(AboutHomeUtils, "snippetsURL", function() {
  let updateURL = Services.prefs
                          .getCharPref(SNIPPETS_URL_PREF)
                          .replace("%STARTPAGE_VERSION%", STARTPAGE_VERSION);
  return Services.urlFormatter.formatURL(updateURL);
});
