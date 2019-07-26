


"use strict";

var m = require("main");
var self = require("sdk/self");

exports.testMain = function(test) {
  var callbacks = { quit: function() {
    test.pass();
    test.done();
  } };

  test.waitUntilDone();
  
  m.main({ staticArgs: {quitWhenDone: true} }, callbacks);
};

exports.testData = function(test) {
  test.assert(self.data.load("panel.js").length > 0);
};
