







var Cc = Components.classes;
var Ci = Components.interfaces;


function JabberClient( clientName, realm, clientPassword, transport, authenticator ) {
  this._init( clientName, realm, clientPassword, transport, authenticator );
}
JabberClient.prototype = {
 
 NOT_CONNECTED: 0,
 CALLED_SERVER: 1,
 AUTHENTICATING: 2,
 CONNECTED: 3,
 FAILED: -1,

 
 IQ_WAIT: 0,
 IQ_OK: 1,
 IQ_ERROR: -1,

 _init: function( clientName, realm, clientPassword, transport, authenticator ) {
    this._myName = clientName;
    this._realm = realm;
    this._fullName = clientName + "@" + realm;
    this._myPassword = clientPassword;
    this._connectionStatus = this.NOT_CONNECTED;
    this._error = null;
    this._streamOpen = false;
    this._transportLayer = transport;
    this._authenticationLayer = authenticator;
    this._authenticationLayer.initialize( clientName, realm, clientPassword );
    this._messageHandlers = [];
    this._iqResponders = [];
    this._nextIqId = 0;
    this._pendingIqs = {};
  },

 __parser: null,
  get _parser() {
   if (!this.__parser)
      this.__parser = Cc["@mozilla.org/xmlextras/domparser;1"].createInstance( Ci.nsIDOMParser );
    return this.__parser;
  },

  __threadManager: null,
  get _threadManager() {
    if (!this.__threadManager)
      this.__threadManager = Cc["@mozilla.org/thread-manager;1"].getService();
    return this.__threadManager;
  },


 parseError: function( streamErrorNode ) {
    dump( "Uh-oh, there was an error!\n" );
    var error = streamErrorNode.childNodes[0];
    dump( "Name: " + error.nodeName + " Value: " + error.nodeValue + "\n" );
    this._error = error.nodeName;
    this.disconnect();
    



  },

 setError: function( errorText ) {
    dump( "Error: " + errorText + "\n" );
    this._error = errorText;
    this._connectionStatus = this.FAILED;
  },

 onIncomingData: function( messageText ) {
    var responseDOM = this._parser.parseFromString( messageText, "text/xml" );
    
    if (responseDOM.documentElement.nodeName == "parsererror" ) {
      




      var response = messageText + this._makeClosingXml();
      responseDOM = this._parser.parseFromString( response, "text/xml" );
    }
    if ( responseDOM.documentElement.nodeName == "parsererror" ) {
      


      response = this._makeHeaderXml( this._fullName ) + messageText + this._makeClosingXml();
      responseDOM = this._parser.parseFromString( response, "text/xml" );
    }
    if ( responseDOM.documentElement.nodeName == "parsererror" ) {
      
      this.setError( "Can't parse incoming XML." );
      return;
    }

    var rootElem = responseDOM.documentElement;

    if ( this._connectionStatus == this.CALLED_SERVER ) {
      
      

      
      response = this._authenticationLayer.generateResponse( rootElem );
      if ( response == false ) {
	this.setError( this._authenticationLayer.getError() );
      } else if ( response == this._authenticationLayer.COMPLETION_CODE ){
	this._connectionStatus = this.CONNECTED;
	dump( "We be connected!!\n" );
      } else {
	this._transportLayer.send( response );
      }
      return;
    }

    if ( this._connectionStatus == this.CONNECTED ) {
      

      var errors = rootElem.getElementsByTagName( "stream:error" );
      if ( errors.length > 0 ) {
	this.setError( errors[0].firstChild.nodeName );
	return;
      }
      var presences = rootElem.getElementsByTagName( "presence" );
      if (presences.length > 0 ) {
	var from = presences[0].getAttribute( "from" );
	if ( from != undefined ) {
	  dump( "I see that " + from + " is online.\n" );
	}
      }
      if ( rootElem.nodeName == "message" ) {
	this.processIncomingMessage( rootElem );
      } else {
	var messages = rootElem.getElementsByTagName( "message" );
	if (messages.length > 0 ) {
	  for ( var message in messages ) {
	    this.processIncomingMessage( messages[ message ] );
	  }
	}
      }
      if ( rootElem.nodeName == "iq" ) {
	this.processIncomingIq( rootElem );
      } else {
	var iqs = rootElem.getElementsByTagName( "iq" );
	if ( iqs.length > 0 ) {
	  for ( var iq in iqs ) {
	    this.processIncomingIq( iqs[ iq ] );
	  }
	}
      }
    }
  },

 processIncomingMessage: function( messageElem ) {
    dump( "in processIncomingMessage: messageElem is a " + messageElem + "\n" );
    var from = messageElem.getAttribute( "from" );
    var contentElem = messageElem.firstChild;
    
    while ( contentElem.nodeType != 3 ) {
      contentElem = contentElem.firstChild;
    }
    dump( "Incoming message to you from " + from + ":\n" );
    dump( contentElem.nodeValue );
    for ( var x in this._messageHandlers ) {
      
      
      this._messageHandlers[x].handle( contentElem.nodeValue, from );
    }
  },

 processIncomingIq: function( iqElem ) {
    



    var buddy = iqElem.getAttribute( "from " );
    var id = iqElem.getAttribute( id );

    switch( iqElem.getAttribute( "type" ) ) {
    case "get":
      


      var variable = iqElem.firstChild.firstChild.getAttribute( "var" );
      
      
      var value = this._iqResponders[0].get( variable );
      var query = "<query><getresult value='" + value + "'/></query>";
      var xml = _makeIqXml( this._fullName, buddy, "result", id, query );
      this._transportLayer.send( xml );
    break;
    case "set":
      



      var variable = iqElem.firstChild.firstChild.getAttribute( "var" );
      var newValue = iqElem.firstChild.firstChildgetAttribute( "value" );
      
      
      
      this._iqResponders[0].set( variable, value );
      var xml = _makeIqXml( this._fullName, buddy, "result", id, "<query/>" );
      this._transportLayer.send( xml );
    break;
    case "result":
      



      if ( this._pendingIqs[ id ] == undefined ) {
	this.setError( "Unexpected IQ reply id" + id );
	return;
      }
      



      var newValue = iqElem.firstChild.firstChild.getAttribute( "value" );
      if ( newValue != undefined ) {
	this._pendingIqs[ id ].value = newValue;
      } else {
	this._pendingIqs[ id ].value = true;
      }
      this._pendingIqs[ id ].status = this.IQ_OK;
      break;
    case "error":
      

      var elems = iqElem.getElementsByTagName( "error" );
      var errorNode = elems[0].firstChild;
      if ( errorNode.nodeValue != null ) {
	this.setError( errorNode.nodeValue );
      } else {
	this.setError( errorNode.nodeName );
      }
      if ( this._pendingIqs[ id ] != undefined ) {
	this._pendingIqs[ id ].status = this.IQ_ERROR;
      }
      break;
    }
  },

 registerMessageHandler: function( handlerObject ) {
    


    this._messageHandlers.push( handlerObject );
  },

 registerIQResponder: function( handlerObject ) {
    


    this._iqResponders.push( handlerObject );
  },
 
 onTransportError: function( errorText ) {
    this.setError( errorText );
  },
 
 connect: function( host ) {
    
    this._transportLayer.connect();
    this._transportLayer.setCallbackObject( this );
    this._transportLayer.send( this._makeHeaderXml( host ) );    

    this._connectionStatus = this.CALLED_SERVER;
    
    
  },

 _makeHeaderXml: function( recipient ) {
    return "<?xml version='1.0'?><stream:stream to='" + recipient + "' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' version='1.0'>";
  },

 _makeMessageXml: function( messageText, fullName, recipient ) {
    

    msgXml = "<message xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' from='" + fullName + "' to='" + recipient + "' xml:lang='en'><body>" + messageText + "</body></message>";
    dump( "Message xml: \n" );
    dump( msgXml );
    return msgXml;
  },

 _makePresenceXml: function( fullName ) {
    
    
    
    return "<presence from ='" + fullName + "'><show/></presence>";
  },
 
 _makeIqXml: function( fullName, recipient, type, id, query ) {
    






    
    return "<iq xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' from='" + fullName + "' to='" + recipient + "' type='" + type + "' id='" + id + "'>" + query + "</iq>";
  },

 _makeClosingXml: function () {
    return "</stream:stream>";
  },

 _generateIqId: function() {
    
    
    var id = "client_" + this._nextIqId;
    this._nextIqId = this._nextIqId + 1;
    return id;
  },

 _sendIq: function( recipient, query, type ) {
    var id = this._generateIqId();
    this._pendingIqs[ id ] = { status: this.IQ_WAIT };
    this._transportLayer.send( this._makeIqXml( this._fullName,
						recipient,
						type,
						id,
						query ) );
    




    var thread = this._threadManager.currentThread;
    while( this._pendingIqs[ id ].status == this.IQ_WAIT ) {
      thread.processNextEvent( true );
    }
    if ( this._pendingIqs[ id ].status == this.IQ_OK ) {
      return this._pendingIqs[ id ].value;
    } else if ( this._pendingIqs[ id ].status == this.IQ_ERROR ) {
      return false;
    }
    
  },

 iqGet: function( recipient, variable ) {
    var query = "<query><getvar var='" + variable + "'/></query>";
    return this._sendIq( recipient, query, "get" );
  },
 
 iqSet: function( recipient, variable, value ) {
    var query = "<query><setvar var='" + variable + "' value='" + value + "'/></query>";
    return this._sendIq( recipient, query, "set" );
  },

 sendMessage: function( recipient, messageText ) {
    
    
    var body = this._makeMessageXml( messageText, this._fullName, recipient );
    this._transportLayer.send( body );
  },

 announcePresence: function() {
    this._transportLayer.send( "<presence/>" );
  },

 subscribeForPresence: function( buddyId ) {
    
    
    
    
    
  },

 disconnect: function() {
    
    
    this._transportLayer.send( this._makeClosingXml() );
    this._transportLayer.disconnect();
  },

 waitForConnection: function( ) {
    var thread = this._threadManager.currentThread;
    while ( this._connectionStatus != this.CONNECTED &&
	    this._connectionStatus != this.FAILED ) {
      thread.processNextEvent( true );
    }
  },

 waitForDisconnect: function() {
    var thread = this._threadManager.currentThread;
    while ( this._connectionStatus == this.CONNECTED ) {
      thread.processNextEvent( true );
    }
  }

};




