const EXPORTED_SYMBOLS = ['HTTPPollingTransport'];

var Cc = Components.classes;
var Ci = Components.interfaces;










function InputStreamBuffer() {
}
InputStreamBuffer.prototype = {
  _data: "",
  append: function( stuff ) {
    this._data = this._data + stuff;
  },
  clear: function() {
    this._data = "";
  },
  getData: function() {
    return this._data;
  }
}







function SocketClient( host, port ) {
  this._init( host, port );
}
SocketClient.prototype = {
  __threadManager: null,
  get _threadManager() {
    if (!this.__threadManager)
      this.__threadManager = Cc["@mozilla.org/thread-manager;1"].getService();
    return this.__threadManager;
  },

  __transport: null,
  get _transport() {
    if (!this.__transport) {
      var transportService = Cc["@mozilla.org/network/socket-transport-service;1"].getService(Ci.nsISocketTransportService);
      this.__transport = transportService.createTransport(['starttls'],
							   1,    
							   this._host,
							   this._port,
							   null); 
    }
    return this.__transport;
  },

  _init: function( host, port ) {
    this._host = host;
    this._port = port;
    this._contentRead = "";
    this._buffer = null;
    this.connect();
  },
 
  connect: function() {
    var outstream = this._transport.openOutputStream( 0,  
						      0,  
						      0 ); 
    this._outstream = outstream;

    var buffer = new InputStreamBuffer;
    this._buffer = buffer;

    
    
    var rawInputStream = this._transport.openInputStream( 0, 0, 0 );
    var scriptablestream = Cc["@mozilla.org/scriptableinputstream;1"].createInstance(Ci.nsIScriptableInputStream);
    scriptablestream.init(rawInputStream);

    
    var pump = Cc["@mozilla.org/network/input-stream-pump;1"].createInstance(Ci.nsIInputStreamPump);
    pump.init(rawInputStream, -1, -1, 0, 0, 
	      false); 

    
    var dataListener = {
      data : "",
      onStartRequest: function(request, context){
      },
      onStopRequest: function(request, context, status){
	      rawInputStream.close();
	      outstream.close();
      },
      onDataAvailable: function(request, context, inputStream, offset, count){
        
        
        
        buffer.append( scriptablestream.read( count ));
      }
    };
    
    pump.asyncRead(dataListener, null); 
    

    
  },

  send: function( messageText ) {
    this._outstream.write( messageText, messageText.length );
  },

  getBinaryOutStream: function() {
    var binaryOutStream = Cc["@mozilla.org/binaryoutputstream;1"].createInstance(Ci.nsIBinaryOutputStream);
    binaryOutStream.setOutputStream( this._outstream ); 
    return binaryOutStream;
  },

  disconnect: function() {
    var thread = this._threadManager.currentThread;
    while ( thread.hasPendingEvents() ) {
	    thread.processNextEvent( true );
    }
  },
 
  checkResponse: function() {
    return this._getData();
  },

  waitForResponse: function() {
    var thread = this._threadManager.currentThread;
    while( this._buffer.getData().length == 0 ) {
    	thread.processNextEvent( true );
    }
    var output = this._buffer.getData();
    this._buffer.clear();
    return output;
  },

  startTLS: function() {
    this._transport.securityInfo.QueryInterface(Ci.nsISSLSocketControl);
    this._transport.securityInfo.StartTLS();
  }
};










