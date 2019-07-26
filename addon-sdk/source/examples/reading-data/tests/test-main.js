



var m = require("main");
var self = require("self");

exports.testReplace = function(test) {
  var input = "Hello World";
  var output = m.replaceMom(input);
  test.assertEqual(output, "Hello Mom");
  var callbacks = { quit: function() {} };

  
  m.main({ staticArgs: {} }, callbacks);
};

exports.testID = function(test) {
  
  
  test.assert(self.id.length > 0);
  test.assertEqual(self.data.url("sample.html"),
                   "resource://reading-data-example-at-jetpack-dot-mozillalabs-dot-com/reading-data/data/sample.html");
};
