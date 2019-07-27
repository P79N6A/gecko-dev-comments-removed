



Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;

const POSITION_UNAVAILABLE = Ci.nsIDOMGeoPositionError.POSITION_UNAVAILABLE;
const SETTINGS_DEBUG_ENABLED = "geolocation.debugging.enabled";
const SETTINGS_CHANGED_TOPIC = "mozsettings-changed";
const SETTINGS_WIFI_ENABLED = "wifi.enabled";

let gLoggingEnabled = false;












let gLocationRequestTimeout = 5000;

let gWifiScanningEnabled = true;
let gCellScanningEnabled = false;

function LOG(aMsg) {
  if (gLoggingEnabled) {
    aMsg = "*** WIFI GEO: " + aMsg + "\n";
    Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).logStringMessage(aMsg);
    dump(aMsg);
  }
}

function CachedRequest(loc, cellInfo, wifiList) {
  this.location = loc;

  let wifis = new Set();
  if (wifiList) {
    for (let i = 0; i < wifiList.length; i++) {
      wifis.add(wifiList[i].macAddress);
    }
  }

  
  
  function makeCellKey(cell) {
    return "" + cell.radio + ":" + cell.mobileCountryCode + ":" +
    cell.mobileNetworkCode + ":" + cell.locationAreaCode + ":" +
    cell.cellId;
  }

  let cells = new Set();
  if (cellInfo) {
    for (let i = 0; i < cellInfo.length; i++) {
      cells.add(makeCellKey(cellInfo[i]));
    }
  }

  this.hasCells = () => cells.size > 0;

  this.hasWifis = () => wifis.size > 0;

  
  this.isCellEqual = function(cellInfo) {
    if (!this.hasCells()) {
      return false;
    }

    let len1 = cells.size;
    let len2 = cellInfo.length;

    if (len1 != len2) {
      LOG("cells not equal len");
      return false;
    }

    for (let i = 0; i < len2; i++) {
      if (!cells.has(makeCellKey(cellInfo[i]))) {
        return false;
      }
    }
    return true;
  };

  
  this.isWifiApproxEqual = function(wifiList) {
    if (!this.hasWifis()) {
      return false;
    }

    
    let common = 0;
    for (let i = 0; i < wifiList.length; i++) {
      if (wifis.has(wifiList[i].macAddress)) {
        common++;
      }
    }
    let kPercentMatch = 0.5;
    return common >= (Math.max(wifis.size, wifiList.length) * kPercentMatch);
  };

  this.isGeoip = function() {
    return !this.hasCells() && !this.hasWifis();
  };

  this.isCellAndWifi = function() {
    return this.hasCells() && this.hasWifis();
  };

  this.isCellOnly = function() {
    return this.hasCells() && !this.hasWifis();
  };

  this.isWifiOnly = function() {
    return this.hasWifis() && !this.hasCells();
  };
 }

let gCachedRequest = null;
let gDebugCacheReasoning = ""; 










function isCachedRequestMoreAccurateThanServerRequest(newCell, newWifiList)
{
  gDebugCacheReasoning = "";
  let isNetworkRequestCacheEnabled = true;
  try {
    
    isNetworkRequestCacheEnabled = Services.prefs.getBoolPref("geo.wifi.debug.requestCache.enabled");
    if (!isNetworkRequestCacheEnabled) {
      gCachedRequest = null;
    }
  } catch (e) {}

  if (!gCachedRequest || !isNetworkRequestCacheEnabled) {
    gDebugCacheReasoning = "No cached data";
    return false;
  }

  if (!newCell && !newWifiList) {
    gDebugCacheReasoning = "New req. is GeoIP.";
    return true;
  }

  if (newCell && newWifiList && (gCachedRequest.isCellOnly() || gCachedRequest.isWifiOnly())) {
    gDebugCacheReasoning = "New req. is cell+wifi, cache only cell or wifi.";
    return false;
  }

  if (newCell && gCachedRequest.isWifiOnly()) {
    
    
    
    var isHighAccuracyWifi = gCachedRequest.location.coords.accuracy < 5000;
    gDebugCacheReasoning = "Req. is cell, cache is wifi, isHigh:" + isHighAccuracyWifi;
    return isHighAccuracyWifi;
  }

  let hasEqualCells = false;
  if (newCell) {
    hasEqualCells = gCachedRequest.isCellEqual(newCell);
  }

  let hasEqualWifis = false;
  if (newWifiList) {
    hasEqualWifis = gCachedRequest.isWifiApproxEqual(newWifiList);
  }

  gDebugCacheReasoning = "EqualCells:" + hasEqualCells + " EqualWifis:" + hasEqualWifis;

  if (gCachedRequest.isCellOnly()) {
    gDebugCacheReasoning += ", Cell only.";
    if (hasEqualCells) {
      return true;
    }
  } else if (gCachedRequest.isWifiOnly() && hasEqualWifis) {
    gDebugCacheReasoning +=", Wifi only."
    return true;
  } else if (gCachedRequest.isCellAndWifi()) {
     gDebugCacheReasoning += ", Cache has Cell+Wifi.";
    if ((hasEqualCells && hasEqualWifis) ||
        (!newWifiList && hasEqualCells) ||
        (!newCell && hasEqualWifis))
    {
     return true;
    }
  }

  return false;
}

