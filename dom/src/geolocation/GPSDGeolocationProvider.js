







































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;

var gLoggingEnabled = false;

function LOG(aMsg) {
  if (gLoggingEnabled)
  {
    aMsg = ("*** GPSD GEO: " + aMsg);
    Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).logStringMessage(aMsg);
    dump(aMsg);
  }
}

function GeoPositionCoordsObject(latitude, longitude, altitude, accuracy, altitudeAccuracy, heading, speed) {

  this.latitude = latitude;
  this.longitude = longitude;
  this.altitude = altitude;
  this.accuracy = accuracy;
  this.altitudeAccuracy = altitudeAccuracy;
  this.heading = heading;
  this.speed = speed;

};

GeoPositionCoordsObject.prototype = {

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMGeoPositionCoords, Ci.nsIClassInfo]),
  
  
  getInterfaces: function(countRef) {
    var interfaces = [Ci.nsIDOMGeoPositionCoords, Ci.nsIClassInfo, Ci.nsISupports];
    countRef.value = interfaces.length;
    return interfaces;
  },
  
  getHelperForLanguage: function(language) null,
  contractID: null,
  classDescription: "Geoposition Coordinate Object",
  classID: null,
  implementationLanguage: Ci.nsIProgrammingLanguage.JAVASCRIPT,
  flags: Ci.nsIClassInfo.DOM_OBJECT,

  latitude: null,
  longitude: null,
  altitude: null,
  accuracy: null,
  altitudeAccuracy: null,
  heading: null,
  speed: null,

};

function GeoPositionObject(latitude, longitude, altitude, accuracy, altitudeAccuracy, heading, speed, timestamp) {
  this.coords = new GeoPositionCoordsObject(latitude, longitude, altitude, accuracy, altitudeAccuracy, heading, speed);
  this.timestamp = timestamp;
};

GeoPositionObject.prototype = {

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMGeoPosition, Ci.nsIClassInfo]),

  
  getInterfaces: function(countRef) {
    var interfaces = [Ci.nsIDOMGeoPosition, Ci.nsIClassInfo, Ci.nsISupports];
    countRef.value = interfaces.length;
    return interfaces;
  },

  getHelperForLanguage: function(language) null,
  contractID: null,
  classDescription: "Geoposition Object",
  classID: null,
  implementationLanguage: Ci.nsIProgrammingLanguage.JAVASCRIPT,
  flags: Ci.nsIClassInfo.DOM_OBJECT,

  coords: null,
  timestamp: null,

};

function GPSDProvider() {
  this.prefService = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch).QueryInterface(Ci.nsIPrefService);

  try {
    gLoggingEnabled = this.prefService.getBoolPref("geo.gpsd.logging.enabled");
  } catch (e) {}
};

GPSDProvider.prototype = {
  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIGeolocationProvider]),
  
  classDescription: "Returns a geolocation from a GPSD source",
  classID: Components.ID("{0A3BE523-0F2A-32CC-CCD8-1E5986D5A79D}"),
  contractID: "@mozilla.org/geolocation/gpsd/provider;1",
  _xpcom_categories: [{
    category: "geolocation-provider",
  }],
  
  prefService: null,

  transport: null,
  outputStream: null,
  inputStream: null,
  
  startup: function() {

    LOG("startup called\n");
    var socketTransportService = Cc["@mozilla.org/network/socket-transport-service;1"].getService(Ci.nsISocketTransportService);
    
    var hostIPAddr = "127.0.0.1";
    var hostPort = "2947";

    try {
      hostIPAddr = this.prefService.getCharPref("geo.gpsd.host.ipaddr");
    } catch (e) {}

    try {
      hostPort = this.prefService.getCharPref("geo.gpsd.host.port");
    } catch (e) {}

    LOG("Host info: " + hostIPAddr + ":" + hostPort + "\n");

    this.transport = socketTransportService.createTransport(null, 0, hostIPAddr, hostPort, null);
    
    
    this.outputStream = this.transport.openOutputStream(0,0,0);
    this.inputStream = this.transport.openInputStream(0,0,0);

  },
  
  shutdown: function() {
    LOG("shutdown called\n"); 
    this.outputStream.close();
    this.inputStream.close();
    this.transport.close();
  },
  
  isReady: function() {
    LOG("isReady called\n");
    try {
      this.inputStream.available();
    } catch (e) {
      return false;
    }

    return true;
  },
  
  watch: function(c) {
    LOG("watch called\n");    

    
    
    var bufferOption = "J=1\n";
    this.outputStream.write(bufferOption, bufferOption.length);

    
    var mode = "w\n";
    this.outputStream.write(mode, mode.length);
    
    var dataListener = {
      onStartRequest: function(request, context) {},
      onStopRequest: function(request, context, status) {},
      onDataAvailable: function(request, context, inputStream, offset, count) {
    
        var sInputStream = Cc["@mozilla.org/scriptableinputstream;1"].createInstance(Ci.nsIScriptableInputStream);
        sInputStream.init(inputStream);

        var s = sInputStream.read(count);
        
        var response = s.split('=');
        
        var header = response[0];
        var info = response[1];
        
        
        if (header != 'GPSD,O') {
          
          return;
        }
        
        
        if (info == '?') {
          
          return;
        }
    
        
        var fields = info.split(' ');
        
        
        if (fields[0] != 'RMC') {
          return;
        }

        LOG("Got info: " + info);

        for (var i = 0; i < fields.length; i++) {
          if (fields[i] == '?') {
            fields[i] = null;
          }
        }
        
        var timestamp = fields[1]; 
        var timeError = fields[2]; 
        var latitude = fields[3]; 
        var longitude = fields[4]; 
        var altitude = fields[5]; 
        var horizontalError = fields[6]; 
        var verticalError = fields[7]; 
        var course = fields[8]; 
        var speed = fields[9]; 
        
        var geoPos = new GeoPositionObject(latitude, longitude, altitude, horizontalError, verticalError, course, speed, timestamp);
        
        c.update(geoPos);
    
      }
      
    };
    
    var pump = Cc["@mozilla.org/network/input-stream-pump;1"].createInstance(Ci.nsIInputStreamPump);
    pump.init(this.inputStream, -1, -1, 0, 0, false);
    pump.asyncRead(dataListener, null);

  },
  
};

var components = [GPSDProvider];

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule(components);
}
