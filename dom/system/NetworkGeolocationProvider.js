









































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;

let gLoggingEnabled = false;
let gTestingEnabled = false;

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

function WifiGeoPositionProvider() {
  try {
    gLoggingEnabled = Services.prefs.getBoolPref("geo.wifi.logging.enabled");
  } catch (e) {}

  try {
    gTestingEnabled = Services.prefs.getBoolPref("geo.wifi.testing");
  } catch (e) {}

  wifiService = null;
  timer = null;
  hasSeenWiFi = false;
  started = false;
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

  watch: function(c) {
    LOG("watch called");
    if (!this.wifiService) {
      this.wifiService = Cc["@mozilla.org/wifi/monitor;1"].getService(Components.interfaces.nsIWifiMonitor);
      this.wifiService.startWatching(this);
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

  getAccessTokenForURL: function(url)
  {
    
    let accessToken = "";
    try {
      let accessTokenPrefName = "geo.wifi.access_token." + url;
      accessToken = Services.prefs.getCharPref(accessTokenPrefName);

      
      let accessTokenDate = Services.prefs.getIntPref(accessTokenPrefName + ".time");
      
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
      providerUrl = providerUrl + "&access_token="+access_token;

    function sort(a, b) {
      return b.signal - a.signal;
    };

    function encode(ap) {
      
      ap.ssid = ap.ssid.replace("|", "\\|");
      
      return "&wifi=mac:"+ap.mac+"|ssid:"+ap.ssid+"|ss:"+ap.signal;
    };

    if (accessPoints) {
        accessPoints.sort(sort).map(encode).join("");
        
        let x = providerUrl.length - 2000;
        if (x >= 0) {
            
            let doomed = providerUrl.lastIndexOf("&", 2000);
            LOG("Doomed:"+doomed);
            providerUrl = providerUrl.substring(0, doomed);
        }
    }

    providerUrl = encodeURI(providerUrl);
    LOG("************************************* Sending request:\n" + providerUrl + "\n");

    
    let xhr = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"]
                        .createInstance(Ci.nsIXMLHttpRequest);

    
    xhr.mozBackgroundRequest = true;
    xhr.open("GET", providerUrl, false);
    xhr.channel.loadFlags = Ci.nsIChannel.LOAD_ANONYMOUS;
    xhr.onerror = function(req) {
        LOG("onerror: " + req);
    };
    xhr.onload = function (req) {  
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
          try { accessToken = Services.prefs.getCharPref(accessTokenPrefName); } catch (e) {}

          if (accessToken != newAccessToken) {
            
              LOG("New Access Token: " + newAccessToken + "\n" + accessTokenPrefName);
              try {
                Services.prefs.setIntPref(accessTokenPrefName + ".time", nowInSeconds());
                Services.prefs.setCharPref(accessTokenPrefName, newAccessToken);
              } catch (x) {
                  
              }
          }
        }
    };

    LOG("************************************* ------>>>> sending.");
    xhr.send(null);
  },

  onError: function (code) {
    LOG("wifi error: " + code);
  },

  notify: function (timer) {
    if (gTestingEnabled) {
      
      this.onChange(null);
    }
    else {
      if (!this.hasSeenWiFi)
        this.onChange(null);
      this.timer = null;
    }
  },
};

let NSGetFactory = XPCOMUtils.generateNSGetFactory([WifiGeoPositionProvider]);
