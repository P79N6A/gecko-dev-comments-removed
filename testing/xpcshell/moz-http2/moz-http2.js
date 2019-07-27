



var http2 = require('../node-http2');
var fs = require('fs');
var url = require('url');
var crypto = require('crypto');


var http2_compression = require('../node-http2/lib/protocol/compressor');
var HeaderSetDecompressor = http2_compression.HeaderSetDecompressor;
var originalRead = HeaderSetDecompressor.prototype.read;
var lastDecompressor;
var decompressedPairs;
HeaderSetDecompressor.prototype.read = function() {
  if (this != lastDecompressor) {
    lastDecompressor = this;
    decompressedPairs = [];
  }
  var pair = originalRead.apply(this, arguments);
  if (pair) {
    decompressedPairs.push(pair);
  }
  return pair;
}

var http2_connection = require('../node-http2/lib/protocol/connection');
var Connection = http2_connection.Connection;
var originalClose = Connection.prototype.close;
Connection.prototype.close = function (error, lastId) {
  if (lastId !== undefined) {
    this._lastIncomingStream = lastId;
  }

  originalClose.apply(this, arguments);
}

function getHttpContent(path) {
  var content = '<!doctype html>' +
                '<html>' +
                '<head><title>HOORAY!</title></head>' +
                '<body>You Win! (by requesting' + path + ')</body>' +
                '</html>';
  return content;
}

function generateContent(size) {
  var content = '';
  for (var i = 0; i < size; i++) {
    content += '0';
  }
  return content;
}


var m = {
  mp1res: null,
  mp2res: null,
  buf: null,
  mp1start: 0,
  mp2start: 0,

  checkReady: function() {
    if (this.mp1res != null && this.mp2res != null) {
      this.buf = generateContent(30*1024);
      this.mp1start = 0;
      this.mp2start = 0;
      this.send(this.mp1res, 0);
      setTimeout(this.send.bind(this, this.mp2res, 0), 5);
    }
  },

  send: function(res, start) {
    var end = Math.min(start + 1024, this.buf.length);
    var content = this.buf.substring(start, end);
    res.write(content);
    if (end < this.buf.length) {
      setTimeout(this.send.bind(this, res, end), 10);
    } else {
      res.end();
    }
  }
};

var h11required_conn = null;
var h11required_header = "yes";
var didRst = false;
var rstConnection = null;

