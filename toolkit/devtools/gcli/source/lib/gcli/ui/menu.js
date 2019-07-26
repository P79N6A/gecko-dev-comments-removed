















'use strict';

var util = require('../util/util');
var l10n = require('../util/l10n');
var domtemplate = require('../util/domtemplate');
var host = require('../util/host');




var menuCssPromise;
var menuHtmlPromise;










function Menu(options) {
  options = options || {};
  this.document = options.document || document;
  this.maxPredictions = options.maxPredictions || 8;

  
  this._choice = null;

  
  if (!this.document) {
    throw new Error('No document');
  }

  this.element =  util.createElement(this.document, 'div');
  this.element.classList.add('gcli-menu');

  if (menuCssPromise == null) {
    menuCssPromise = host.staticRequire(module, './menu.css');
  }
  menuCssPromise.then(function(menuCss) {
    
    if (menuCss != null) {
      util.importCss(menuCss, this.document, 'gcli-menu');
    }
  }.bind(this));

  this.templateOptions = { blankNullUndefined: true, stack: 'menu.html' };
  if (menuHtmlPromise == null) {
    menuHtmlPromise = host.staticRequire(module, './menu.html');
  }
  menuHtmlPromise.then(function(menuHtml) {
    this.template = util.toDom(this.document, menuHtml);
  }.bind(this));

  
  this.items = [];

  this.onItemClick = util.createEvent('Menu.onItemClick');
}




Menu.prototype.l10n = l10n.propertyLookup;




Menu.prototype.destroy = function() {
  this.element = undefined;
  this.template = undefined;
  this.document = undefined;
  this.items = undefined;
};






Menu.prototype.onItemClickInternal = function(ev) {
  var name = ev.currentTarget.querySelector('.gcli-menu-name').textContent;
  this.onItemClick({ name: name });
};




Object.defineProperty(Menu.prototype, 'selected', {
  get: function() {
    var item = this.element.querySelector('.gcli-menu-highlight .gcli-menu-name');
    if (!item) {
      return null;
    }
    return item.textContent;
  },
  enumerable: true
});







Menu.prototype.show = function(items, match) {
  
  if (this.template == null) {
    return;
  }

  this.items = items.filter(function(item) {
    return item.hidden === undefined || item.hidden !== true;
  }.bind(this));

  if (match) {
    this.items = this.items.map(function(item) {
      return getHighlightingProxy(item, match, this.template.ownerDocument);
    }.bind(this));
  }

  if (this.items.length === 0) {
    this.element.style.display = 'none';
    return;
  }

  if (this.items.length >= this.maxPredictions) {
    this.items.splice(-1);
    this.hasMore = true;
  }
  else {
    this.hasMore = false;
  }

  var options = this.template.cloneNode(true);
  domtemplate.template(options, this, this.templateOptions);

  util.clearElement(this.element);
  this.element.appendChild(options);

  this.element.style.display = 'block';
};




function getHighlightingProxy(item, match, document) {
  if (typeof Proxy === 'undefined') {
    return item;
  }
  return Proxy.create({
    get: function(rcvr, name) {
      var value = item[name];
      if (name !== 'name') {
        return value;
      }

      var startMatch = value.indexOf(match);
      if (startMatch === -1) {
        return value;
      }

      var before = value.substr(0, startMatch);
      var after = value.substr(startMatch + match.length);
      var parent = util.createElement(document, 'span');
      parent.appendChild(document.createTextNode(before));
      var highlight = util.createElement(document, 'span');
      highlight.classList.add('gcli-menu-typed');
      highlight.appendChild(document.createTextNode(match));
      parent.appendChild(highlight);
      parent.appendChild(document.createTextNode(after));
      return parent;
    }
  });
}




Menu.prototype.getChoiceIndex = function() {
  return this._choice == null ? 0 : this._choice;
};




Menu.prototype.incrementChoice = function() {
  if (this._choice == null) {
    this._choice = 0;
  }

  
  
  
  this._choice--;
  this._updateHighlight();
};




Menu.prototype.decrementChoice = function() {
  if (this._choice == null) {
    this._choice = 0;
  }

  
  this._choice++;
  this._updateHighlight();
};




Menu.prototype.unsetChoice = function() {
  this._choice = null;
  this._updateHighlight();
};




Menu.prototype._updateHighlight = function() {
  var nodes = this.element.querySelectorAll('.gcli-menu-option');
  for (var i = 0; i < nodes.length; i++) {
    nodes[i].classList.remove('gcli-menu-highlight');
  }

  if (this._choice == null || nodes.length === 0) {
    return;
  }

  var index = this._choice % nodes.length;
  if (index < 0) {
    index = nodes.length + index;
  }

  nodes.item(index).classList.add('gcli-menu-highlight');
};




Menu.prototype.hide = function() {
  this.element.style.display = 'none';
};




Menu.prototype.setMaxHeight = function(height) {
  this.element.style.maxHeight = height + 'px';
};

exports.Menu = Menu;
