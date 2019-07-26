








Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;

let gLoggingEnabled = false;
let gTestingEnabled = false;
let gUseScanning = true;

let gPrivateAccessToken = '';
let gPrivateAccessTime = 0;

function LOG(aMsg) {
  if (gLoggingEnabled)
  {
    aMsg = "*** WIFI GEO: " + aMsg + "\n";
    Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).logStringMessage(aMsg);
    dump(aMsg);
  }
}

function WifiGeoCoordsObject(lat, lon, acc, alt, altacc) {
  this.latitude = lat;
  this.longitude = lon;
  this.accuracy = acc;
  this.altitude = alt;
  this.altitudeAccuracy = altacc;
}

WifiGeoCoordsObject.prototype = {

  QueryInterface:  XPCOMUtils.generateQI([Ci.nsIDOMGeoPositionCoords]),

  classInfo: XPCOMUtils.generateCI({interfaces: [Ci.nsIDOMGeoPositionCoords],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT,
                                    classDescription: "wifi geo position coords object"}),
};

function WifiGeoPositionObject(lat, lng, acc) {
  this.coords = new WifiGeoCoordsObject(lat, lng, acc, 0, 0);
  this.address = null;
  this.timestamp = Date.now();
}

WifiGeoPositionObject.prototype = {

  QueryInterface:   XPCOMUtils.generateQI([Ci.nsIDOMGeoPosition]),

  
  classInfo: XPCOMUtils.generateCI({interfaces: [Ci.nsIDOMGeoPosition],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT,
                                    classDescription: "wifi geo location position object"}),
};

function privateBrowsingObserver(aSubject, aTopic, aData) {
  gPrivateAccessToken = '';
  gPrivateAccessTime = 0;
}

function WifiGeoPositionProvider() {
  try {
    gLoggingEnabled = Services.prefs.getBoolPref("geo.wifi.logging.enabled");
  } catch (e) {}

  try {
    gTestingEnabled = Services.prefs.getBoolPref("geo.wifi.testing");
  } catch (e) {}

  try {
    gUseScanning = Services.prefs.getBoolPref("geo.wifi.scan");
  } catch (e) {}

  this.wifiService = null;
  this.timer = null;
  this.hasSeenWiFi = false;
  this.started = false;
  this.lastRequestPrivate = false;

  Services.obs.addObserver(privateBrowsingObserver, "last-pb-context-exited", false);
}

