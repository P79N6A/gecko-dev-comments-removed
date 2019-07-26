



var data = require("sdk/self").data;

var reddit_panel = require("sdk/panel").Panel({
  width: 240,
  height: 320,
  contentURL: "http://www.reddit.com/.mobile?keep_extension=True",
  contentScriptFile: [data.url("jquery-1.4.4.min.js"),
                      data.url("panel.js")]
});

reddit_panel.port.on("click", function(url) {
  require("sdk/tabs").open(url);
});

require("sdk/widget").Widget({
  id: "open-reddit-btn",
  label: "Reddit",
  contentURL: "http://www.reddit.com/static/favicon.ico",
  panel: reddit_panel
});

exports.main = function(options, callbacks) {
  
  
  if (options.staticArgs.quitWhenDone)
    callbacks.quit();
};
