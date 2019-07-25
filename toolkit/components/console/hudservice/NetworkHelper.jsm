




















































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "NetUtil", function () {
  var obj = {};
  Cu.import("resource://gre/modules/NetUtil.jsm", obj);
  return obj.NetUtil;
});

var EXPORTED_SYMBOLS = ["NetworkHelper"];








var NetworkHelper =
{
  









  convertToUnicode: function NH_convertToUnicode(aText, aCharset)
  {
    if (!aCharset) {
      return aText;
    }

    let conv = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
               createInstance(Ci.nsIScriptableUnicodeConverter);
    conv.charset = aCharset;

    try {
      return conv.ConvertToUnicode(aText);
    }
    catch (ex) {
      Cu.reportError("NH_convertToUnicode(aText, '" +
        aCharset + "') exception: " + ex);
      return aText;
    }
  },

  







  readAndConvertFromStream: function NH_readAndConvertFromStream(aStream, aCharset)
  {
    let text = null;
    try {
      text = NetUtil.readInputStreamToString(aStream, aStream.available())
      return this.convertToUnicode(text, aCharset);
    }
    catch (err) {
      return text;
    }
  },

   








  readPostTextFromRequest: function NH_readPostTextFromRequest(aRequest, aBrowser)
  {
    if (aRequest instanceof Ci.nsIUploadChannel) {
      let iStream = aRequest.uploadStream;

      let isSeekableStream = false;
      if (iStream instanceof Ci.nsISeekableStream) {
        isSeekableStream = true;
      }

      let prevOffset;
      if (isSeekableStream) {
        prevOffset = iStream.tell();
        iStream.seek(Ci.nsISeekableStream.NS_SEEK_SET, 0);
      }

      
      let charset = aBrowser.contentWindow.document.characterSet;
      let text = this.readAndConvertFromStream(iStream, charset);

      
      
      
      if (isSeekableStream && prevOffset == 0) {
        iStream.seek(Ci.nsISeekableStream.NS_SEEK_SET, 0);
      }
      return text;
    }
    return null;
  },

  







  readPostTextFromPage: function NH_readPostTextFromPage(aBrowser)
  {
    let webNav = aBrowser.webNavigation;
    if (webNav instanceof Ci.nsIWebPageDescriptor) {
      let descriptor = webNav.currentDescriptor;

      if (descriptor instanceof Ci.nsISHEntry && descriptor.postData &&
          descriptor instanceof Ci.nsISeekableStream) {
        descriptor.seek(NS_SEEK_SET, 0);

        let charset = browser.contentWindow.document.characterSet;
        return this.readAndConvertFromStream(descriptor, charset);
      }
    }
    return null;
  },

  





  getWindowForRequest: function NH_getWindowForRequest(aRequest)
  {
    let loadContext = this.getRequestLoadContext(aRequest);
    if (loadContext) {
      return loadContext.associatedWindow;
    }
    return null;
  },

  





  getRequestLoadContext: function NH_getRequestLoadContext(aRequest)
  {
    if (aRequest && aRequest.notificationCallbacks) {
      try {
        return aRequest.notificationCallbacks.getInterface(Ci.nsILoadContext);
      } catch (ex) { }
    }

    if (aRequest && aRequest.loadGroup
                 && aRequest.loadGroup.notificationCallbacks) {
      try {
        return aRequest.loadGroup.notificationCallbacks.getInterface(Ci.nsILoadContext);
      } catch (ex) { }
    }

    return null;
  },

  











  loadFromCache: function NH_loadFromCache(aUrl, aCharset, aCallback)
  {
    let channel = NetUtil.newChannel(aUrl);

    
    channel.loadFlags = Ci.nsIRequest.LOAD_FROM_CACHE |
      Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE |
      Ci.nsICachingChannel.LOAD_BYPASS_LOCAL_CACHE_IF_BUSY;

    NetUtil.asyncFetch(channel, function (aInputStream, aStatusCode, aRequest) {
      if (!Components.isSuccessCode(aStatusCode)) {
        aCallback(null);
        return;
      }

      
      
      let aChannel = aRequest.QueryInterface(Ci.nsIChannel);
      let contentCharset = aChannel.contentCharset || aCharset;

      
      aCallback(NetworkHelper.readAndConvertFromStream(aInputStream,
                                                       contentCharset));
    });
  },

  
  
  mimeCategoryMap: {
    "text/plain": "txt",
    "text/html": "html",
    "text/xml": "xml",
    "text/xsl": "txt",
    "text/xul": "txt",
    "text/css": "css",
    "text/sgml": "txt",
    "text/rtf": "txt",
    "text/x-setext": "txt",
    "text/richtext": "txt",
    "text/javascript": "js",
    "text/jscript": "txt",
    "text/tab-separated-values": "txt",
    "text/rdf": "txt",
    "text/xif": "txt",
    "text/ecmascript": "js",
    "text/vnd.curl": "txt",
    "text/x-json": "json",
    "text/x-js": "txt",
    "text/js": "txt",
    "text/vbscript": "txt",
    "view-source": "txt",
    "view-fragment": "txt",
    "application/xml": "xml",
    "application/xhtml+xml": "xml",
    "application/atom+xml": "xml",
    "application/rss+xml": "xml",
    "application/vnd.mozilla.maybe.feed": "xml",
    "application/vnd.mozilla.xul+xml": "xml",
    "application/javascript": "js",
    "application/x-javascript": "js",
    "application/x-httpd-php": "txt",
    "application/rdf+xml": "xml",
    "application/ecmascript": "js",
    "application/http-index-format": "txt",
    "application/json": "json",
    "application/x-js": "txt",
    "multipart/mixed": "txt",
    "multipart/x-mixed-replace": "txt",
    "image/svg+xml": "svg",
    "application/octet-stream": "bin",
    "image/jpeg": "image",
    "image/jpg": "image",
    "image/gif": "image",
    "image/png": "image",
    "image/bmp": "image",
    "application/x-shockwave-flash": "flash",
    "video/x-flv": "flash",
    "audio/mpeg3": "media",
    "audio/x-mpeg-3": "media",
    "video/mpeg": "media",
    "video/x-mpeg": "media",
    "audio/ogg": "media",
    "application/ogg": "media",
    "application/x-ogg": "media",
    "application/x-midi": "media",
    "audio/midi": "media",
    "audio/x-mid": "media",
    "audio/x-midi": "media",
    "music/crescendo": "media",
    "audio/wav": "media",
    "audio/x-wav": "media",
    "text/json": "json",
    "application/x-json": "json",
    "application/json-rpc": "json"
  }
}