WifiGeoPositionProvider.prototype = {
  classID:          Components.ID("{77DA64D3-7458-4920-9491-86CC9914F904}"),
  QueryInterface:   XPCOMUtils.generateQI([Ci.nsIGeolocationProvider,
                                           Ci.nsIWifiListener,
                                           Ci.nsITimerCallback]),
  startup:  function() {
    if (this.started)
      return;
    this.started = true;
    this.hasSeenWiFi = false;

    LOG("startup called.  testing mode is" + gTestingEnabled);

    
    
    
    this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    if (!gTestingEnabled)
      this.timer.initWithCallback(this, 5000, this.timer.TYPE_ONE_SHOT);
    else
      this.timer.initWithCallback(this, 200, this.timer.TYPE_REPEATING_SLACK);
  },

  watch: function(c, requestPrivate) {
    LOG("watch called");

    if (!this.wifiService && gUseScanning) {
      this.wifiService = Cc["@mozilla.org/wifi/monitor;1"].getService(Components.interfaces.nsIWifiMonitor);
      this.wifiService.startWatching(this);
      this.lastRequestPrivate = requestPrivate;
    }
    if (this.hasSeenWiFi) {
      this.hasSeenWiFi = false;
      if (gUseScanning) {
        this.wifiService.stopWatching(this);
        this.wifiService.startWatching(this);
      } else {
        
        this.timer.initWithCallback(this, 5000, this.timer.TYPE_ONE_SHOT);
      }
      this.lastRequestPrivate = requestPrivate;
    }
  },

  shutdown: function() { 
    LOG("shutdown called");
    if(this.wifiService) {
      this.wifiService.stopWatching(this);
      this.wifiService = null;
    }
    if (this.timer != null) {
      this.timer.cancel();
      this.timer = null;
    }

    
    
    
    
    
    if (Services.prefs.getIntPref("network.cookie.lifetimePolicy") != 0)
      Services.prefs.deleteBranch("geo.wifi.access_token.");
    this.started = false;
  },

  setHighAccuracy: function(enable) {
  },

  getAccessTokenForURL: function(url)
  {
    
    let accessToken = "";
    try {
      if (this.lastRequestPrivate) {
        accessToken = gPrivateAccessToken;
      } else {
        let accessTokenPrefName = "geo.wifi.access_token." + url;
        accessToken = Services.prefs.getCharPref(accessTokenPrefName);
      }

      
      let accessTokenDate;
      if (this.lastRequestPrivate) {
        accessTokenDate = gPrivateAccessTime;
      } else {
        accessTokenDate = Services.prefs.getIntPref(accessTokenPrefName + ".time");
      }
      
      let accessTokenInterval = 1209600;  
      try {
        accessTokenInterval = Services.prefs.getIntPref("geo.wifi.access_token.recycle_interval");
      } catch (e) {}
            
      if ((Date.now() / 1000) - accessTokenDate > accessTokenInterval)
        accessToken = "";
    }
    catch (e) {
      accessToken = "";
    }
    return accessToken;
  },

  onChange: function(accessPoints) {
    LOG("onChange called");
    this.hasSeenWiFi = true;

    let providerUrlBase = "https://maps.googleapis.com/maps/api/browserlocation/json";
    try {
        providerUrlBase = Services.prefs.getCharPref("geo.wifi.uri");      
    } catch (x) {};
    let providerUrl;

    let query = providerUrlBase.indexOf("?");
    if (query == -1)
      providerUrl = providerUrlBase + "?"
    else
      providerUrl = providerUrlBase + "&";
    providerUrl = providerUrl + "browser=firefox&sensor=true";
    

    let accessToken = this.getAccessTokenForURL(providerUrlBase);
    if (accessToken !== "")
      providerUrl = providerUrl + "&access_token="+accessToken;

    function sort(a, b) {
      return b.signal - a.signal;
    };

    function encode(ap) {
      
      ap.ssid = ap.ssid.replace("|", "\\|");
      
      return "&wifi=mac:"+ap.mac+"|ssid:"+ap.ssid+"|ss:"+ap.signal;
    };

    if (accessPoints) {
        providerUrl = providerUrl + accessPoints.sort(sort).map(encode).join("");
    }

    providerUrl = encodeURI(providerUrl);

    
    let x = providerUrl.length - 2000;
    if (x >= 0) {
	
	let doomed = providerUrl.lastIndexOf("&", 2000);
	LOG("Doomed:"+doomed);
	providerUrl = providerUrl.substring(0, doomed);
    }
    
    LOG("************************************* Sending request:\n" + providerUrl + "\n");

    
    let xhr = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"]
                        .createInstance(Ci.nsIXMLHttpRequest);

    
    xhr.mozBackgroundRequest = true;
    xhr.open("GET", providerUrl, true);
    xhr.channel.loadFlags = Ci.nsIChannel.LOAD_ANONYMOUS;
    xhr.addEventListener("error", function(req) {
        LOG("onerror: " + req);
    }, false);
    xhr.addEventListener("load", function (req) {  
        LOG("service returned: " + req.target.responseText);
        response = JSON.parse(req.target.responseText);
        











        if (response.status != "OK")
          return;

        if (response.location) {
          let newLocation = new WifiGeoPositionObject(response.location.lat,
                                                      response.location.lng,
                                                      response.accuracy);

          let update = Cc["@mozilla.org/geolocation/service;1"].getService(Ci.nsIGeolocationUpdate);
          update.update(newLocation);
        }

        
        let newAccessToken = response.access_token;
        if (newAccessToken !== undefined)
        {
          let accessToken = "";
          let accessTokenPrefName = "geo.wifi.access_token." + providerUrlBase;
          if (this.lastRequestPrivate) {
            accessTokenPrefName = gPrivateAccessToken;
          } else {
            try { accessToken = Services.prefs.getCharPref(accessTokenPrefName); } catch (e) {}
          }

          if (accessToken != newAccessToken) {
            
            LOG("New Access Token: " + newAccessToken + "\n" + accessTokenPrefName);
            if (this.lastRequestPrivate) {
              gPrivateAccessToken = newAccessToken;
              gPrivateAccessTime = nowInSeconds();
            } else {
              try {
                Services.prefs.setIntPref(accessTokenPrefName + ".time", nowInSeconds());
                Services.prefs.setCharPref(accessTokenPrefName, newAccessToken);
              } catch (x) {
                  
              }
            }
          }
        }
    }, false);

    LOG("************************************* ------>>>> sending.");
    xhr.send(null);
  },

  onError: function (code) {
    LOG("wifi error: " + code);
  },

  notify: function (timer) {
    if (gTestingEnabled || !gUseScanning) {
      
      this.onChange(null);
    }
    else {
      if (!this.hasSeenWiFi)
        this.onChange(null);
      this.timer = null;
    }
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([WifiGeoPositionProvider]);
