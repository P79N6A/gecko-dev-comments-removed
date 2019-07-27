var spdy = exports;


spdy.utils = require('./spdy/utils');


spdy.protocol = require('./spdy/protocol');


spdy.response = require('./spdy/response');


spdy.scheduler = require('./spdy/scheduler');


spdy.zlibpool = require('./spdy/zlib-pool');


spdy.Stream = require('./spdy/stream').Stream;
spdy.Connection = require('./spdy/connection').Connection;


spdy.server = require('./spdy/server');
spdy.Server = spdy.server.Server;
spdy.createServer = spdy.server.create;


spdy.Agent = require('./spdy/client').Agent;
spdy.createAgent = require('./spdy/client').create;
