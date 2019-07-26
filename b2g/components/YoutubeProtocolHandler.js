



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "cpmm", function() {
  return Cc["@mozilla.org/childprocessmessagemanager;1"]
           .getService(Ci.nsIMessageSender);
});

function YoutubeProtocolHandler() {
}

YoutubeProtocolHandler.prototype = {
  classID: Components.ID("{c3f1b945-7e71-49c8-95c7-5ae9cc9e2bad}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIProtocolHandler]),

  scheme: "vnd.youtube",
  defaultPort: -1,
  protocolFlags: Ci.nsIProtocolHandler.URI_NORELATIVE |
                 Ci.nsIProtocolHandler.URI_NOAUTH |
                 Ci.nsIProtocolHandler.URI_LOADABLE_BY_ANYONE,

  
  
  newURI: function yt_phNewURI(aSpec, aOriginCharset, aBaseURI) {
    let uri = Cc["@mozilla.org/network/standard-url;1"]
              .createInstance(Ci.nsIStandardURL);
    let id = aSpec.substring(this.scheme.length + 1);
    id = id.substring(0, id.indexOf('?'));
    uri.init(Ci.nsIStandardURL.URLTYPE_STANDARD, -1, this.scheme + "://dummy_host/" + id, aOriginCharset,
             aBaseURI);
    return uri.QueryInterface(Ci.nsIURI);
  },

  newChannel: function yt_phNewChannel(aURI) {
    
    let infoURI = "http://www.youtube.com/get_video_info?&video_id=" +
                  aURI.path.substring(1);

    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                .createInstance(Ci.nsIXMLHttpRequest);
    xhr.open("GET", infoURI, true);
    xhr.addEventListener("load", function() {
      try {
        let info = parseYoutubeVideoInfo(xhr.responseText);
        cpmm.sendAsyncMessage("content-handler", info);
      }
      catch(e) {
        
        
        
        
        log(e.message);
      }
    });
    xhr.send(null);

    function log(msg) {
      msg = "YoutubeProtocolHandler.js: " + (msg.join ? msg.join(" ") : msg);
      Cc["@mozilla.org/consoleservice;1"]
        .getService(Ci.nsIConsoleService)
        .logStringMessage(msg);
    }

    
    
    
    
    
    
    
    
    function parseYoutubeVideoInfo(response) {
      
      function extractParameters(q) {
        let params = q.split("&");
        let result = {};
        for(let i = 0, n = params.length; i < n; i++) {
          let param = params[i];
          let pos = param.indexOf('=');
          if (pos === -1) 
            continue;
          let name = param.substring(0, pos);
          let value = param.substring(pos+1);
          result[name] = decodeURIComponent(value);
        }
        return result;
      }

      let params = extractParameters(response);
      
      
      
      if (params.status === 'fail') {
        
        
        
        
        
        
        
        
        
        
        return {
          status: params.status,
          errorcode: params.errorcode,
          reason:  (params.reason || '').replace(/\+/g, ' '),
          type: 'video/3gpp',
          url: 'https://m.youtube.com'
        }
      }

      
      let result = {
        status: params.status,
      };

      
      let streamsText = params.url_encoded_fmt_stream_map;
      if (!streamsText)
        throw Error("No url_encoded_fmt_stream_map parameter");
      let streams = streamsText.split(',');
      for(let i = 0, n = streams.length; i < n; i++) {
        streams[i] = extractParameters(streams[i]);
      }

      
      
      
      
      let formats = [
        "17", 
        "36", 
        "43", 
#ifdef MOZ_WIDGET_GONK
        "18", 
#endif
      ];

      
      
      streams.sort(function(a, b) {
        let x = a.itag ? formats.indexOf(a.itag) : -1;
        let y = b.itag ? formats.indexOf(b.itag) : -1;
        return y - x;
      });

      let bestStream = streams[0];

      
      if (formats.indexOf(bestStream.itag) === -1) 
        throw Error("No supported video formats");

      result.url = bestStream.url + '&signature=' + (bestStream.sig || '');
      result.type = bestStream.type;
      
      if (result.type && result.type.indexOf(';') !== -1) {
        result.type = result.type.split(';',1)[0];
      }

      if (params.title) {
        result.title = params.title.replace(/\+/g, ' ');
      }
      if (params.length_seconds) {
        result.duration = params.length_seconds;
      }
      if (params.thumbnail_url) {
        result.poster = params.thumbnail_url;
      }

      return result;
    }

    throw Components.results.NS_ERROR_ILLEGAL_VALUE;
  },

  allowPort: function yt_phAllowPort(aPort, aScheme) {
    return false;
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([YoutubeProtocolHandler]);
