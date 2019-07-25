






































(function() {




window.Profile = {
  
  
  silent: true,

  
  
  cutoff: 4,

  
  
  _time: Date.now(),

  
  
  
  wrap: function(obj, name) {
    let self = this;
    [i for (i in Iterator(obj))].forEach(function([key, val]) {
      if (typeof val != "function")
        return;

      obj[key] = function() {
        let start = Date.now();
        try {
          return val.apply(obj, arguments);
        } finally {
          let diff = Date.now() - start;
          if (diff >= self.cutoff && !self.silent)
            Utils.log("profile: " + name + "." + key + " = " + diff + "ms");
        }
      };
    });
  },

  
  
  
  checkpoint: function(label) {
    var now = Date.now();

    if (label && !this.silent)
      Utils.log("profile checkpoint: " + label + " = " + (now - this._time) + "ms");

    this._time = now;
  }
};

})();