function HTTPPollingTransport( serverUrl, useKeys, interval ) {
  this._init( serverUrl, useKeys, interval );
}
HTTPPollingTransport.prototype = {
  _init: function( serverUrl, useKeys, interval ) {
    this._log = Log4Moz.Service.getLogger("Service.XmppTransportLayer");
    this._log.info("Initializing transport: serverUrl=" + serverUrl + ", useKeys=" + useKeys + ", interval=" + interval);
    this._serverUrl = serverUrl
    this._n = 0;
    this._key = this._makeSeed();
    this._latestCookie = "";
    this._connectionId = 0;
    this._callbackObject = null;
    this._useKeys = useKeys;
    this._interval = interval;
    this._outgoingRetryBuffer = "";
    this._retryCount = 0;
    this._retryCap = 0;
  },

  __request: null,
  get _request() {
    if (!this.__request)
      this.__request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance( Ci.nsIXMLHttpRequest );
    return this.__request;
  },

  __hasher: null,
  get _hasher() {
    if (!this.__hasher)
      this.__hasher = Cc["@mozilla.org/security/hash;1"].createInstance( Ci.nsICryptoHash );
    return this.__hasher;
  },

  __timer: null,
  get _timer() {
    if (!this.__timer)
      this.__timer = Cc["@mozilla.org/timer;1"].createInstance( Ci.nsITimer );
    return this.__timer;
  },

  _makeSeed: function() {
    return "foo";
  },

  _advanceKey: function() {
    var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                    createInstance(Ci.nsIScriptableUnicodeConverter);

    
    
    converter.charset = "UTF-8";
    
    
    var result = {};
    
    var data = converter.convertToByteArray(this._key, result);

    this._n += 1;
    this._hasher.initWithString( "SHA1" );
    this._hasher.update( data, data.length );
    this._key = this._hasher.finish( true ); 
  },

  _setIdFromCookie: function( self, cookie ) {
    
    
    var cookieSegments = cookie.split( ";" );
    cookieSegments = cookieSegments[0].split( "=" );
    var newConnectionId = cookieSegments[1];
    switch( newConnectionId) {
    case "0:0":
      self._onError( "Unknown error!\n" );
      break;
    case "-1:0":
      self._onError( "Server error!\n" );
      break;
    case "-2:0":
      self._onError( "Bad request!\n" );
      break;
    case "-3:0":
      self._onError( "Key sequence error!\n" );
      break;
    default :
      self._connectionId = cookieSegments[1];
      
      break;
    }
  },

  _onError: function( errorText ) {
    dump( "Transport error: " + errorText + "\n" );
    if ( this._callbackObject != null ) {
      this._callbackObject.onTransportError( errorText );
    }
    this.disconnect();
  },

  _doPost: function( requestXml ) {
    var request = this._request;
    var callbackObj = this._callbackObject;
    var self = this;
    var contents = "";

    if ( this._useKey ) {
      this._advanceKey();
      contents = this._connectionId + ";" + this._key + "," + requestXml;
    } else {
      contents = this._connectionId + "," + requestXml;
      


    }    

    var _processReqChange = function() {
      
      if ( request.readyState == 4 ) {
        if ( request.status == 200) {
          
          
          self._log.debug("Server says: " + request.responseText);
          
          var latestCookie = request.getResponseHeader( "Set-Cookie" );
          if ( latestCookie.length > 0 ) {
            self._setIdFromCookie( self, latestCookie );
          }

          
          if ( callbackObj != null && request.responseText.length > 0 ) {
            callbackObj.onIncomingData( request.responseText );
          }
        } else {
          self._log.error( "Got HTTP status code " + request.status );
          if ( request.status == 0 ) {
            




            if (self._retryCount >= self._retryCap) {
              self._onError("Maximum number of retries reached. Unable to communicate with the server.");
            }
            else {
              self._outgoingRetryBuffer = requestXml;
              self._retryCount++;
            }
          }
          else if (request.status == 404) {
            self._onError("Provided URL is not valid.");
          }
          else {
            self._onError("Unable to communicate with the server.");
          }
        }
      }
    };

    try {
      request.open( "POST", this._serverUrl, true ); 
      request.setRequestHeader( "Content-type", "application/x-www-form-urlencoded;charset=UTF-8" );
      request.setRequestHeader( "Content-length", contents.length );
      request.setRequestHeader( "Connection", "close" );
      request.onreadystatechange = _processReqChange;
      this._log.debug("Sending: " + contents);
      request.send( contents );
    } catch(ex) { 
      this._onError("Unable to send message to server: " + this._serverUrl);
      this._log.error("Connection failure: " + ex);
    }
  },

  send: function( messageXml ) {
    this._doPost( messageXml );
  },
 
  setCallbackObject: function( callbackObject ) {
    this._callbackObject = callbackObject;
  },

  notify: function( timer ) {
    


    




    var outgoingMsg = this._outgoingRetryBuffer
    this._outgoingRetryBuffer = "";
    this._doPost( outgoingMsg );
  },
 
  connect: function() {
    
    this._init(this._serverUrl, this._useKeys, this._interval);

    

    
    

    this._timer.initWithCallback( this, 
        this._interval, 
        this._timer.TYPE_REPEATING_SLACK );
  },

  disconnect: function () {
    this._request.abort();
    this._timer.cancel();
  },

  testKeys: function () {
    this._key = "foo";
    this._log.debug(this._key);
    for ( var x = 1; x < 7; x++ ) {
      this._advanceKey();
      this._log.debug(this._key);
    }
  }
};