function WifiGeoCoordsObject(lat, lon, acc, alt, altacc) {
  this.latitude = lat;
  this.longitude = lon;
  this.accuracy = acc;
  this.altitude = alt;
  this.altitudeAccuracy = altacc;
}

WifiGeoCoordsObject.prototype = {
  QueryInterface:  XPCOMUtils.generateQI([Ci.nsIDOMGeoPositionCoords])
};

function WifiGeoPositionObject(lat, lng, acc) {
  this.coords = new WifiGeoCoordsObject(lat, lng, acc, 0, 0);
  this.address = null;
  this.timestamp = Date.now();
}

WifiGeoPositionObject.prototype = {
  QueryInterface:   XPCOMUtils.generateQI([Ci.nsIDOMGeoPosition])
};

function WifiGeoPositionProvider() {
  try {
    gLoggingEnabled = Services.prefs.getBoolPref("geo.wifi.logging.enabled");
  } catch (e) {}

  try {
    gLocationRequestTimeout = Services.prefs.getIntPref("geo.wifi.timeToWaitBeforeSending");
  } catch (e) {}

  try {
    gWifiScanningEnabled = Services.prefs.getBoolPref("geo.wifi.scan");
  } catch (e) {}

  try {
    gCellScanningEnabled = Services.prefs.getBoolPref("geo.cell.scan");
  } catch (e) {}

  this.wifiService = null;
  this.timer = null;
  this.started = false;
}