function handleRequest(req, res) {
  var u = url.parse(req.url);
  var content = getHttpContent(u.pathname);
  var push, push1, push1a, push2, push3;

  if (req.httpVersionMajor === 2) {
    res.setHeader('X-Connection-Http2', 'yes');
    res.setHeader('X-Http2-StreamId', '' + req.stream.id);
  } else {
    res.setHeader('X-Connection-Http2', 'no');
  }

  if (u.pathname === '/exit') {
    res.setHeader('Content-Type', 'text/plain');
    res.writeHead(200);
    res.end('ok');
    process.exit();
  }

  else if ((u.pathname === '/multiplex1') && (req.httpVersionMajor === 2)) {
    res.setHeader('Content-Type', 'text/plain');
    res.writeHead(200);
    m.mp1res = res;
    m.checkReady();
    return;
  }

  else if ((u.pathname === '/multiplex2') && (req.httpVersionMajor === 2)) {
    res.setHeader('Content-Type', 'text/plain');
    res.writeHead(200);
    m.mp2res = res;
    m.checkReady();
    return;
  }

  else if (u.pathname === "/header") {
    var val = req.headers["x-test-header"];
    if (val) {
      res.setHeader("X-Received-Test-Header", val);
    }
  }

  else if (u.pathname === "/doubleheader") {
    res.setHeader('Content-Type', 'text/html');
    res.writeHead(200);
    res.write(content);
    res.writeHead(200);
    res.end();
    return;
  }

  else if (u.pathname === "/cookie_crumbling") {
    res.setHeader("X-Received-Header-Pairs", JSON.stringify(decompressedPairs));
  }

  else if (u.pathname === "/push") {
    push = res.push('/push.js');
    push.writeHead(200, {
      'content-type': 'application/javascript',
      'pushed' : 'yes',
      'content-length' : 11,
      'X-Connection-Http2': 'yes'
    });
    push.end('// comments');
    content = '<head> <script src="push.js"/></head>body text';
  }

  else if (u.pathname === "/push2") {
    push = res.push('/push2.js');
    push.writeHead(200, {
      'content-type': 'application/javascript',
      'pushed' : 'yes',
      
      'X-Connection-Http2': 'yes'
    });
    push.end('// comments');
    content = '<head> <script src="push2.js"/></head>body text';
  }

  else if (u.pathname === "/pushapi1") {
    push1 = res.push(
	{ hostname: 'localhost:6944', port: 6944, path : '/pushapi1/1', method : 'GET',
	  headers: {'x-pushed-request': 'true', 'x-foo' : 'bar'}});
    push1.writeHead(200, {
      'pushed' : 'yes',
      'content-length' : 1,
      'subresource' : '1',
      'X-Connection-Http2': 'yes'
      });
    push1.end('1');

    push1a = res.push(
	{ hostname: 'localhost:6944', port: 6944, path : '/pushapi1/1', method : 'GET',
	  headers: {'x-foo' : 'bar', 'x-pushed-request': 'true'}});
    push1a.writeHead(200, {
      'pushed' : 'yes',
      'content-length' : 1,
      'subresource' : '1a',
      'X-Connection-Http2': 'yes'
      });
    push1a.end('1');

    push2 = res.push(
	{ hostname: 'localhost:6944', port: 6944, path : '/pushapi1/2', method : 'GET',
	  headers: {'x-pushed-request': 'true'}});
    push2.writeHead(200, {
	  'pushed' : 'yes',
	  'subresource' : '2',
	  'content-length' : 1,
	  'X-Connection-Http2': 'yes'
      });
    push2.end('2');

    push3 = res.push(
	{ hostname: 'localhost:6944', port: 6944, path : '/pushapi1/3', method : 'GET',
	  headers: {'x-pushed-request': 'true'}});
    push3.writeHead(200, {
	  'pushed' : 'yes',
	  'content-length' : 1,
	  'subresource' : '3',
	  'X-Connection-Http2': 'yes'
      });
     push3.end('3');

    content = '0';
  }

  else if (u.pathname === "/big") {
    content = generateContent(128 * 1024);
    var hash = crypto.createHash('md5');
    hash.update(content);
    var md5 = hash.digest('hex');
    res.setHeader("X-Expected-MD5", md5);
  }

  else if (u.pathname === "/post") {
    if (req.method != "POST") {
      res.writeHead(405);
      res.end('Unexpected method: ' + req.method);
      return;
    }

    var post_hash = crypto.createHash('md5');
    req.on('data', function receivePostData(chunk) {
      post_hash.update(chunk.toString());
    });
    req.on('end', function finishPost() {
      var md5 = post_hash.digest('hex');
      res.setHeader('X-Calculated-MD5', md5);
      res.writeHead(200);
      res.end(content);
    });

    return;
  }

  else if (u.pathname === "/h11required_stream") {
    if (req.httpVersionMajor === 2) {
      h11required_conn = req.stream.connection;
      res.stream.reset('HTTP_1_1_REQUIRED');
      return;
    }
  }

  else if (u.pathname === "/h11required_session") {
    if (req.httpVersionMajor === 2) {
      if (h11required_conn !== req.stream.connection) {
        h11required_header = "no";
      }
      res.stream.connection.close('HTTP_1_1_REQUIRED', res.stream.id - 2);
      return;
    } else {
      res.setHeader('X-H11Required-Stream-Ok', h11required_header);
    }
  }

  else if (u.pathname === "/rstonce") {
    if (!didRst) {
      didRst = true;
      rstConnection = req.stream.connection;
      req.stream.reset('REFUSED_STREAM');
      return;
    }

    if (rstConnection === null ||
        rstConnection !== req.stream.connection) {
      res.writeHead(400);
      res.end("WRONG CONNECTION, HOMIE!");
      return;
    }

    res.writeHead(200);
    res.end("It's all good.");
    return;
  }

  res.setHeader('Content-Type', 'text/html');
  res.writeHead(200);
  res.end(content);
}


var options = {
  key: fs.readFileSync(__dirname + '/../moz-spdy/spdy-key.pem'),
  cert: fs.readFileSync(__dirname + '/../moz-spdy/spdy-cert.pem'),
  ca: fs.readFileSync(__dirname + '/../moz-spdy/spdy-ca.pem'),
  
};

var server = http2.createServer(options, handleRequest);
server.on('connection', function(socket) {
  socket.on('error', function() {
    
    
    
  });
});
server.listen(6944);
console.log('HTTP2 server listening on port 6944');
