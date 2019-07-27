



var express = require('express');
var app = express();

var port = process.env.PORT || 3000;
var loopServerPort = process.env.LOOP_SERVER_PORT || 5000;

app.get('/content/config.js', function (req, res) {
  "use strict";

  res.set('Content-Type', 'text/javascript');
  res.send(
    "var loop = loop || {};" +
    "loop.config = loop.config || {};" +
    "loop.config.serverUrl = 'http://localhost:" + loopServerPort + "';"
  );

});


app.use('/', express.static(__dirname + '/../'));

app.use('/', express.static(__dirname + '/'));

app.use('/standalone/content', express.static(__dirname + '/../content'));

var server = app.listen(port);

var baseUrl = "http://localhost:" + port + "/";

console.log("Serving repository root over HTTP at " + baseUrl);
console.log("Static contents are available at " + baseUrl + "content/");
console.log("Tests are viewable at " + baseUrl + "test/");
console.log("Use this for development only.");


function shutdown(cb) {
  "use strict";

  try {
    server.close(function () {
      process.exit(0);
      if (cb !== undefined) {
        cb();
      }
    });

  } catch (ex) {
    console.log(ex + " while calling server.close)");
  }
}

process.on('SIGTERM', shutdown);
