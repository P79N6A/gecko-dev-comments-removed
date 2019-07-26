



this.EXPORTED_SYMBOLS = ["EventEmitter"];




this.EventEmitter = function EventEmitter() {};








EventEmitter.decorate = function EventEmitter_decorate (aObjectToDecorate) {
  let emitter = new EventEmitter();
  aObjectToDecorate.on = emitter.on.bind(emitter);
  aObjectToDecorate.off = emitter.off.bind(emitter);
  aObjectToDecorate.once = emitter.once.bind(emitter);
  aObjectToDecorate.emit = emitter.emit.bind(emitter);
};

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
      aListener.apply(null, arguments);
    }.bind(this);
    this.on(aEvent, handler);
  },

  








  off: function EventEmitter_off(aEvent, aListener) {
    if (!this._eventEmitterListeners)
      return;
    let listeners = this._eventEmitterListeners.get(aEvent);
    if (listeners) {
      this._eventEmitterListeners.set(aEvent, listeners.filter(function(l) aListener != l));
    }
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
        try {
          listener.apply(null, arguments);
        }
        catch (ex) {
          
          let msg = ex + ": " + ex.stack;
          Components.utils.reportError(msg);
          dump(msg + "\n");
        }
      }
    }
  }
};
