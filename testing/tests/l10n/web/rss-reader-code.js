





































var rssController = {
  __proto__: baseController,
  beforeSelect: function() {
    this.hashes = {};
    rssView.setUpHandlers();
    var _t = this;
    var callback = function(obj) {
      delete _t.req;
      if (view != rssView) {
        
        return;
      }
      _t.result = obj;
      rssView.updateView(keys(_t.result));
    };
    this.req = JSON.get('results/' + this.tag + '/feed-reader-results.json',
                        callback);
  },
  beforeUnSelect: function() {
    rssView.destroyHandlers();
  },
  showView: function(aClosure) {
    
  },
  getContent: function(aLoc) {
    var row = document.createDocumentFragment();
    var lst = this.result[aLoc];
    var _t = this;
    for each (var pair in lst) {
      
      var td = document.createElement('td');
      td.className = 'feedreader';
      td.innerHTML = pair[0];
      td.title = pair[1];
      row.appendChild(td);
    }
    return row;
  }
};
var rssView = {
  __proto__: baseView,
  setUpHandlers: function() {
    _t = this;
  },
  destroyHandlers: function() {
    if (this._dv) {
      this._dv.hide();
    }
  }
};
controller.addPane('RSS', 'rss', rssController, rssView);

