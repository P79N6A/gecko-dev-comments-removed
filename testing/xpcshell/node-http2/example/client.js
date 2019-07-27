var fs = require('fs');
var path = require('path');
var http2 = require('..');


http2.globalAgent = new http2.Agent({
  log: require('../test/util').createLogger('client')
});


process.env.NODE_TLS_REJECT_UNAUTHORIZED = "0";


var url = process.argv.pop();
var request = process.env.HTTP2_PLAIN ? http2.raw.get(url) : http2.get(url);


request.on('response', function(response) {
  response.pipe(process.stdout);
  response.on('end', finish);
});


request.on('push', function(pushRequest) {
  var filename = path.join(__dirname, '/push-' + push_count);
  push_count += 1;
  console.error('Receiving pushed resource: ' + pushRequest.url + ' -> ' + filename);
  pushRequest.on('response', function(pushResponse) {
    pushResponse.pipe(fs.createWriteStream(filename)).on('finish', finish);
  });
});


var push_count = 0;
var finished = 0;
function finish() {
  finished += 1;
  if (finished === (1 + push_count)) {
    process.exit();
  }
}
