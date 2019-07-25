if(typeof(atob) == 'undefined') {




var keyStr = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

function btoa(input) {
   var output = "";
   var chr1, chr2, chr3;
   var enc1, enc2, enc3, enc4;
   var i = 0;

   do {
      chr1 = input.charCodeAt(i++);
      chr2 = input.charCodeAt(i++);
      chr3 = input.charCodeAt(i++);

      enc1 = chr1 >> 2;
      enc2 = ((chr1 & 3) << 4) | (chr2 >> 4);
      enc3 = ((chr2 & 15) << 2) | (chr3 >> 6);
      enc4 = chr3 & 63;

      if (isNaN(chr2)) {
         enc3 = enc4 = 64;
      } else if (isNaN(chr3)) {
         enc4 = 64;
      }

      output = output + keyStr.charAt(enc1) + keyStr.charAt(enc2) + 
         keyStr.charAt(enc3) + keyStr.charAt(enc4);
   } while (i < input.length);
   
   return output;
}

function atob(input) {
   var output = "";
   var chr1, chr2, chr3;
   var enc1, enc2, enc3, enc4;
   var i = 0;

   
   input = input.replace(/[^A-Za-z0-9\+\/\=]/g, "");

   do {
      enc1 = keyStr.indexOf(input.charAt(i++));
      enc2 = keyStr.indexOf(input.charAt(i++));
      enc3 = keyStr.indexOf(input.charAt(i++));
      enc4 = keyStr.indexOf(input.charAt(i++));

      chr1 = (enc1 << 2) | (enc2 >> 4);
      chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
      chr3 = ((enc3 & 3) << 6) | enc4;

      output = output + String.fromCharCode(chr1);

      if (enc3 != 64) {
         output = output + String.fromCharCode(chr2);
      }
      if (enc4 != 64) {
         output = output + String.fromCharCode(chr3);
      }
   } while (i < input.length);

   return output;
}
}















function BaseAuthenticator() {
}
BaseAuthenticator.prototype = {
 COMPLETION_CODE: "success!",

 initialize: function( userName, realm, password  ) {
    this._name = userName;
    this._realm = realm;
    this._password = password;
    this._stepNumber = 0;
    this._errorMsg = "";
  },

 getError: function () {
    


    return this._errorMsg;
  },

 generateResponse: function( rootElem ) {
    






    this._errorMsg = "generateResponse() should be overridden by subclass.";
    return false;
  },
 
 verifyProtocolSupport: function( rootElem, protocolName ) {
    




    if ( rootElem.nodeName != "stream:stream" ) {
      this._errorMsg = "Expected stream:stream but got " + rootElem.nodeName;
      return false;
    }
      
    dump( "Got response from server...\n" );
    dump( "ID is " + rootElem.getAttribute( "id" ) + "\n" );
    
    dump( "From: " + rootElem.getAttribute( "from" ) + "\n" );
    if (rootElem.childNodes.length == 0) {
      
      
      this._errorMsg = "Expected child nodes but got none.";
      return false;
    }

    var child = rootElem.childNodes[0];
    if (child.nodeName == "stream:error" ) {
      this._errorMsg = this.parseError( child );
      return false;
    }

    if ( child.nodeName != "stream:features" ) {
      this._errorMsg = "Expected stream:features but got " + child.nodeName;
      return false;
    }
      
    var protocolSupported = false;
    var mechanisms = child.getElementsByTagName( "mechanism" );
    for ( var x = 0; x < mechanisms.length; x++ ) {
      if ( mechanisms[x].firstChild.nodeValue == protocolName ) {
	protocolSupported = true;
      }
    }
      
    if ( !protocolSupported ) {
      this._errorMsg = protocolName + " not supported by server!";
      return false;
    }
    return true;
  }

};

