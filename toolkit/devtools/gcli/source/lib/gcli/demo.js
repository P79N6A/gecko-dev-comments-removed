















'use strict';














exports.items = [
  require('./cli').items,
  require('./commands/clear').items,
  require('./commands/connect').items,
  require('./commands/context').items,
  require('./commands/exec').items,
  require('./commands/global').items,
  require('./commands/help').items,
  require('./commands/intro').items,
  require('./commands/lang').items,
  require('./commands/mocks').items,
  require('./commands/pref').items,
  require('./commands/preflist').items,
  require('./commands/test').items,

  require('./commands/demo/alert').items,
  require('./commands/demo/bugs').items,
  require('./commands/demo/demo').items,
  require('./commands/demo/echo').items,
  require('./commands/demo/edit').items,
  
  
  require('./commands/demo/sleep').items,
  require('./commands/demo/theme').items,

  
].reduce(function(prev, curr) { return prev.concat(curr); }, []);
