Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;

var gLoggingEnabled = false;
var gTestingEnabled = false;

function nowInSeconds()
{
    return Date.now() / 1000;
}

function LOG(aMsg) {
  if (gLoggingEnabled)
  {
    aMsg = "*** WIFI GEO: " + aMsg + "\n";
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

    QueryInterface:   XPCOMUtils.generateQI([Ci.nsIDOMGeoPositionAddress]),

    classInfo: XPCOMUtils.generateCI({interfaces: [Ci.nsIDOMGeoPositionAddress],
                                      flags: Ci.nsIClassInfo.DOM_OBJECT})
};

function WifiGeoCoordsObject(lat, lon, acc, alt, altacc) {
    this.latitude = lat;
    this.longitude = lon;
    this.accuracy = acc;
    this.altitude = alt;
    this.altitudeAccuracy = altacc;
};

WifiGeoCoordsObject.prototype = {

    QueryInterface:   XPCOMUtils.generateQI([Ci.nsIDOMGeoPositionCoords]),

    classInfo: XPCOMUtils.generateCI({interfaces: [Ci.nsIDOMGeoPositionCoords],
                                      flags: Ci.nsIClassInfo.DOM_OBJECT,
                                      classDescription: "wifi geo position coords object"}),

    latitude: 0,
    longitude: 0,
    accuracy: 0,
    altitude: 0,
    altitudeAccuracy: 0,

};

function WifiGeoPositionObject(location) {

    this.coords = new WifiGeoCoordsObject(location.latitude,
                                          location.longitude,
                                          location.accuracy || 12450, 
                                          location.altitude || 0,
                                          location.altitude_accuracy || 0);

    if (location.address) {
        let address = location.address;
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

    QueryInterface:   XPCOMUtils.generateQI([Ci.nsIDOMGeoPosition]),

    
    classInfo: XPCOMUtils.generateCI({interfaces: [Ci.nsIDOMGeoPosition],
                                      flags: Ci.nsIClassInfo.DOM_OBJECT,
                                      classDescription: "wifi geo location position object"}),

    coords: null,
    timestamp: 0,
};

function HELD() {};
 
 
HELD.encode = function(requestObject) {
    
    var requestString = "<locationRequest xmlns=\"urn:ietf:params:xml:ns:geopriv:held\">";
    
    if (requestObject.wifi_towers && requestObject.wifi_towers.length > 0) {
      requestString += "<measurements xmlns=\"urn:ietf:params:xml:ns:geopriv:lm\">";
      requestString += "<wifi xmlns=\"urn:ietf:params:xml:ns:geopriv:lm:wifi\">";
      for (var i=0; i < requestObject.wifi_towers.length; ++i) {
        requestString += "<neighbourWap>";
        requestString += "<bssid>" + requestObject.wifi_towers[i].mac_address     + "</bssid>";
        requestString += "<ssid>"  + requestObject.wifi_towers[i].ssid            + "</ssid>";
        requestString += "<rssi>"  + requestObject.wifi_towers[i].signal_strength + "</rssi>";
        requestString += "</neighbourWap>";
      }
      
      requestString += "</wifi></measurements>";
    }
    requestString += "</locationRequest>";
    return requestString;
};


HELD.decode = function(responseXML) {
    
    function nsResolver(prefix) {
        var ns = {
            'held': 'urn:ietf:params:xml:ns:geopriv:held',
            'pres': 'urn:ietf:params:xml:ns:pidf',
            'gp': 'urn:ietf:params:xml:ns:pidf:geopriv10',
            'gml': 'http://www.opengis.net/gml',
            'gs': 'http://www.opengis.net/pidflo/1.0',
        };
        return ns[prefix] || null;
    }

    var xpathEval = Components.classes["@mozilla.org/dom/xpath-evaluator;1"].createInstance(Ci.nsIDOMXPathEvaluator);

    
    var pos = xpathEval.evaluate(
        '/held:locationResponse/pres:presence/pres:tuple/pres:status/gp:geopriv/gp:location-info/gs:Circle/gml:pos',
        responseXML,
        nsResolver,
        Ci.nsIDOMXPathResult.STRING_TYPE,
        null);

    var rad = xpathEval.evaluate(
        '/held:locationResponse/pres:presence/pres:tuple/pres:status/gp:geopriv/gp:location-info/gs:Circle/gs:radius',
        responseXML,
        nsResolver,
        Ci.nsIDOMXPathResult.NUMBER_TYPE,
        null );

    var uom = xpathEval.evaluate(
        '/held:locationResponse/pres:presence/pres:tuple/pres:status/gp:geopriv/gp:location-info/gs:Circle/gs:radius/@uom',
        responseXML,
        nsResolver,
        Ci.nsIDOMXPathResult.STRING_TYPE,
        null);

    
    if ((pos.stringValue == null) ||
        (rad.numberValue == null) ||
        (uom.stringValue == null) ||
        (uom.stringValue != "urn:ogc:def:uom:EPSG::9001")) {
        return null;
    }

    
    var coords = pos.stringValue.split(/[ \t\n]+/);

    
    var obj = {
        location: {
            latitude: parseFloat(coords[0]),
            longitude: parseFloat(coords[1]),
            accuracy: rad.numberValue
        }
    };
    return obj;
}  

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
    classID:          Components.ID("{77DA64D3-7458-4920-9491-86CC9914F904}"),
    QueryInterface:   XPCOMUtils.generateQI([Ci.nsIGeolocationProvider,
                                             Ci.nsIWifiListener,
                                             Ci.nsITimerCallback]),

    prefService:     null,
    wifi_service:    null,
    timer:           null,
    hasSeenWiFi:     false,
    started:         false,

    startup:         function() {
        if (this.started == true)
            return;

        this.started = true;

        LOG("startup called.  testing mode is" + gTestingEnabled);
        
        
        
        this.hasSeenWiFi = false;
        this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        if (gTestingEnabled == false)
            this.timer.initWithCallback(this, 5000, this.timer.TYPE_ONE_SHOT);
        else
            this.timer.initWithCallback(this, 200, this.timer.TYPE_REPEATING_SLACK);
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

        this.started = false;
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
        }
        return accessToken;
    },

    onChange: function(accessPoints) {

        LOG("onChange called");
        this.hasSeenWiFi = true;

        
        var xhr = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"]
                            .createInstance(Ci.nsIXMLHttpRequest);

        
        xhr.mozBackgroundRequest = true;

        var provider_url      = this.prefService.getCharPref("geo.wifi.uri");
        var provider_protocol = 0;
        try {
            provider_protocol = this.prefService.getIntPref("geo.wifi.protocol");
        } catch (e) {}

        LOG("provider url = " + provider_url);

        xhr.open("POST", provider_url, false);
        
        
        xhr.channel.loadFlags = Ci.nsIChannel.LOAD_ANONYMOUS;
            
        xhr.onerror = function(req) {
            LOG("onerror: " + req);
        };

        xhr.onload = function (req) {  

            LOG("xhr onload...");

            try { 
                
                var response;
                switch (provider_protocol) {
                    case 1:
                        LOG("service returned: " + req.target.responseXML);
                        response = HELD.decode(req.target.responseXML);
                        break;
                    case 0:
                    default:
                        LOG("service returned: " + req.target.responseText);
                        response = JSON.parse(req.target.responseText);
                }
            } catch (e) {
                LOG("Parse failed");
                return;
            }

            
            

            
            var newAccessToken = response.access_token;
            if (newAccessToken != undefined)
            {
                var prefService = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
                var accessToken = "";
                var accessTokenPrefName = "geo.wifi.access_token." + req.target.channel.URI.spec;
                try { accessToken = prefService.getCharPref(accessTokenPrefName); } catch (e) {}

                if (accessToken != newAccessToken) {
                    
                    LOG("New Access Token: " + newAccessToken + "\n" + accessTokenPrefName);
                    
                    try {
                        prefService.setIntPref(accessTokenPrefName + ".time", nowInSeconds());
                        prefService.setCharPref(accessTokenPrefName, newAccessToken);
                    } catch (x) {
                        
                    }
                }
            }

            if (response.location) {
                var newLocation = new WifiGeoPositionObject(response.location);

                var update = Cc["@mozilla.org/geolocation/service;1"].getService(Ci.nsIGeolocationUpdate);
                update.update(newLocation);
            }
        };

        var accessToken = this.getAccessTokenForURL(provider_url);

        var request = {
            version: "1.1.0",
            request_address: true,
        };

        if (accessToken != "")
            request.access_token = accessToken;

        if (accessPoints != null) {
            function filterBlankSSIDs(ap) ap.ssid != ""
            function deconstruct(ap) ({
                    mac_address: ap.mac,
                        ssid: ap.ssid,
                        signal_strength: ap.signal
                        })
            request.wifi_towers = accessPoints.filter(filterBlankSSIDs).map(deconstruct);
        }

        var requestString;
        switch (provider_protocol) {
          case 1:
              requestString = HELD.encode(request);
              break;
          case 0:
          default:
              requestString = JSON.stringify(request);
        }
        LOG("client sending: " + requestString);
 
        try {
          xhr.send(requestString);
        } catch (e) {}
    },

    onError: function (code) {
        LOG("wifi error: " + code);
    },

    notify: function (timer) {
        if (!gTestingEnabled) {
            if (this.hasSeenWiFi == false)
                this.onChange(null);
            this.timer = null;
            return;
        }
        
        this.onChange(null);
    },

};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([WifiGeoPositionProvider]);
