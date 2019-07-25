












const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ctor = Components.Constructor;
const Exception = Components.Exception;

const URIDEFAULT = "moz-icon://file.%e";
const URISIZE = "moz-icon://file.%e?size=%s";


const IOService = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);




var extensions = [
  'png', 'jpg', 'jpeg',
  'pdf',
  'mpg', 'avi', 'mov',
  'zip', 'rar', 'tar.gz', 'tar.bz2',
];


var sizes = [
  16,
  24,
  32,
  48,
  64
];


function Listener() {}
Listener.prototype = {
  _dataReceived: false,
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIStreamListener)
        || iid.equals(Ci.nsIRequestObserver)
        || iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
  onStopRequest: function(aRequest, aContext, aStatusCode) {
    
    
    
    do_check_true(!!this._dataReceived);

    do_test_finished();
  },
  onStartRequest: function() {},
  onDataAvailable: function(aRequest, aContext, aInputStream, aOffset, aCount) {
    this._dataReceived |= aInputStream.available() > 0;

    aRequest.cancel(0x804B0002); 
  }
};


function verifyChannelFor(aExt, aURI) {
  var uri = aURI.replace(/%e/g, aExt);
  try {
    var channel = IOService.newChannel(uri, null, null);
    channel.asyncOpen(new Listener(), null);

    do_test_pending();
  }
  catch (ex) {
    
    do_throw("Cannot open channel for " + uri + " Error: " + ex);
  }
}


function run_test() {
  for each (let ext in extensions) {
    verifyChannelFor(ext, URIDEFAULT);
    for each (let size in sizes) {
      verifyChannelFor(ext, URISIZE.replace(/%s/g, size));
    }
  }
};
