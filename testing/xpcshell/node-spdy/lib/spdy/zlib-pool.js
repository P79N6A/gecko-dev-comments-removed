var zlibpool = exports,
    spdy = require('../spdy');






function Pool(compression) {
  this.compression = compression;
  this.pool = {
    'spdy/2': [],
    'spdy/3': [],
    'spdy/3.1': []
  };
}






zlibpool.create = function create(compression) {
  return new Pool(compression);
};

var x = 0;




Pool.prototype.get = function get(version, callback) {
  if (this.pool[version].length > 0) {
    return this.pool[version].pop();
  } else {
    var id = version.split('/', 2)[1];

    return {
      version: version,
      deflate: spdy.utils.createDeflate(id, this.compression),
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
    if (--waiting === 0)
      self.pool[pair.version].push(pair);
  }
};
