



const {Cu, CC} = require("chrome");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const {Services} = Cu.import("resource://gre/modules/Services.jsm");

const XMLHttpRequest = CC("@mozilla.org/xmlextras/xmlhttprequest;1");

function getJSON(bypassCache, pref) {
  if (!bypassCache) {
    try {
      let str = Services.prefs.getCharPref(pref + "_cache");
      let json = JSON.parse(str);
      return promise.resolve(json);
    } catch(e) {}
  }


  let deferred = promise.defer();

  let xhr = new XMLHttpRequest();

  xhr.onload = () => {
    let json;
    try {
      json = JSON.parse(xhr.responseText);
    } catch(e) {
      return deferred.reject("Not valid JSON");
    }
    Services.prefs.setCharPref(pref + "_cache", xhr.responseText);
    deferred.resolve(json);
  }

  xhr.onerror = (e) => {
    deferred.reject("Network error");
  }

  xhr.open("get", Services.prefs.getCharPref(pref));
  xhr.send();

  return deferred.promise;
}



exports.GetTemplatesJSON = function(bypassCache) {
  return getJSON(bypassCache, "devtools.webide.templatesURL");
}

exports.GetAddonsJSON = function(bypassCache) {
  return getJSON(bypassCache, "devtools.webide.addonsURL");
}
