#ifdef 0



#endif
















function Batch(aCallback) {
  this._callback = aCallback;
}

Batch.prototype = {
  


  _count: 0,

  


  _closed: false,

  


  push: function Batch_push() {
    if (!this._closed)
      this._count++;
  },

  


  pop: function Batch_pop() {
    if (this._count)
      this._count--;

    if (this._closed)
      this._check();
  },

  


  close: function Batch_close() {
    if (this._closed)
      return;

    this._closed = true;
    this._check();
  },

  


  _check: function Batch_check() {
    if (this._count == 0 && this._callback) {
      this._callback();
      this._callback = null;
    }
  }
};
