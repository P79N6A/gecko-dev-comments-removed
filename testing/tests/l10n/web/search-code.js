





































var searchController = {
  __proto__: baseController,
  get path() {
    return 'results/' + this.tag + '/search-results.json';
  },
  beforeSelect: function() {
    this.hashes = {};
    searchView.setUpHandlers();
    var _t = this;
    var callback = function(obj) {
      delete _t.req;
      if (view != searchView) {
        
        return;
      }
      _t.result = obj;
      searchView.updateView(keys(_t.result.locales));
    };
    this.req = JSON.get('results/' + this.tag + '/search-results.json',
                        callback);
  },
  beforeUnSelect: function() {
    this.hashes = {};
    searchView.destroyHandlers();
  },
  showView: function(aClosure) {
    
  },
  getContent: function(aLoc) {
    var row = document.createDocumentFragment();
    var lst = this.result.locales[aLoc].list;
    var _t = this;
    lst.sort(function(a,b){return _t.cmp.apply(_t,[a,b])});
    var orders = this.result.locales[aLoc].orders;
    if (orders) {
      var explicit = [];
      var implicit = [];
      for (var sn in orders) {
        if (explicit[sn]) {
          fatal = true;
          break;
        }
        explicit[sn] = orders[sn];
      }
      explicit = [];
      for each (var path in lst) {
        var shortName = this.result.details[path].ShortName;
        if (orders[shortName]) {
          explicit[orders[shortName] - 1] = path;
        }
        else {
          implicit.push(path);
        }
      }
      lst = explicit.concat(implicit);
    }
    row.innerHTML = '<td class="locale"><a href="https://bugzilla.mozilla.org/buglist.cgi?query_format=advanced&amp;short_desc_type=regexp&amp;short_desc=' + aLoc + '%5B%20-%5D&amp;chfieldto=Now&amp;field0-0-0=blocked&amp;type0-0-0=substring&amp;value0-0-0=347914">bug</a></td>';
    for each (var path in lst) {
      
      var innerContent;
      var cl = '';
      if (path.match(/^mozilla/)) {
        cl += ' enUS';
      }
      var localName = path.substr(path.lastIndexOf('/') + 1);
      if (this.result.details[path].error) {
        innerContent = 'error in ' + localName;
        cl += ' error';
      }
      else {
        var shortName = this.result.details[path].ShortName;
        var img = this.result.details[path].Image;
        if (this.result.locales[aLoc].orders && this.result.locales[aLoc].orders[shortName]) {
          cl += " ordered";
        }
        innerContent = '<img src="' + img + '">' + shortName;
      }
      var td = document.createElement('td');
      td.className = 'searchplugin' + cl;
      td.innerHTML = innerContent;
      row.appendChild(td);
      td.details = this.result.details[path];
      
      if (td.details.error) {
        
        continue;
      }
      if (this.hashes[localName]) {
        this.hashes[localName].nodes.push(td);
        if (this.hashes[localName].conflict) {
          td.className += ' conflict';
        }
        else if (this.hashes[localName].key != td.details.md5) {
          this.hashes[localName].conflict = true;
          for each (td in this.hashes[localName].nodes) {
            td.className += ' conflict';
          }
        }
      }
      else {
        this.hashes[localName] =  {key: td.details.md5, nodes: [td]};
      }
    }
    for (localName in this.hashes) {
      if ( ! ('conflict' in this.hashes[localName])) {
        continue;
      }
      locs = [];
      for each (var td in this.hashes[localName].nodes) {
        locs.push(td.parentNode.firstChild.textContent);
      }
      YAHOO.widget.Logger.log('difference in ' + localName + ' for ' + locs.join(', '));
    }
    return row;
  },
  cmp: function(l,r) {
    var lName = this.result.details[l].ShortName;
    var rName =  this.result.details[r].ShortName;
    if (lName) lName = lName.toLowerCase();
    if (rName) rName = rName.toLowerCase();
    return lName < rName ? -1 : lName > rName ? 1 : 0;
  }
};
var searchView = {
  __proto__: baseView,
  setUpHandlers: function() {
    _t = this;
    this.omv = function(event){_t.onMouseOver.apply(_t, [event]);};
    this.omo = function(event){_t.onMouseOut.apply(_t, [event]);};
    this.content.addEventListener("mouseover", this.omv, true);
    this.content.addEventListener("mouseout", this.omv, true);
  },
  destroyHandlers: function() {
    this.content.removeEventListener("mouseover", this.omv, true);
    this.content.removeEventListener("mouseout", this.omv, true);
    if (this._dv) {
      this._dv.hide();
    }
  },
  onMouseOver: function(event) {
    if (!event.target.details)
      return;
    var _e = {
      details: event.target.details,
      clientX: event.clientX,
      clientY: event.clientY
    };
    this.pending = setTimeout(function(){searchView.showDetail(_e);}, 500);
  },
  onMouseOut: function(event) {
    if (this.pending) {
      clearTimeout(this.pending);
      delete this.pending;
      return;
    }
    if (!event.target.details)
      return;
  },
  showDetail: function(event) {
    delete this.pending;
    if (!this._dv) {
      this._dv = new YAHOO.widget.Panel('dv', {visible:true,draggable:true,constraintoviewport:true});
      this._dv.beforeHideEvent.subscribe(function(){delete this._dv;}, null);
    }
    var dt = event.details;
    if (dt.error) {
      this._dv.setHeader("Error");
      this._dv.setBody(dt.error);
    }
    else {
      this._dv.setHeader("Details");
      var c = '';
      var q = 'test';
      var len = 0;
      for each (var url in dt.urls) {
        var uc = url.template;
        var sep = (uc.indexOf('?') > -1) ? '&amp;' : '?';
        for (var p in url.params) {
          uc += sep + p + '=' + url.params[p];
          sep = '&amp;';
        }
        uc = uc.replace('{searchTerms}', q);
        var mpContent = '';
        if (url.MozParams) {
          for each (var mp in url.MozParams) {
            
            if (mp.condition == 'pref') {
              mpContent += ', using pref ' + mp.name + '=' + mp.pref;
            }
          }
        }
        c += '<button onclick="window.open(this.textContent)">' + uc + '</button>' +mpContent+ '<br>';
        len  = len < c.length ? c.length : len;
      }
      this._dv.setBody(c);
    }
    this._dv.render(document.body);
    this._dv.moveTo(event.clientX + window.scrollX, event.clientY + window.scrollY);
    this._dv.show();
  }
};
controller.addPane('Search', 'search', searchController, searchView);

