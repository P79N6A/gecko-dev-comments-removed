var zlibpool = exports,
    spdy = require('../spdy');





function Pool() {
  this.pool = [];
}





zlibpool.create = function create() {
  return new Pool();
};

var x = 0;




Pool.prototype.get = function get(callback) {
  if (this.pool.length > 0) {
    return this.pool.pop();
  } else {
    return {
      deflate: spdy.utils.createDeflate(),
      inflate: spdy.utils.createInflate()
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
      self.pool.push(pair);
    }
  }
};
