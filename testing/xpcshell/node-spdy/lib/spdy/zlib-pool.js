var zlibpool = exports,
    spdy = require('../spdy');





function Pool() {
  this.pool = {
    'spdy/2': [],
    'spdy/3': []
  };
}





zlibpool.create = function create() {
  return new Pool();
};

var x = 0;




Pool.prototype.get = function get(version, callback) {
  if (this.pool[version].length > 0) {
    return this.pool[version].pop();
  } else {
    var id = version.split('/', 2)[1];

    return {
      version: version,
      deflate: spdy.utils.createDeflate(id),
      inflate: spdy.utils.createInflate(id)
    };
  }
};





Pool.prototype.put = function put(pair) {
  var self = this,
      waiting = 2;

  spdy.utils.resetZlibStream(pair.inflate, done);
  spdy.utils.resetZlibStream(pair.deflate, done);

  function done() {
    if (--waiting === 0) {
      self.pool[pair.version].push(pair);
    }
  }
};
