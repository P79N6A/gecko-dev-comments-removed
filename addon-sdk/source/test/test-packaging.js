



var url = require("sdk/url");
var file = require("sdk/io/file");
var {Cm,Ci} = require("chrome");
var options = require("@loader/options");

exports.testPackaging = function(test) {

  test.assertEqual(options.metadata.description,
                   "Add-on development made easy.",
                   "packaging metadata should be available");
};
