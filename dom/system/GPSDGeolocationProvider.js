







































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

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMGeoPositionCoords]),
  
  
  classInfo: XPCOMUtils.generateCI({interfaces: [Ci.nsIDOMGeoPositionCoords],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT,
                                    classDescription: "Geoposition Coordinate Object"}),

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

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMGeoPosition]),

  
  classInfo: XPCOMUtils.generateCI({interfaces: [Ci.nsIDOMGeoPosition],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT,
                                    classDescription: "Geoposition Object"}),

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
  
  classID: Components.ID("{0A3BE523-0F2A-32CC-CCD8-1E5986D5A79D}"),
  
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
    this.transport.close(Components.results.NS_OK);
  },
  
  watch: function(c) {
    LOG("watch called\n");    
    try {
        
        var mode = '?WATCH={"enable":true,"json":true}';
        this.outputStream.write(mode, mode.length);
    } catch (e) { return; }

    var dataListener = {
      onStartRequest: function(request, context) {},
      onStopRequest: function(request, context, status) {},
      onDataAvailable: function(request, context, inputStream, offset, count) {
    
        var sInputStream = Cc["@mozilla.org/scriptableinputstream;1"].createInstance(Ci.nsIScriptableInputStream);
        sInputStream.init(inputStream);

        var responseSentence = sInputStream.read(count);
        
        var response = null; 
        try {
          response = JSON.parse(responseSentence);
          } catch (e) { return; }
        
        
        if (response.class != 'TPV') {
          
          return;
        }

        
        if (response.mode == '1') {
          
          return;
        }
        
        LOG("Got info: " + responseSentence);
 
        
        
        if (response.time && response.lat && response.lon
            && response.epx && response.epy) {
        var timestamp = response.time; 
        var latitude = response.lat; 
        var longitude = response.lon; 
        var horizontalError = Math.max(response.epx,response.epy); } 
        else { return; }
        
        
        var altitude = null;
        var verticalError = null; 
        if (response.alt && response.epv) {
          altitude = response.alt; 
          verticalError = response.epv; 
        } 

        var speed = null;
        if (response.speed) { var speed = response.speed; } 
         
        var course = null;
        if (response.track) { var course = response.track; } 
        
        var geoPos = new GeoPositionObject(latitude, longitude, altitude, horizontalError, verticalError, course, speed, timestamp);
        
        c.update(geoPos);
        LOG("Position updated:" + timestamp + "," + latitude + "," + longitude + ","
             + horizontalError + "," + altitude + "," + verticalError + "," + course 
             + "," + speed);
    
      }
      
    };
    
    var pump = Cc["@mozilla.org/network/input-stream-pump;1"].createInstance(Ci.nsIInputStreamPump);
    pump.init(this.inputStream, -1, -1, 0, 0, false);
    pump.asyncRead(dataListener, null);

  },
  
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([GPSDProvider]);
