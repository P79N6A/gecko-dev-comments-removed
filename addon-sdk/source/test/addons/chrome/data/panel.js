


'use strict';

self.port.on('echo', _ => {
  self.port.emit('echo', '');
});
