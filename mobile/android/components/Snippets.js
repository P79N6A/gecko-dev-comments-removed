



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Home", "resource://gre/modules/Home.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS", "resource://gre/modules/osfile.jsm");

XPCOMUtils.defineLazyGetter(this, "gEncoder", function() { return new gChromeWin.TextEncoder(); });
XPCOMUtils.defineLazyGetter(this, "gDecoder", function() { return new gChromeWin.TextDecoder(); });

const SNIPPETS_ENABLED = Services.prefs.getBoolPref("browser.snippets.enabled");


const SNIPPETS_UPDATE_URL_PREF = "browser.snippets.updateUrl";


const SNIPPETS_GEO_URL_PREF = "browser.snippets.geoUrl";


const SNIPPETS_GEO_LAST_UPDATE_PREF = "browser.snippets.geoLastUpdate";


const SNIPPETS_COUNTRY_CODE_PREF = "browser.snippets.countryCode";


const SNIPPETS_GEO_UPDATE_INTERVAL_MS = 86400000*30;


const SNIPPETS_VERSION = 1;

XPCOMUtils.defineLazyGetter(this, "gSnippetsURL", function() {
  let updateURL = Services.prefs.getCharPref(SNIPPETS_UPDATE_URL_PREF).replace("%SNIPPETS_VERSION%", SNIPPETS_VERSION);
  return Services.urlFormatter.formatURL(updateURL);
});

XPCOMUtils.defineLazyGetter(this, "gGeoURL", function() {
  return Services.prefs.getCharPref(SNIPPETS_GEO_URL_PREF);
});

XPCOMUtils.defineLazyGetter(this, "gCountryCode", function() {
  try {
    return Services.prefs.getCharPref(SNIPPETS_COUNTRY_CODE_PREF);
  } catch (e) {
    
    return "";
  }
});

XPCOMUtils.defineLazyGetter(this, "gChromeWin", function() {
  return Services.wm.getMostRecentWindow("navigator:browser");
});




function update() {
  
  let lastUpdate = 0;
  try {
    lastUpdate = parseFloat(Services.prefs.getCharPref(SNIPPETS_GEO_LAST_UPDATE_PREF));
  } catch (e) {}

  if (Date.now() - lastUpdate > SNIPPETS_GEO_UPDATE_INTERVAL_MS) {
    
    
    updateCountryCode(updateSnippets);
  } else {
    updateSnippets();
  }
}






function updateCountryCode(callback) {
  _httpGetRequest(gGeoURL, function(responseText) {
    
    let data = JSON.parse(responseText);
    Services.prefs.setCharPref(SNIPPETS_COUNTRY_CODE_PREF, data.country_code);

    
    Services.prefs.setCharPref(SNIPPETS_GEO_LAST_UPDATE_PREF, Date.now());

    callback();
  });
}





function updateSnippets() {
  _httpGetRequest(gSnippetsURL, function(responseText) {
    cacheSnippets(responseText);
    updateBanner(responseText);
  });
}






function cacheSnippets(response) {
  let path = OS.Path.join(OS.Constants.Path.profileDir, "snippets.json");
  let data = gEncoder.encode(response);
  let promise = OS.File.writeAtomic(path, data, { tmpPath: path + ".tmp" });
  promise.then(null, e => Cu.reportError("Error caching snippets: " + e));
}




function loadSnippetsFromCache() {
  let path = OS.Path.join(OS.Constants.Path.profileDir, "snippets.json");
  let promise = OS.File.read(path);
  promise.then(array => updateBanner(gDecoder.decode(array)), e => {
    
    if (e instanceof OS.File.Error && e.becauseNoSuchFile) {
      update();
    } else {
      Cu.reportError("Error loading snippets from cache: " + e);
    }
  });
}



var gMessageIds = [];













function updateBanner(response) {
  
  gMessageIds.forEach(function(id) {
    Home.banner.remove(id);
  })
  gMessageIds = [];

  let messages = JSON.parse(response);
  messages.forEach(function(message) {
    
    if ("target_geo" in message && message.target_geo != gCountryCode) {
      return;
    }
    let id = Home.banner.add({
      text: message.text,
      icon: message.icon,
      onclick: function() {
        gChromeWin.BrowserApp.addTab(message.url);
      },
      onshown: function() {
        
      }
    });
    
    gMessageIds.push(id);
  });
}







function _httpGetRequest(url, callback) {
  let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Ci.nsIXMLHttpRequest);
  try {
    xhr.open("GET", url, true);
  } catch (e) {
    Cu.reportError("Error opening request to " + url + ": " + e);
    return;
  }
  xhr.onerror = function onerror(e) {
    Cu.reportError("Error making request to " + url + ": " + e.error);
  }
  xhr.onload = function onload(event) {
    if (xhr.status !== 200) {
      Cu.reportError("Request to " + url + " returned status " + xhr.status);
      return;
    }
    if (callback) {
      callback(xhr.responseText);
    }
  }
  xhr.send(null);
}

function Snippets() {}

Snippets.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsITimerCallback]),
  classID: Components.ID("{a78d7e59-b558-4321-a3d6-dffe2f1e76dd}"),

  observe: function(subject, topic, data) {
    if (!SNIPPETS_ENABLED) {
      return;
    }
    switch(topic) {
      case "profile-after-change":
        loadSnippetsFromCache();
        break;
    }
  },

  
  notify: function(timer) {
    if (!SNIPPETS_ENABLED) {
      return;
    }
    update();
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([Snippets]);