WifiGeoPositionProvider.prototype = {
  classID:          Components.ID("{77DA64D3-7458-4920-9491-86CC9914F904}"),
  QueryInterface:   XPCOMUtils.generateQI([Ci.nsIGeolocationProvider,
                                           Ci.nsIWifiListener,
                                           Ci.nsITimerCallback,
                                           Ci.nsIObserver]),
  listener: null,

  observe: function(aSubject, aTopic, aData) {
    if (aTopic != SETTINGS_CHANGED_TOPIC) {
      return;
    }

    try {
      let setting = JSON.parse(aData);
      if (setting.key == SETTINGS_DEBUG_ENABLED) {
        gLoggingEnabled = setting.value;
      } else if (setting.key == SETTINGS_WIFI_ENABLED) {
        gWifiScanningEnabled = setting.value;
      }
    } catch (e) {
    }
  },

  resetTimer: function() {
    if (this.timer) {
      this.timer.cancel();
      this.timer = null;
    }
    
    this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this.timer.initWithCallback(this,
                                gLocationRequestTimeout,
                                this.timer.TYPE_REPEATING_SLACK);
  },

  startup:  function() {
    if (this.started)
      return;

    this.started = true;
    let self = this;
    let settingsCallback = {
      handle: function(name, result) {
        
        
        if (name == SETTINGS_DEBUG_ENABLED && !gLoggingEnabled) {
          gLoggingEnabled = result;
        } else if (name == SETTINGS_WIFI_ENABLED) {
          gWifiScanningEnabled = result;
          if (self.wifiService) {
            self.wifiService.stopWatching(self);
          }
          if (gWifiScanningEnabled) {
            self.wifiService = Cc["@mozilla.org/wifi/monitor;1"].getService(Ci.nsIWifiMonitor);
            self.wifiService.startWatching(self);
          }
        }
      },

      handleError: function(message) {
        gLoggingEnabled = false;
        LOG("settings callback threw an exception, dropping");
      }
    };

    try {
      Services.obs.addObserver(this, SETTINGS_CHANGED_TOPIC, false);
      let settings = Cc["@mozilla.org/settingsService;1"].getService(Ci.nsISettingsService);
      settings.createLock().get(SETTINGS_WIFI_ENABLED, settingsCallback);
      settings.createLock().get(SETTINGS_DEBUG_ENABLED, settingsCallback);
    } catch(ex) {
      
    }

    if (gWifiScanningEnabled && Cc["@mozilla.org/wifi/monitor;1"]) {
      if (this.wifiService) {
        this.wifiService.stopWatching(this);
      }
      this.wifiService = Cc["@mozilla.org/wifi/monitor;1"].getService(Ci.nsIWifiMonitor);
      this.wifiService.startWatching(this);
    }

    this.resetTimer();
    LOG("startup called.");
  },

  watch: function(c) {
    this.listener = c;
  },

  shutdown: function() {
    LOG("shutdown called");
    if (this.started == false) {
      return;
    }

    
    
    gCachedRequest = null;

    if (this.timer) {
      this.timer.cancel();
      this.timer = null;
    }

    if(this.wifiService) {
      this.wifiService.stopWatching(this);
      this.wifiService = null;
    }

    Services.obs.removeObserver(this, SETTINGS_CHANGED_TOPIC);

    this.listener = null;
    this.started = false;
  },

  setHighAccuracy: function(enable) {
  },

  onChange: function(accessPoints) {

    
    this.resetTimer();

    function isPublic(ap) {
      let mask = "_nomap"
      let result = ap.ssid.indexOf(mask, ap.ssid.length - mask.length);
      if (result != -1) {
        LOG("Filtering out " + ap.ssid + " " + result);
      }
      return result;
    };

    function sort(a, b) {
      return b.signal - a.signal;
    };

    function encode(ap) {
      return { 'macAddress': ap.mac, 'signalStrength': ap.signal };
    };

    let wifiData = null;
    if (accessPoints) {
      wifiData = accessPoints.filter(isPublic).sort(sort).map(encode);
    }
    this.sendLocationRequest(wifiData);
  },

  onError: function (code) {
    LOG("wifi error: " + code);
    this.sendLocationRequest(null);
  },

  getMobileInfo: function() {
    LOG("getMobileInfo called");
    try {
      let radioService = Cc["@mozilla.org/ril;1"]
                    .getService(Ci.nsIRadioInterfaceLayer);
      let numInterfaces = radioService.numRadioInterfaces;
      let result = [];
      for (let i = 0; i < numInterfaces; i++) {
        LOG("Looking for SIM in slot:" + i + " of " + numInterfaces);
        let radio = radioService.getRadioInterface(i);
        let iccInfo = radio.rilContext.iccInfo;
        let cell = radio.rilContext.voice.cell;
        let type = radio.rilContext.voice.type;

        if (iccInfo && cell && type) {
          if (type === "gsm" || type === "gprs" || type === "edge") {
            type = "gsm";
          } else {
            type = "wcdma";
          }
          result.push({ radio: type,
                      mobileCountryCode: iccInfo.mcc,
                      mobileNetworkCode: iccInfo.mnc,
                      locationAreaCode: cell.gsmLocationAreaCode,
                      cellId: cell.gsmCellId });
        }
      }
      return result;
    } catch (e) {
      return null;
    }
  },

  notify: function (timer) {
    this.sendLocationRequest(null);
  },

  sendLocationRequest: function (wifiData) {
    let data = {};
    if (wifiData) {
      data.wifiAccessPoints = wifiData;
    }

    if (gCellScanningEnabled) {
      let cellData = this.getMobileInfo();
      if (cellData && cellData.length > 0) {
        data.cellTowers = cellData;
      }
    }

    let useCached = isCachedRequestMoreAccurateThanServerRequest(data.cellTowers,
                                                                 data.wifiAccessPoints);

    LOG("Use request cache:" + useCached + " reason:" + gDebugCacheReasoning);

    if (useCached) {
      gCachedRequest.location.timestamp = Date.now();
      this.listener.update(gCachedRequest.location);
      return;
    }

    
    let url = Services.urlFormatter.formatURLPref("geo.wifi.uri");
    let listener = this.listener;
    LOG("Sending request: " + url + "\n");

    let xhr = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"]
                        .createInstance(Ci.nsIXMLHttpRequest);

    listener.locationUpdatePending();

    try {
      xhr.open("POST", url, true);
    } catch (e) {
      listener.notifyError(POSITION_UNAVAILABLE);
      return;
    }
    xhr.setRequestHeader("Content-Type", "application/json; charset=UTF-8");
    xhr.responseType = "json";
    xhr.mozBackgroundRequest = true;
    xhr.channel.loadFlags = Ci.nsIChannel.LOAD_ANONYMOUS;
    xhr.onerror = function() {
      listener.notifyError(POSITION_UNAVAILABLE);
    };
    xhr.onload = function() {
      LOG("gls returned status: " + xhr.status + " --> " +  JSON.stringify(xhr.response));
      if ((xhr.channel instanceof Ci.nsIHttpChannel && xhr.status != 200) ||
          !xhr.response || !xhr.response.location) {
        listener.notifyError(POSITION_UNAVAILABLE);
        return;
      }

      let newLocation = new WifiGeoPositionObject(xhr.response.location.lat,
                                                  xhr.response.location.lng,
                                                  xhr.response.accuracy);

      listener.update(newLocation);
      gCachedRequest = new CachedRequest(newLocation, data.cellTowers, data.wifiAccessPoints);
    };

    var requestData = JSON.stringify(data);
    LOG("sending " + requestData);
    xhr.send(requestData);
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([WifiGeoPositionProvider]);
