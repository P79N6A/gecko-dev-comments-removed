



var self = require("sdk/self");
var panels = require("sdk/panel");
var widgets = require("sdk/widget");

function replaceMom(html) {
  return html.replace("World", "Mom");
}
exports.replaceMom = replaceMom;

exports.main = function(options, callbacks) {
  console.log("My ID is " + self.id);

  
  var helloHTML = self.data.load("sample.html");

  
  helloHTML = replaceMom(helloHTML);

  
  var myPanel = panels.Panel({
    contentURL: "data:text/html," + helloHTML
  });

  
  var iconURL = self.data.url("mom.png");

  
  
  widgets.Widget({
    id: "test-widget",
    label: "Mom",
    contentURL: iconURL,
    panel: myPanel
  });

  
  
  if (options.staticArgs.quitWhenDone)
    callbacks.quit();
}
