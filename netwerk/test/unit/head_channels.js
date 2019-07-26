


function read_stream(stream, count) {
  
  var wrapper =
      Components.classes["@mozilla.org/binaryinputstream;1"]
                .createInstance(Components.interfaces.nsIBinaryInputStream);
  wrapper.setInputStream(stream);
  


  var data = [];
  while (count > 0) {
    var bytes = wrapper.readByteArray(Math.min(65535, count));
    data.push(String.fromCharCode.apply(null, bytes));
    count -= bytes.length;
    if (bytes.length == 0)
      do_throw("Nothing read from input stream!");
  }
  return data.join('');
}

const CL_EXPECT_FAILURE = 0x1;
const CL_EXPECT_GZIP = 0x2;
const CL_EXPECT_3S_DELAY = 0x4;
const CL_SUSPEND = 0x8;
const CL_ALLOW_UNKNOWN_CL = 0x10;
const CL_EXPECT_LATE_FAILURE = 0x20;
const CL_FROM_CACHE = 0x40; 
const CL_NOT_FROM_CACHE = 0x80; 

const SUSPEND_DELAY = 3000;
















function ChannelListener(closure, ctx, flags) {
  this._closure = closure;
  this._closurectx = ctx;
  this._flags = flags;
}
ChannelListener.prototype = {
  _closure: null,
  _closurectx: null,
  _buffer: "",
  _got_onstartrequest: false,
  _got_onstoprequest: false,
  _contentLen: -1,
  _lastEvent: 0,

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIStreamListener) ||
        iid.equals(Components.interfaces.nsIRequestObserver) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  onStartRequest: function(request, context) {
    try {
      if (this._got_onstartrequest)
        do_throw("Got second onStartRequest event!");
      this._got_onstartrequest = true;
      this._lastEvent = Date.now();

      request.QueryInterface(Components.interfaces.nsIChannel);
      try {
        this._contentLen = request.contentLength;
      }
      catch (ex) {
        if (!(this._flags & (CL_EXPECT_FAILURE | CL_ALLOW_UNKNOWN_CL)))
          do_throw("Could not get contentLength");
      }
      if (this._contentLen == -1 && !(this._flags & (CL_EXPECT_FAILURE | CL_ALLOW_UNKNOWN_CL)))
        do_throw("Content length is unknown in onStartRequest!");

      if ((this._flags & CL_FROM_CACHE)) {
        request.QueryInterface(Ci.nsICachingChannel);
        if (!request.isFromCache()) {
          do_throw("Response is not from the cache (CL_FROM_CACHE)");
        }
      }
      if ((this._flags & CL_NOT_FROM_CACHE)) {
        request.QueryInterface(Ci.nsICachingChannel);
        if (request.isFromCache()) {
          do_throw("Response is from the cache (CL_NOT_FROM_CACHE)");
        }
      }

      if (this._flags & CL_SUSPEND) {
        request.suspend();
        do_timeout(SUSPEND_DELAY, function() { request.resume(); });
      }

    } catch (ex) {
      do_throw("Error in onStartRequest: " + ex);
    }
  },

  onDataAvailable: function(request, context, stream, offset, count) {
    try {
      let current = Date.now();

      if (!this._got_onstartrequest)
        do_throw("onDataAvailable without onStartRequest event!");
      if (this._got_onstoprequest)
        do_throw("onDataAvailable after onStopRequest event!");
      if (!request.isPending())
        do_throw("request reports itself as not pending from onDataAvailable!");
      if (this._flags & CL_EXPECT_FAILURE)
        do_throw("Got data despite expecting a failure");

      if (current - this._lastEvent >= SUSPEND_DELAY &&
          !(this._flags & CL_EXPECT_3S_DELAY))
       do_throw("Data received after significant unexpected delay");
      else if (current - this._lastEvent < SUSPEND_DELAY &&
               this._flags & CL_EXPECT_3S_DELAY)
        do_throw("Data received sooner than expected");
      else if (current - this._lastEvent >= SUSPEND_DELAY &&
               this._flags & CL_EXPECT_3S_DELAY)
        this._flags &= ~CL_EXPECT_3S_DELAY; 

      this._buffer = this._buffer.concat(read_stream(stream, count));
      this._lastEvent = current;
    } catch (ex) {
      do_throw("Error in onDataAvailable: " + ex);
    }
  },

  onStopRequest: function(request, context, status) {
    try {
      var success = Components.isSuccessCode(status);
      if (!this._got_onstartrequest)
        do_throw("onStopRequest without onStartRequest event!");
      if (this._got_onstoprequest)
        do_throw("Got second onStopRequest event!");
      this._got_onstoprequest = true;
      if ((this._flags & (CL_EXPECT_FAILURE | CL_EXPECT_LATE_FAILURE)) && success)
        do_throw("Should have failed to load URL (status is " + status.toString(16) + ")");
      else if (!(this._flags & (CL_EXPECT_FAILURE | CL_EXPECT_LATE_FAILURE)) && !success)
        do_throw("Failed to load URL: " + status.toString(16));
      if (status != request.status)
        do_throw("request.status does not match status arg to onStopRequest!");
      if (request.isPending())
        do_throw("request reports itself as pending from onStopRequest!");
      if (!(this._flags & (CL_EXPECT_FAILURE | CL_EXPECT_LATE_FAILURE)) &&
          !(this._flags & CL_EXPECT_GZIP) &&
          this._contentLen != -1)
          do_check_eq(this._buffer.length, this._contentLen)
    } catch (ex) {
      do_throw("Error in onStopRequest: " + ex);
    }
    try {
      this._closure(request, this._buffer, this._closurectx);
    } catch (ex) {
      do_throw("Error in closure function: " + ex);
    }
  }
};

var ES_ABORT_REDIRECT = 0x01;

function ChannelEventSink(flags)
{
  this._flags = flags;
}

ChannelEventSink.prototype = {
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIInterfaceRequestor) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  getInterface: function(iid) {
    if (iid.equals(Ci.nsIChannelEventSink))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  asyncOnChannelRedirect: function(oldChannel, newChannel, flags, callback) {
    if (this._flags & ES_ABORT_REDIRECT)
      throw Cr.NS_BINDING_ABORTED;

    callback.onRedirectVerifyCallback(Cr.NS_OK);
  }
};






function LoadContextCallback(appId, inBrowserElement, isPrivate, isContent) {
  this.appId = appId;
  this.isInBrowserElement = inBrowserElement;
  this.usePrivateBrowsing = isPrivate;
  this.isContent = isContent;
}

LoadContextCallback.prototype = {
  associatedWindow: null,
  topWindow : null,
  isAppOfType: function(appType) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
  QueryInterface: function(iid) {
    if (iid == Ci.nsILoadContext ||
               Ci.nsIInterfaceRequestor ||
               Ci.nsISupports) {
        return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
  getInterface: function(iid) {
    if (iid.equals(Ci.nsILoadContext))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
}

