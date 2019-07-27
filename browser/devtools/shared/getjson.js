



const {Cu, CC} = require("chrome");
const promise = require("promise");
const {Services} = Cu.import("resource://gre/modules/Services.jsm");

const XMLHttpRequest = CC("@mozilla.org/xmlextras/xmlhttprequest;1");


exports.getJSON = function (prefName, bypassCache) {
  if (!bypassCache) {
    try {
      let str = Services.prefs.getCharPref(prefName + "_cache");
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
      return deferred.reject("Invalid JSON");
    }
    Services.prefs.setCharPref(prefName + "_cache", xhr.responseText);
    deferred.resolve(json);
  }

  xhr.onerror = (e) => {
    deferred.reject("Network error");
  }

  xhr.open("get", Services.prefs.getCharPref(prefName));
  xhr.send();

  return deferred.promise;
}
