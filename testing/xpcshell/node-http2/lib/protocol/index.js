






































exports.VERSION = 'h2-16';

exports.Endpoint = require('./endpoint').Endpoint;


exports.serializers = {};
var modules = ['./framer', './compressor', './flow', './connection', './stream', './endpoint'];
modules.map(require).forEach(function(module) {
  for (var name in module.serializers) {
    exports.serializers[name] = module.serializers[name];
  }
});









































