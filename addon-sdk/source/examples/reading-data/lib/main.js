


"use strict";

var self = require("sdk/self");
var { Panel } = require("sdk/panel");
var { ToggleButton } = require("sdk/ui");

function replaceMom(html) {
  return html.replace("World", "Mom");
}
exports.replaceMom = replaceMom;

exports.main = function(options, callbacks) {
  console.log("My ID is " + self.id);

  
  var helloHTML = self.data.load("sample.html");

  
  helloHTML = replaceMom(helloHTML);

  
  var myPanel = Panel({
    contentURL: "data:text/html," + helloHTML,
    onHide: handleHide
  });

  
  
  var button = ToggleButton({
    id: "test-widget",
    label: "Mom",
    icon: './mom.png',
    onChange: handleChange
  });

  
  
  if (options.staticArgs.quitWhenDone)
    callbacks.quit();
}

function handleChange(state) {
  if (state.checked) {
    myPanel.show({ position: button });
  }
}

function handleHide() {
  button.state('window', { checked: false });
}
