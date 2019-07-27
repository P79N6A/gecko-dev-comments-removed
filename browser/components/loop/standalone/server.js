










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
    "loop.config.serverUrl = 'http://localhost:" + loopServerPort + "/v0';",
    "loop.config.feedbackApiUrl = '" + feedbackApiUrl + "';",
    "loop.config.feedbackProductName = '" + feedbackProductName + "';",
    
    
    "loop.config.marketplaceUrl = 'http://fake-market.herokuapp.com/iframe-install.html'",
    "loop.config.downloadFirefoxUrl = 'https://www.mozilla.org/firefox/new/?scene=2&utm_source=hello.firefox.com&utm_medium=referral&utm_campaign=non-webrtc-browser#download-fx';",
    "loop.config.privacyWebsiteUrl = 'https://www.mozilla.org/privacy/firefox-hello/';",
    "loop.config.learnMoreUrl = 'https://www.mozilla.org/hello/';",
    "loop.config.legalWebsiteUrl = 'https://www.mozilla.org/about/legal/terms/firefox-hello/';",
    "loop.config.fxosApp = loop.config.fxosApp || {};",
    "loop.config.fxosApp.name = 'Loop';",
    "loop.config.fxosApp.rooms = true;",
    "loop.config.fxosApp.manifestUrl = 'http://fake-market.herokuapp.com/apps/packagedApp/manifest.webapp';",
    "loop.config.roomsSupportUrl = 'https://support.mozilla.org/kb/group-conversations-firefox-hello-webrtc';",
    "loop.config.guestSupportUrl = 'https://support.mozilla.org/kb/respond-firefox-hello-invitation-guest-mode';",
    "loop.config.generalSupportUrl = 'https://support.mozilla.org/kb/respond-firefox-hello-invitation-guest-mode';",
    "loop.config.unsupportedPlatformUrl = 'https://support.mozilla.org/en-US/kb/which-browsers-will-work-firefox-hello-video-chat'"
  ].join("\n"));
}

app.get('/content/config.js', getConfigFile);
app.get('/content/c/config.js', getConfigFile);






app.use('/test', express.static(__dirname + '/../test'));
app.use('/ui', express.static(__dirname + '/../ui'));



app.use('/standalone/content', express.static(__dirname + '/content'));





app.use('/content', express.static(__dirname + '/content'));
app.use('/content', express.static(__dirname + '/../content'));

app.use('/content/c', express.static(__dirname + '/content'));
app.use('/content/c', express.static(__dirname + '/../content'));



function serveIndex(req, res) {
  return res.sendfile(__dirname + '/content/index.html');
}

app.get(/^\/content\/[\w\-]+$/, serveIndex);
app.get(/^\/content\/c\/[\w\-]+$/, serveIndex);

var server = app.listen(port);

var baseUrl = "http://localhost:" + port + "/";

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
