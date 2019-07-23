


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

      request.QueryInterface(Components.interfaces.nsIChannel);
      this._contentLen = request.contentLength;
      if (this._contentLen == -1)
        do_throw("Content length is unknown in onStartRequest!");
    } catch (ex) {
      do_throw("Error in onStartRequest: " + ex);
    }
  },

  onDataAvailable: function(request, context, stream, offset, count) {
    try {
      if (!this._got_onstartrequest)
        do_throw("onDataAvailable without onStartRequest event!");
      if (this._got_onstoprequest)
        do_throw("onDataAvailable after onStopRequest event!");
      if (!request.isPending())
        do_throw("request reports itself as not pending from onDataAvailable!");
      if (this._flags & CL_EXPECT_FAILURE)
        do_throw("Got data despite expecting a failure");

      this._buffer = this._buffer.concat(read_stream(stream, count));
    } catch (ex) {
      do_throw("Error in onDataAvailable: " + ex);
    }
  },

  onStopRequest: function(request, context, status) {
    try {
      if (!this._got_onstartrequest)
        do_throw("onStopRequest without onStartRequest event!");
      if (this._got_onstoprequest)
        do_throw("Got second onStopRequest event!");
      this._got_onstoprequest = true;
      var success = Components.isSuccessCode(status);
      if ((this._flags & CL_EXPECT_FAILURE) && success)
        do_throw("Should have failed to load URL (status is " + status.toString(16) + ")");
      else if (!(this._flags & CL_EXPECT_FAILURE) && !success)
        do_throw("Failed to load URL: " + status.toString(16));
      if (status != request.status)
        do_throw("request.status does not match status arg to onStopRequest!");
      if (request.isPending())
        do_throw("request reports itself as pending from onStopRequest!");
      if (!(this._flags & CL_EXPECT_FAILURE) &&
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
