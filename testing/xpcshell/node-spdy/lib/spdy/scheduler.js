var spdy = require('../spdy');
var utils = spdy.utils;
var scheduler = exports;






function Scheduler(connection) {
  this.connection = connection;
  this.priorities = [[], [], [], [], [], [], [], []];
  this._tickListener = this._tickListener.bind(this);
  this._tickListening = false;
  this._tickCallbacks = [];
  this._corked = false;
}





exports.create = function create(connection) {
  return new Scheduler(connection);
};







Scheduler.prototype.schedule = function schedule(stream, data) {
  
  if (stream._spdyState.destroyed)
    return;
  this.scheduleLast(stream, data);
};







Scheduler.prototype.scheduleLast = function scheduleLast(stream, data) {
  var priority = stream._spdyState.priority;
  priority = Math.min(priority, 7);
  priority = Math.max(priority, 0);
  this.priorities[priority].push(data);
};




Scheduler.prototype._tickListener = function tickListener() {
  var priorities = this.priorities;
  var tickCallbacks = this._tickCallbacks;

  this._tickListening = false;
  this.priorities = [[], [], [], [], [], [], [], []];
  this._tickCallbacks = [];

  
  for (var i = 0; i < 8; i++)
    for (var j = 0; j < priorities[i].length; j++)
      this.connection.write(priorities[i][j]);

  
  for (var i = 0; i < tickCallbacks.length; i++)
    tickCallbacks[i]();
  if (this._corked) {
    this.connection.uncork();
    if (this._tickListening)
      this.connection.cork();
    else
      this._corked = false;
  }
};





Scheduler.prototype.tick = function tick(cb) {
  if (cb)
    this._tickCallbacks.push(cb);
  if (this._tickListening)
    return;
  this._tickListening = true;

  if (!this._corked) {
    this.connection.cork();
    this._corked = true;
  }
  if (!this.connection._spdyState.parser.needDrain) {
    utils.nextTick(this._tickListener);
  } else {
    this.connection._spdyState.parser.once('drain', this._tickListener);
  }
};
