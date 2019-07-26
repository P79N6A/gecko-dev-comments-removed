



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Home", "resource://gre/modules/Home.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS", "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task", "resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyGetter(this, "gEncoder", function() { return new gChromeWin.TextEncoder(); });
XPCOMUtils.defineLazyGetter(this, "gDecoder", function() { return new gChromeWin.TextDecoder(); });


const SNIPPETS_UPDATE_URL_PREF = "browser.snippets.updateUrl";


const SNIPPETS_STATS_URL_PREF = "browser.snippets.statsUrl";


const SNIPPETS_GEO_URL_PREF = "browser.snippets.geoUrl";


const SNIPPETS_GEO_LAST_UPDATE_PREF = "browser.snippets.geoLastUpdate";


const SNIPPETS_COUNTRY_CODE_PREF = "browser.snippets.countryCode";


const SNIPPETS_GEO_UPDATE_INTERVAL_MS = 86400000*30;


const SNIPPETS_VERSION = 1;

XPCOMUtils.defineLazyGetter(this, "gSnippetsURL", function() {
  let updateURL = Services.prefs.getCharPref(SNIPPETS_UPDATE_URL_PREF).replace("%SNIPPETS_VERSION%", SNIPPETS_VERSION);
  return Services.urlFormatter.formatURL(updateURL);
});


XPCOMUtils.defineLazyGetter(this, "gSnippetsPath", function() {
  return OS.Path.join(OS.Constants.Path.profileDir, "snippets.json");
});

XPCOMUtils.defineLazyGetter(this, "gStatsURL", function() {
  return Services.prefs.getCharPref(SNIPPETS_STATS_URL_PREF);
});


XPCOMUtils.defineLazyGetter(this, "gStatsPath", function() {
  return OS.Path.join(OS.Constants.Path.profileDir, "snippets-stats.txt");
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
  let data = gEncoder.encode(response);
  let promise = OS.File.writeAtomic(gSnippetsPath, data, { tmpPath: gSnippetsPath + ".tmp" });
  promise.then(null, e => Cu.reportError("Error caching snippets: " + e));
}




function loadSnippetsFromCache() {
  let promise = OS.File.read(gSnippetsPath);
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
        
        if (Math.random() < .1) {
          writeStat(message.id, new Date().toISOString());
        }
      }
    });
    
    gMessageIds.push(id);
  });
}







function writeStat(snippetId, timestamp) {
  let data = gEncoder.encode(snippetId + "," + timestamp + ";");

  Task.spawn(function() {
    try {
      let file = yield OS.File.open(gStatsPath, { append: true, write: true });
      try {
        yield file.write(data);
      } finally {
        yield file.close();
      }
    } catch (ex if ex instanceof OS.File.Error && ex.becauseNoSuchFile) {
      
      yield OS.File.writeAtomic(gStatsPath, data, { tmpPath: gStatsPath + ".tmp" });
    }
  }).then(null, e => Cu.reportError("Error writing snippets stats: " + e));
}




function sendStats() {
  let promise = OS.File.read(gStatsPath);
  promise.then(array => sendStatsRequest(gDecoder.decode(array)), e => {
    if (e instanceof OS.File.Error && e.becauseNoSuchFile) {
      
    } else {
      Cu.reportError("Error eading snippets stats: " + e);
    }
  });
}








function sendStatsRequest(data) {
  let params = [];
  let stats = data.split(";");

  
  for (let i = 0; i < stats.length - 1; i++) {
    let stat = stats[i].split(",");
    params.push("s" + i + "=" + encodeURIComponent(stat[0]));
    params.push("t" + i + "=" + encodeURIComponent(stat[1]));
  }

  let url = gStatsURL + "?" + params.join("&");

  
  _httpGetRequest(url, removeStats);
}




function removeStats() {
  let promise = OS.File.remove(gStatsPath);
  promise.then(null, e => Cu.reportError("Error removing snippets stats: " + e));
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
    if (!Services.prefs.getBoolPref("browser.snippets.enabled")) {
      return;
    }
    switch(topic) {
      case "profile-after-change":
        loadSnippetsFromCache();
        break;
    }
  },

  
  notify: function(timer) {
    if (!Services.prefs.getBoolPref("browser.snippets.enabled")) {
      return;
    }
    update();
    sendStats();
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([Snippets]);
