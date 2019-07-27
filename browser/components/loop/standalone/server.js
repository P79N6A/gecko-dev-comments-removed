



var express = require('express');
var app = express();

var port = process.env.PORT || 3000;
var loopServerPort = process.env.LOOP_SERVER_PORT || 5000;
var feedbackApiUrl = process.env.LOOP_FEEDBACK_API_URL ||
                     "https://input.allizom.org/api/v1/feedback";
var feedbackProductName = process.env.LOOP_FEEDBACK_PRODUCT_NAME || "Loop";

function getConfigFile(req, res) {
  "use strict";

  res.set('Content-Type', 'text/javascript');
  res.send([
    "var loop = loop || {};",
    "loop.config = loop.config || {};",
    "loop.config.serverUrl = 'http://localhost:" + loopServerPort + "';",
    "loop.config.feedbackApiUrl = '" + feedbackApiUrl + "';",
    "loop.config.feedbackProductName = '" + feedbackProductName + "';",
  ].join("\n"));
}

app.get('/content/config.js', getConfigFile);


app.use('/', express.static(__dirname + '/../'));



app.use('/', express.static(__dirname + '/content/'));
app.use('/shared', express.static(__dirname + '/../content/shared/'));
app.get('/config.js', getConfigFile);


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
