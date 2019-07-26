



Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/vtt.jsm");

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
    this.parser = new WebVTTParser(window,  new TextDecoder("utf8"));
  },

  parse: function(data, count)
  {
    
    
    var buffer = new Uint8Array(count);
    for (var i = 0; i < count; i++) {
      buffer[i] = data.charCodeAt(i);
    }

    this.parser.parse(buffer);
  },

  flush: function()
  {
    this.parser.flush();
  },

  watch: function(callback)
  {
    this.parser.oncue = callback.onCue;
    this.parser.onregion = callback.onRegion;
  },

  convertCueToDOMTree: function(window, cue)
  {
    return WebVTTParser.convertCueToDOMTree(window, cue.text);
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
