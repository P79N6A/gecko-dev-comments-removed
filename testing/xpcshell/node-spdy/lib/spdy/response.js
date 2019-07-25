var spdy = require('../spdy'),
    http = require('http');






exports._renderHeaders = function() {
  if (this._header) {
    throw new Error("Can't render headers after they are sent to the client.");
  }

  if (!this._headers) return {};

  var headers = {};
  var keys = Object.keys(this._headers);
  for (var i = 0, l = keys.length; i < l; i++) {
    var key = keys[i];
    headers[(this._headerNames[key] || '').toLowerCase()] = this._headers[key];
  }
  return headers;
};







exports.writeHead = function(statusCode) {
  if (this._headerSent) return;
  this._headerSent = true;

  var reasonPhrase, headers, headerIndex;

  if (typeof arguments[1] == 'string') {
    reasonPhrase = arguments[1];
    headerIndex = 2;
  } else {
    reasonPhrase = http.STATUS_CODES[statusCode] || 'unknown';
    headerIndex = 1;
  }
  this.statusCode = statusCode;

  var obj = arguments[headerIndex];

  if (obj && this._headers) {
    
    headers = this._renderHeaders();

    
    var keys = Object.keys(obj);
    for (var i = 0; i < keys.length; i++) {
      var k = keys[i];
      if (k) headers[k] = obj[k];
    }
  } else if (this._headers) {
    
    headers = this._renderHeaders();
  } else {
    
    headers = obj;
  }

  
  this._header = '';

  
  if (this.socket.isGoaway()) return;

  this.socket.lock(function() {
    var socket = this;

    this.framer.replyFrame(
      this.id,
      statusCode,
      reasonPhrase,
      headers,
      function (err, frame) {
        
        socket.connection.write(frame);
        socket.unlock();
      }
    );
  });
};








exports.push = function push(url, headers, callback) {
  if (this.socket._destroyed) {
    return callback(Error('Can\'t open push stream, parent socket destroyed'));
  }

  this.socket.lock(function() {
    var socket = this,
        id = socket.connection.pushId += 2,
        fullUrl = /^\//.test(url) ?
                      this.frame.headers.scheme + '://' +
                      (this.frame.headers.host || 'localhost') +
                      url
                      :
                      url;

    this.framer.streamFrame(
      id,
      this.id,
      {
        method: 'GET',
        url: fullUrl,
        schema: 'https',
        version: 'HTTP/1.1'
      },
      headers,
      function(err, frame) {
        if (err) {
          socket.unlock();
          callback(err);
        } else {
          socket.connection.write(frame);
          socket.unlock();

          var stream = new spdy.server.Stream(socket.connection, {
            type: 'SYN_STREAM',
            push: true,
            id: id,
            assoc: socket.id,
            priority: 0,
            headers: {}
          });

          socket.connection.streams[id] = stream;
          socket.pushes.push(stream);

          callback(null, stream);
        }
      }
    );
  });
};
