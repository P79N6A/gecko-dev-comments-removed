



"use strict";

const EXPORTED_SYMBOLS = [ "AboutHomeUtils" ];

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");


const SNIPPETS_URL_PREF = "browser.aboutHomeSnippets.updateUrl";


const STARTPAGE_VERSION = 3;

let AboutHomeUtils = new Object();





XPCOMUtils.defineLazyGetter(AboutHomeUtils, "defaultSearchEngine", function() {
  let defaultEngine = Services.search.originalDefaultEngine;
  let submission = defaultEngine.getSubmission("_searchTerms_");
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
