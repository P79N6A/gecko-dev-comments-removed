var EXPORTED_SYMBOLS = ["EventEmitter"];

function EventEmitter() {
}

EventEmitter.prototype = {
  







  on: function EventEmitter_on(aEvent, aListener) {
    if (!this._eventEmitterListeners)
      this._eventEmitterListeners = new Map();
    if (!this._eventEmitterListeners.has(aEvent)) {
      this._eventEmitterListeners.set(aEvent, []);
    }
    this._eventEmitterListeners.get(aEvent).push(aListener);
  },

  







  once: function EventEmitter_once(aEvent, aListener) {
    let handler = function() {
      this.off(aEvent, handler);
      aListener();
    }.bind(this);
    this.on(aEvent, handler);
  },

  








  off: function EventEmitter_off(aEvent, aListener) {
    if (!this._eventEmitterListeners)
      return;
    let listeners = this._eventEmitterListeners.get(aEvent);
    this._eventEmitterListeners.set(aEvent, listeners.filter(function(l) aListener != l));
  },

  



  emit: function EventEmitter_emit(aEvent) {
    if (!this._eventEmitterListeners || !this._eventEmitterListeners.has(aEvent))
      return;

    let originalListeners = this._eventEmitterListeners.get(aEvent);
    for (let listener of this._eventEmitterListeners.get(aEvent)) {
      
      
      if (!this._eventEmitterListeners) {
        break;
      }

      
      
      if (originalListeners === this._eventEmitterListeners.get(aEvent) ||
          this._eventEmitterListeners.get(aEvent).some(function(l) l === listener)) {
        listener.apply(null, arguments);
      }
    }
  },
}
