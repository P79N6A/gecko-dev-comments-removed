


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














function ChannelListener(closure, ctx) {
  this._closure = closure;
  this._closurectx = ctx;
}
ChannelListener.prototype = {
  _closure: null,
  _closurectx: null,
  _buffer: "",
  _got_onstartrequest: false,
  _got_onstoprequest: false,
  _contentLen: -1,

  QueryInterface: function(iid) {
    if (iid.Equals(Components.interfaces.nsIStreamChannelListener) ||
        iid.Equals(Components.interfaces.nsIRequestObserver) ||
        iid.Equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  onStartRequest: function(request, context) {
    if (this._got_onstartrequest)
      do_throw("Got second onStartRequest event!");
    this._got_onstartrequest = true;

    request.QueryInterface(Components.interfaces.nsIChannel);
    this._contentLen = request.contentLength;
    if (this._contentLen == -1)
      do_throw("Content length is unknown in onStartRequest!");
  },

  onDataAvailable: function(request, context, stream, offset, count) {
    if (!this._got_onstartrequest)
      do_throw("onDataAvailable without onStartRequest event!");
    if (this._got_onstoprequest)
      do_throw("onDataAvailable after onStopRequest event!");
    if (!request.isPending())
      do_throw("request reports itself as not pending from onStartRequest!");

    this._buffer = this._buffer.concat(read_stream(stream, count));
  },

  onStopRequest: function(request, context, status) {
    if (!this._got_onstartrequest)
      do_throw("onStopRequest without onStartRequest event!");
    if (this._got_onstoprequest)
      do_throw("Got second onStopRequest event!");
    this._got_onstoprequest = true;
    if (!Components.isSuccessCode(status))
      do_throw("Failed to load URL: " + status.toString(16));
    if (status != request.status)
      do_throw("request.status does not match status arg to onStopRequest!");
    if (request.isPending())
      do_throw("request reports itself as pending from onStopRequest!");
    if (this._contentLen != -1 && this._buffer.length != this._contentLen)
      do_throw("did not read nsIChannel.contentLength number of bytes!");

    this._closure(request, this._buffer, this._closurectx);
  }
};
