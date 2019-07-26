(function(mod) {
  if (typeof exports == "object" && typeof module == "object") 
    return mod(exports);
  if (typeof define == "function" && define.amd) 
    return define(["exports"], mod);
  mod((self.tern || (self.tern = {})).signal = {}); 
})(function(exports) {
  function on(type, f) {
    var handlers = this._handlers || (this._handlers = Object.create(null));
    (handlers[type] || (handlers[type] = [])).push(f);
  }
  function off(type, f) {
    var arr = this._handlers && this._handlers[type];
    if (arr) for (var i = 0; i < arr.length; ++i)
      if (arr[i] == f) { arr.splice(i, 1); break; }
  }
  function signal(type, a1, a2, a3, a4) {
    var arr = this._handlers && this._handlers[type];
    if (arr) for (var i = 0; i < arr.length; ++i) arr[i].call(this, a1, a2, a3, a4);
  }

  exports.mixin = function(obj) {
    obj.on = on; obj.off = off; obj.signal = signal;
    return obj;
  };
});
