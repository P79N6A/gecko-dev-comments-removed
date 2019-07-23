Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;

var gLoggingEnabled = true;
var gTestingEnabled = false;

function nowInSeconds()
{
    return Date.now() / 1000;
}

function LOG(aMsg) {
  if (gLoggingEnabled)
  {
    aMsg = ("*** WIFI GEO: " + aMsg);
    Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).logStringMessage(aMsg);
    dump(aMsg);
  }
}

function WifiGeoAddressObject(streetNumber, street, premises, city, county, region, country, countryCode, postalCode) {

  this.streetNumber = streetNumber;
  this.street       = street;
  this.premises     = premises;
  this.city         = city;
  this.county       = county;
  this.region       = region;
  this.country      = country;
  this.countryCode  = countryCode;
  this.postalCode   = postalCode;
}

WifiGeoAddressObject.prototype = {

    QueryInterface:   XPCOMUtils.generateQI([Ci.nsIDOMGeoPositionAddress, Ci.nsIClassInfo]),

    getInterfaces: function(countRef) {
        var interfaces = [Ci.nsIDOMGeoPositionAddress, Ci.nsIClassInfo, Ci.nsISupports];
        countRef.value = interfaces.length;
        return interfaces;
    },

    getHelperForLanguage: function(language) null,
    contractID: "",
    classDescription: "wifi geo position address object",
    classID: null,
    implementationLanguage: Ci.nsIProgrammingLanguage.JAVASCRIPT,
    flags: Ci.nsIClassInfo.DOM_OBJECT,
};

function WifiGeoCoordsObject(lat, lon, acc, alt, altacc) {
    this.latitude = lat;
    this.longitude = lon;
    this.accuracy = acc;
    this.altitude = alt;
    this.altitudeAccuracy = altacc;
};

WifiGeoCoordsObject.prototype = {

    QueryInterface:   XPCOMUtils.generateQI([Ci.nsIDOMGeoPositionCoords, Ci.nsIClassInfo]),

    getInterfaces: function(countRef) {
        var interfaces = [Ci.nsIDOMGeoPositionCoords, Ci.nsIClassInfo, Ci.nsISupports];
        countRef.value = interfaces.length;
        return interfaces;
    },

    getHelperForLanguage: function(language) null,
    contractID: "",
    classDescription: "wifi geo position coords object",
    classID: null,
    implementationLanguage: Ci.nsIProgrammingLanguage.JAVASCRIPT,
    flags: Ci.nsIClassInfo.DOM_OBJECT,

    latitude: 0,
    longitude: 0,
    accuracy: 0,
    altitude: 0,
    altitudeAccuracy: 0,

};

function WifiGeoPositionObject(location, address) {

    this.coords = new WifiGeoCoordsObject(location.latitude,
                                          location.longitude,
                                          location.accuracy || 12450, 
                                          location.altitude || 0,
                                          location.altitude_accuracy || 0);

    if (address) {
        this.address = new WifiGeoAddressObject(address.street_number || null,
                                                address.street || null,
                                                address.premises || null,
                                                address.city || null,
                                                address.county || null,
                                                address.region || null,
                                                address.country || null,
                                                address.country_code || null,
                                                address.postal_code || null);
    }
    else
      this.address = null;

    this.timestamp = Date.now();
};

WifiGeoPositionObject.prototype = {

    QueryInterface:   XPCOMUtils.generateQI([Ci.nsIDOMGeoPosition, Ci.nsIClassInfo]),

    
    getInterfaces: function(countRef) {
        var interfaces = [Ci.nsIDOMGeoPosition, Ci.nsIClassInfo, Ci.nsISupports];
        countRef.value = interfaces.length;
        return interfaces;
    },

    getHelperForLanguage: function(language) null,
    contractID: "",
    classDescription: "wifi geo location position object",
    classID: null,
    implementationLanguage: Ci.nsIProgrammingLanguage.JAVASCRIPT,
    flags: Ci.nsIClassInfo.DOM_OBJECT,

    coords: null,
    timestamp: 0,
};

function WifiGeoPositionProvider() {
    this.prefService = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch).QueryInterface(Ci.nsIPrefService);
    try {
        gLoggingEnabled = this.prefService.getBoolPref("geo.wifi.logging.enabled");
    } catch (e) {}

    try {
        gTestingEnabled = this.prefService.getBoolPref("geo.wifi.testing");
    } catch (e) {}

};

