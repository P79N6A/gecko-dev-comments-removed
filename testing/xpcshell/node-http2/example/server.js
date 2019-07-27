var fs = require('fs');
var path = require('path');
var http2 = require('..');


var cachedFile = fs.readFileSync(path.join(__dirname, './server.js'));
var cachedUrl = '/server.js';


function onRequest(request, response) {
  var filename = path.join(__dirname, request.url);

  
  if (request.url === cachedUrl) {
    if (response.push) {
      
      
      var push = response.push('/client.js');
      push.writeHead(200);
      fs.createReadStream(path.join(__dirname, '/client.js')).pipe(push);
    }
    response.end(cachedFile);
  }

  
  else if ((filename.indexOf(__dirname) === 0) && fs.existsSync(filename) && fs.statSync(filename).isFile()) {
    response.writeHead('200');

    fs.createReadStream(filename).pipe(response);
  }

  
  else {
    response.writeHead('404');
    response.end();
  }
}


var log = require('../test/util').createLogger('server');


var server;
if (process.env.HTTP2_PLAIN) {
  server = http2.raw.createServer({
    log: log
  }, onRequest);
} else {
  server = http2.createServer({
    log: log,
    key: fs.readFileSync(path.join(__dirname, '/localhost.key')),
    cert: fs.readFileSync(path.join(__dirname, '/localhost.crt'))
  }, onRequest);
}
server.listen(process.env.HTTP2_PORT || 8080);