function Md5DigestAuthenticator( ) {
  










}
Md5DigestAuthenticator.prototype = {

 _makeCNonce: function( ) {
    return "\"" + Math.floor( 10000000000 * Math.random() ) + "\"";
  },
 
 generateResponse: function Md5__generateResponse( rootElem ) {
    if ( this._stepNumber == 0 ) {

      if ( this.verifyProtocolSupport( rootElem, "DIGEST-MD5" ) == false ) {
	return false;
      }
      
      this._stepNumber = 1;
      return "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='DIGEST-MD5'/>";

    } else if ( this._stepNumber == 1 ) {
     
      
      var challenge = this._unpackChallenge( rootElem.firstChild.nodeValue );
      dump( "Nonce is " + challenge.nonce + "\n" );
      
      

      
      


















      
      
      var nonceCount = "00000001";
      var digestUri = "xmpp/" + this.realm;
      var cNonce = this._makeCNonce();
    
      var A1 = str_md5( this.name + ":" + this.realm + ":" + this.password ) + ":" + challenge.nonce + ":" + cNonce;
      var A2 = "AUTHENTICATE:" + digestUri;
      var myResponse = hex_md5( hex_md5( A1 ) + ":" + challenge.nonce + ":" + nonceCount + ":" + cNonce + ":auth" + hex_md5( A2 ) );

      var responseDict = {
      username: "\"" + this.name + "\"",
      nonce: challenge.nonce,
      nc: nonceCount,
      cnonce: cNonce,
      qop: "\"auth\"",
      algorithm: "md5-sess",
      charset: "utf-8",
      response: myResponse
      };
      responseDict[ "digest-uri" ] = "\"" + digestUri + "\"";
      var responseStr = this._packChallengeResponse( responseDict );
      this._stepNumber = 2;
      return "<response xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>" + responseStr + "</response>";

    } else if ( this._stepNumber = 2 ) {
      dump( "Got to step 3!" );
      
      
      if ( rootElem.nodeName == "failure" ) {
	this._errorMsg = rootElem.firstChild.nodeName;
	return false;
      }
      
    }
    this._errorMsg = "Can't happen.";
    return false;
  },

 _unpackChallenge: function( challengeString ) {
    var challenge = atob( challengeString );
    dump( "After b64 decoding: " + challenge + "\n" );
    var challengeItemStrings = challenge.split( "," );
    var challengeItems = {};
    for ( var x in challengeItemStrings ) {
      var stuff = challengeItemStrings[x].split( "=" );
      challengeItems[ stuff[0] ] = stuff[1];
    }
    return challengeItems;
  },

 _packChallengeResponse: function( responseDict ) {
    var responseArray = []
    for( var x in responseDict ) {
      responseArray.push( x + "=" + responseDict[x] );
    }
    var responseString = responseArray.join( "," );
    dump( "Here's my response string: \n" );
    dump( responseString + "\n" );
    return btoa( responseString );
  }
};
Md5DigestAuthenticator.prototype.__proto__ = new BaseAuthenticator();


function PlainAuthenticator( ) {
  
}
PlainAuthenticator.prototype = {
 
 generateResponse: function( rootElem ) {
    if ( this._stepNumber == 0 ) {
      if ( this.verifyProtocolSupport( rootElem, "PLAIN" ) == false ) {
	return false;
      }
      var authString = btoa( this._realm + '\0' + this._name + '\0' + this._password );
      this._stepNumber = 1;
      return "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='PLAIN'>" + authString + "</auth>";
    } else if ( this._stepNumber == 1 ) {
      if ( rootElem.nodeName == "failure" ) {
	
	this._errorMsg = rootElem.firstChild.nodeName;
	return false;
      } else if ( rootElem.nodeName == "success" ) {
	
	
	



	
	
	this._stepNumber = 2;
	return "<?xml version='1.0'?><stream:stream to='jonathan-dicarlos-macbook-pro.local' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' version='1.0'>";
      }
    } else if ( this._stepNumber == 2 ) {
      
      
      var bindNodes = rootElem.getElementsByTagName( "bind" );
      if ( bindNodes.length > 0 ) {
	this._needBinding = true;
      }
      var sessionNodes = rootElem.getElementsByTagName( "session" );
      if ( sessionNodes.length > 0 ) {
	this._needSession = true;
      }

      if ( !this._needBinding && !this._needSession ) {
	
	return this.COMPLETION_CODE;
      }
      
      if ( this._needBinding ) {
	
	
	this._stepNumber = 3;
	return "<iq type='set' id='bind_1'><bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'/></iq>";
      } 
      
      this._errorMsg = "Server requested session not binding: can't happen?";
      return false;
    } else if ( this._stepNumber == 3 ) {
      
      var jidNodes = rootElem.getElementsByTagName( "jid" );
      if ( jidNodes.length == 0 ) {
	this._errorMsg = "Expected JID node from server, got none.";
	return false;
      }
      this._jid = jidNodes[0].firstChild.nodeValue;
      
      
      dump( "JID set to " + this._jid );

      
      if ( this._needSession ) {
	this._stepNumber = 4;
	return "<iq to='" + this._realm + "' type='set' id='sess_1'><session xmlns='urn:ietf:params:xml:ns:xmpp-session'/></iq>";
      } else {
	return this.COMPLETION_CODE;
      }
    } else if ( this._stepNumber == 4 ) {
      
      return this.COMPLETION_CODE;
    }
  }

};
PlainAuthenticator.prototype.__proto__ = new BaseAuthenticator();
