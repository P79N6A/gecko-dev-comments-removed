



Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


var Ci = Components.interfaces;

var WEBVTTPARSERWRAPPER_CID = "{1604a67f-3b72-4027-bcba-6dddd5be6b10}";
var WEBVTTPARSERWRAPPER_CONTRACTID = "@mozilla.org/webvttParserWrapper;1";

function WebVTTParserWrapper()
{
  
}

WebVTTParserWrapper.prototype =
{
  loadParser: function(window)
  {
    
  },

  parse: function(data, count)
  {
    
    
    var buffer = new Uint8Array(count);
    for (var i = 0; i < count; i++) {
      buffer[i] = data.charCodeAt(i);
    }

    
  },

  flush: function()
  {
    
  },

  watch: function(callback)
  {
    
  },

  convertCueToDOMTree: function(window, cue)
  {
    
  },

  classDescription: "Wrapper for the JS WebVTTParser (vtt.js)",
  classID: Components.ID(WEBVTTPARSERWRAPPER_CID),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebVTTParserWrapper]),
  classInfo: XPCOMUtils.generateCI({
    classID:    WEBVTTPARSERWRAPPER_CID,
    contractID: WEBVTTPARSERWRAPPER_CONTRACTID,
    interfaces: [Ci.nsIWebVTTParserWrapper]
  })
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([WebVTTParserWrapper]);
