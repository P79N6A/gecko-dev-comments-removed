var scheduler = exports;






function Scheduler(connection) {
  this.connection = connection;
  this.priorities = [[], [], [], []];
  this._tickListener = null;
}





exports.create = function create(connection) {
  return new Scheduler(connection);
};







Scheduler.prototype.schedule = function schedule(stream, data) {
  this.priorities[stream.priority].push(data);
};





Scheduler.prototype.tick = function tick() {
  if (this._tickListener !== null) return;
  var self = this;
  this._tickListener = function() {
    var priorities = self.priorities;

    self._tickListener = null;
    self.priorities = [[], [], [], []];

    
    for (var i = 0; i < 4; i++) {
      for (var j = 0; j < priorities[i].length; j++) {
        self.connection.write(
          priorities[i][j]
        );
      }
    }
  };

  if (this.connection.parser.drained) {
    process.nextTick(this._tickListener);
  } else {
    this.connection.parser.once('drain', this._tickListener);
  }
};