WifiGeoPositionProvider.prototype = {
    classDescription: "A component that returns a geolocation based on WIFI",
    classID:          Components.ID("{77DA64D3-7458-4920-9491-86CC9914F904}"),
    contractID:       "@mozilla.org/geolocation/provider;1",
    QueryInterface:   XPCOMUtils.generateQI([Ci.nsIGeolocationProvider, Ci.nsIWifiListener, Ci.nsITimerCallback]),

    prefService:     null,

    provider_url:    null,
    wifi_service:    null,
    timer:           null,
    hasSeenWiFi:     false,

    observe: function (aSubject, aTopic, aData) {
        if (aTopic == "private-browsing") {
            if (aData == "enter" || aData == "exit") {
                try {
                    let branch = this.prefService.getBranch("geo.wifi.access_token.");
                    branch.deleteBranch("");
                } catch (e) {}
            }
        }
    },

    startup:         function() {
        LOG("startup called");

        this.provider_url = this.prefService.getCharPref("geo.wifi.uri");
        LOG("provider url = " + this.provider_url);

        
        
        
        this.hasSeenWiFi = false;
        this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        if (gTestingEnabled == false)
            this.timer.initWithCallback(this, 5000, this.timer.TYPE_ONE_SHOT);
        else
            this.timer.initWithCallback(this, 200, this.timer.TYPE_REPEATING_SLACK);


        let os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
        os.addObserver(this, "private-browsing", false);
    },

    isReady:         function() {
        LOG("isReady called");
        return true
    },
  
    watch: function(c) {
        LOG("watch called");
        if (!this.wifi_service) {
            this.wifi_service = Cc["@mozilla.org/wifi/monitor;1"].getService(Components.interfaces.nsIWifiMonitor);
            this.wifi_service.startWatching(this);
        }
    },

    shutdown: function() { 
        LOG("shutdown  called");
        if(this.wifi_service)
            this.wifi_service.stopWatching(this);
        this.wifi_service = null;

        if (this.timer != null) {
            this.timer.cancel();
            this.timer = null;
        }

        
        
        
        
        
        let prefBranch = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
        if (prefBranch.getIntPref("network.cookie.lifetimePolicy") != 0)
            prefBranch.deleteBranch("geo.wifi.access_token.");

        let os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
        os.removeObserver(this, "private-browsing");
    },

    getAccessTokenForURL: function(url)
    {
        
        var accessToken = "";
        
        try {
            var accessTokenPrefName = "geo.wifi.access_token." + url;
            accessToken = this.prefService.getCharPref(accessTokenPrefName);
            
            
            var accessTokenDate = this.prefService.getIntPref(accessTokenPrefName + ".time");
            
            var accessTokenInterval = 1209600;  
            try {
                accessTokenInterval = this.prefService.getIntPref("geo.wifi.access_token.recycle_interval");
            } catch (e) {}
            
            if (nowInSeconds() - accessTokenDate > accessTokenInterval)
                accessToken = "";
        }
        catch (e) {
            accessToken = "";
            LOG("Error: "+ e);
        }
        return accessToken;
    },

    onChange: function(accessPoints) {

        LOG("onChange called");
        this.hasSeenWiFi = true;

        
        var xhr = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Ci.nsIXMLHttpRequest);

        
        xhr.mozBackgroundRequest = true;

        xhr.open("POST", this.provider_url, false);
        
        
        xhr.channel.loadFlags = Ci.nsIChannel.LOAD_ANONYMOUS;
            
        xhr.onerror = function(req) {
            LOG("onerror: " + req);
        };

        xhr.onload = function (req) {  

            LOG("service returned: " + req.target.responseText);

            
            var response = JSON.parse(req.target.responseText);

            
            

            
            var newAccessToken = response.access_token;
            if (newAccessToken != undefined)
            {
                var prefService = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
                var accessToken = "";
                var accessTokenPrefName = "geo.wifi.access_token." + req.target.channel.URI.spec;
                try { accessToken = prefService.getCharPref(accessTokenPrefName); } catch (e) {}

                if (accessToken != newAccessToken) {
                    
                    LOG("New Access Token: " + newAccessToken + "\n" + accessTokenPrefName);
                    
                    prefService.setIntPref(accessTokenPrefName + ".time", nowInSeconds());
                    prefService.setCharPref(accessTokenPrefName, newAccessToken);
                }
            }

            var address = null;
            try {
                address = response.location.address;
            } catch (e) {
                LOG("No address in response");
            }

            LOG("sending update to geolocation.");

            var newLocation = new WifiGeoPositionObject(response.location, address);

            var update = Cc["@mozilla.org/geolocation/service;1"].getService(Ci.nsIGeolocationUpdate);
            update.update(newLocation);
        };

        var accessToken = this.getAccessTokenForURL(this.provider_url);

        var request = {
            version: "1.1.0",
            request_address: true,
        };

        if (accessToken != "")
            request.access_token = accessToken;

        if (accessPoints != null) {
            request.wifi_towers = accessPoints.map(function (ap) ({
                        mac_address: ap.mac,
                        ssid: ap.ssid,
                        signal_strength: ap.signal,
                    }));
        }

        var jsonString = JSON.stringify(request);
        LOG("client sending: " + jsonString);

        xhr.send(jsonString);
    },

    onError: function (code) {
        LOG("wifi error: " + code);
    },

    notify: function (timer) {
        if (this.hasSeenWiFi == false)
            this.onChange(null);
        this.timer = null;
    },

};

var components = [WifiGeoPositionProvider];
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule(components);
}
