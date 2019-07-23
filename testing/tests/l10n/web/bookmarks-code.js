





































var bookmarksView = {
  __proto__: baseView,
  setUpHandlers: function() {
    _t = this;
  },
  destroyHandlers: function() {
  },
  
  createFullTree: function ct(aDetails, aNode, aName) {
    if (!aDetails || ! (aDetails instanceof Object))
      return;
    var cat = new YAHOO.widget.TextNode(aName, aNode, true);
    for each (postfix in ["Files", ""]) {
      var subRes = aDetails[aName + postfix];
      var append = postfix ? ' (files)' : '';
      for (mod in subRes) {
        var mn = new YAHOO.widget.TextNode(mod + append, cat, true);
        for (fl in subRes[mod]) {
          if (postfix) {
            new YAHOO.widget.TextNode(subRes[mod][fl], mn, false);
            continue;
          }
          var fn = new YAHOO.widget.TextNode(fl, mn, false);
          var keys = [];
          for each (k in subRes[mod][fl]) {
            keys.push(k);
          }
          new YAHOO.widget.HTMLNode("<pre>" + keys.join("\n") + "</pre>", fn, true, false);
        }
      }
    }
    return cat;
  }
};
var bookmarksController = {
  __proto__: baseController,
  get path() {
    return 'results/' + this.tag + '/bookmarks-results.json';
  },
  beforeSelect: function() {
    this.result = {};
    this.isShown = false;
    bookmarksView.setUpHandlers();
    var _t = this;
    var callback = function(obj) {
      delete _t.req;
      if (view != bookmarksView) {
        
        return;
      }
      _t.result = obj;
      bookmarksView.updateView(keys(_t.result));
    };
    this.req = JSON.get('results/' + this.tag + '/bookmarks-results.json', callback);
  },
  beforeUnSelect: function() {
    if (this.req) {
      this.req.abort();
      delete this.req;
    }
    bookmarksView.destroyHandlers();
  },
  showView: function(aClosure) {
    
  },
  getContent: function(aLoc) {
    var row = view.getCell();
    var inner = document.createElement('div');
    row.appendChild(inner);
    var r = this.result[aLoc];
    var id = "bookmarks-" + aLoc;
    inner.id = id;
    var hasDetails = true;
    t = new YAHOO.widget.TreeView(inner);
    var d = this.result[aLoc];
    var innerContent = d['__title__'];
    function createChildren(aData, aParent, isOpen) {
      if ('children' in aData) {
        
        innerContent = aData['__title__'];
        if ('__desc__' in aData && aData['__desc__']) {
          innerContent += '<br><em>' + aData['__desc__'] + '</em>';
        }
        var nd = new YAHOO.widget.HTMLNode(innerContent, aParent, isOpen, true);
        for each (child in aData['children']) {
          createChildren(child, nd, true);
        }
      }
      else {
        
        innerContent = '';
        if ('ICON' in aData) {
          innerContent = '<img src="' + aData['ICON'] + '">';
        }
        innerContent += '<a href="' + aData['HREF'] + '">' +
          aData['__title__'] + '</a>';
        if ('FEEDURL' in aData) {
          innerContent += '<a href="' + aData['FEEDURL'] + '">&nbsp;<em>(FEED)</em></a>';
        }
        new YAHOO.widget.HTMLNode(innerContent, aParent, false, false);
      }
    }
    createChildren(d, t.getRoot(), false);
    t.draw();
    return row;
  }
};
controller.addPane('Bookmarks', 'bookmarks', bookmarksController, 
                   bookmarksView);
