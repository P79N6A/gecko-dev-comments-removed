



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Accounts.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Home", "resource://gre/modules/Home.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS", "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task", "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UITelemetry", "resource://gre/modules/UITelemetry.jsm");


XPCOMUtils.defineLazyGetter(this, "gEncoder", function() { return new gChromeWin.TextEncoder(); });
XPCOMUtils.defineLazyGetter(this, "gDecoder", function() { return new gChromeWin.TextDecoder(); });


const SNIPPETS_UPDATE_URL_PREF = "browser.snippets.updateUrl";


const SNIPPETS_STATS_URL_PREF = "browser.snippets.statsUrl";


const SNIPPETS_GEO_URL_PREF = "browser.snippets.geoUrl";


const SNIPPETS_GEO_LAST_UPDATE_PREF = "browser.snippets.geoLastUpdate";


const SNIPPETS_COUNTRY_CODE_PREF = "browser.snippets.countryCode";


const SNIPPETS_REMOVED_IDS_PREF = "browser.snippets.removedIds";


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
    try {
      let messages = JSON.parse(responseText);
      updateBanner(messages);

      
      cacheSnippets(responseText);
    } catch (e) {
      Cu.reportError("Error parsing snippets responseText: " + e);
    }
  });
}






function cacheSnippets(response) {
  let data = gEncoder.encode(response);
  let promise = OS.File.writeAtomic(gSnippetsPath, data, { tmpPath: gSnippetsPath + ".tmp" });
  promise.then(null, e => Cu.reportError("Error caching snippets: " + e));
}




function loadSnippetsFromCache() {
  let promise = OS.File.read(gSnippetsPath);
  promise.then(array => {
    let messages = JSON.parse(gDecoder.decode(array));
    updateBanner(messages);
  }, e => {
    if (e instanceof OS.File.Error && e.becauseNoSuchFile) {
      Services.console.logStringMessage("Couldn't show snippets because cache does not exist yet.");
    } else {
      Cu.reportError("Error loading snippets from cache: " + e);
    }
  });
}



var gMessageIds = [];












function updateBanner(messages) {
  
  gMessageIds.forEach(function(id) {
    Home.banner.remove(id);
  })
  gMessageIds = [];

  try {
    let removedSnippetIds = JSON.parse(Services.prefs.getCharPref(SNIPPETS_REMOVED_IDS_PREF));
    messages = messages.filter(function(message) {
      
      return removedSnippetIds.indexOf(message.id) === -1;
    });
  } catch (e) {
    
  }

  messages.forEach(function(message) {
    
    if ("target_geo" in message && message.target_geo != gCountryCode) {
      return;
    }
    let id = Home.banner.add({
      text: message.text,
      icon: message.icon,
      weight: message.weight,
      onclick: function() {
        let parentId = gChromeWin.BrowserApp.selectedTab.id;
        gChromeWin.BrowserApp.addTab(message.url, { parentId: parentId });
        UITelemetry.addEvent("action.1", "banner", null, message.id);
      },
      ondismiss: function() {
        
        Home.banner.remove(id);
        removeSnippet(message.id);
        UITelemetry.addEvent("cancel.1", "banner", null, message.id);
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






function removeSnippet(snippetId) {
  let removedSnippetIds;
  try {
    removedSnippetIds = JSON.parse(Services.prefs.getCharPref(SNIPPETS_REMOVED_IDS_PREF));
  } catch (e) {
    removedSnippetIds = [];
  }

  removedSnippetIds.push(snippetId);
  Services.prefs.setCharPref(SNIPPETS_REMOVED_IDS_PREF, JSON.stringify(removedSnippetIds));
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

function loadSyncPromoBanner() {
  Accounts.anySyncAccountsExist().then(
    (exist) => {
      
      if (exist) {
        return;
      }

      let stringBundle = Services.strings.createBundle("chrome://browser/locale/sync.properties");
      let text = stringBundle.GetStringFromName("promoBanner.message.text");
      let link = stringBundle.GetStringFromName("promoBanner.message.link");

      let id = Home.banner.add({
        text: text + "<a href=\"#\">" + link + "</a>",
        icon: "drawable://sync_promo",
        onclick: function() {
          
          Home.banner.remove(id);
          Accounts.launchSetup();

          UITelemetry.addEvent("action.1", "banner", null, "syncpromo");
        },
        ondismiss: function() {
          
          Home.banner.remove(id);
          Services.prefs.setBoolPref("browser.snippets.syncPromo.enabled", false);

          UITelemetry.addEvent("cancel.1", "banner", null, "syncpromo");
        }
      });
    },
    (err) => {
      Cu.reportError("Error checking whether sync account exists: " + err);
    }
  );
}

function loadHomePanelsBanner() {
  let stringBundle = Services.strings.createBundle("chrome://browser/locale/aboutHome.properties");
  let text = stringBundle.GetStringFromName("banner.firstrunHomepage.text");

  let id = Home.banner.add({
    text: text,
    icon: "drawable://homepage_banner_firstrun",
    onclick: function() {
      
      Home.banner.remove(id);
      
      Services.prefs.setBoolPref("browser.snippets.firstrunHomepage.enabled", false);

      UITelemetry.addEvent("action.1", "banner", null, "firstrun-homepage");
    },
    ondismiss: function() {
      Home.banner.remove(id);
      Services.prefs.setBoolPref("browser.snippets.firstrunHomepage.enabled", false);

      UITelemetry.addEvent("cancel.1", "banner", null, "firstrun-homepage");
    }
  });
}

function Snippets() {}

Snippets.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsITimerCallback]),
  classID: Components.ID("{a78d7e59-b558-4321-a3d6-dffe2f1e76dd}"),

  observe: function(subject, topic, data) {
    switch(topic) {
      case "browser-delayed-startup-finished":
        if (Services.prefs.getBoolPref("browser.snippets.firstrunHomepage.enabled")) {
          loadHomePanelsBanner();
        } else if (Services.prefs.getBoolPref("browser.snippets.syncPromo.enabled")) {
          loadSyncPromoBanner();
        }

        if (Services.prefs.getBoolPref("browser.snippets.enabled")) {
          loadSnippetsFromCache();
        }
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
