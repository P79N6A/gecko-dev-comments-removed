var spdy = exports;


spdy.utils = require('./spdy/utils');


spdy.protocol = {};

try {
  spdy.protocol.generic = require('./spdy/protocol/generic.node');
} catch (e) {
  spdy.protocol.generic = require('./spdy/protocol/generic.js');
}


spdy.protocol[2] = require('./spdy/protocol/v2');

spdy.parser = require('./spdy/parser');


spdy.response = require('./spdy/response');


spdy.scheduler = require('./spdy/scheduler');


spdy.zlibpool = require('./spdy/zlib-pool');


spdy.server = require('./spdy/server');
spdy.createServer = spdy.server.create;
