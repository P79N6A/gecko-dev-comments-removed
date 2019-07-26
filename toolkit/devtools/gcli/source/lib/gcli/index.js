















'use strict';

var api = require('./api');
var Terminal = require('./ui/terminal').Terminal;
var settings = require('./settings');


require('./util/legacy');














var items = [
  require('./types/delegate').items,
  require('./types/selection').items,
  require('./types/array').items,

  require('./types/boolean').items,
  require('./types/command').items,
  require('./types/date').items,
  require('./types/file').items,
  require('./types/javascript').items,
  require('./types/node').items,
  require('./types/number').items,
  require('./types/resource').items,
  require('./types/setting').items,
  require('./types/string').items,

  require('./fields/delegate').items,
  require('./fields/selection').items,

  require('./ui/focus').items,
  require('./ui/intro').items,

  require('./converters/converters').items,
  require('./converters/basic').items,
  require('./converters/html').items,
  require('./converters/terminal').items,

  require('./languages/command').items,
  require('./languages/javascript').items,

  
  
  require('./connectors/websocket').items,
  require('./connectors/xhr').items,

  
].reduce(function(prev, curr) { return prev.concat(curr); }, []);

api.populateApi(exports);
exports.addItems(items);









exports.createTerminal = function(options) {
  options = options || {};
  if (options.settings != null) {
    settings.setDefaults(options.settings);
  }

  return Terminal.create(options).then(function(terminal) {
    options.terminal = terminal;
    terminal.language.showIntro();
    return terminal;
  });
};
