















'use strict';

var util = require('../util/util');
var Promise = require('../util/promise').Promise;

var RESOLVED = Promise.resolve(true);




var baseLanguage = {
  item: 'language',
  name: undefined,

  constructor: function(terminal) {
  },

  destroy: function() {
  },

  updateHints: function() {
    util.clearElement(this.terminal.tooltipElement);
  },

  description: '',
  message: '',
  caretMoved: function() {},

  handleUpArrow: function() {
    return Promise.resolve(false);
  },

  handleDownArrow: function() {
    return Promise.resolve(false);
  },

  handleTab: function() {
    this.terminal.unsetChoice();
    return RESOLVED;
  },

  handleInput: function(input) {
    if (input === ':') {
      return this.terminal.setInput('').then(function() {
        return this.terminal.pushLanguage('commands');
      }.bind(this));
    }

    this.terminal.unsetChoice();
    return RESOLVED;
  },

  handleReturn: function(input) {
    var rowoutEle = this.document.createElement('pre');
    rowoutEle.classList.add('gcli-row-out');
    rowoutEle.classList.add('gcli-row-script');
    rowoutEle.setAttribute('aria-live', 'assertive');

    return this.exec(input).then(function(line) {
      rowoutEle.innerHTML = line;

      this.terminal.addElement(rowoutEle);
      this.terminal.scrollToBottom();

      this.focusManager.outputted();

      this.terminal.unsetChoice();
      this.terminal.inputElement.value = '';
    }.bind(this));
  },

  setCursor: function(cursor) {
    this.terminal.inputElement.selectionStart = cursor.start;
    this.terminal.inputElement.selectionEnd = cursor.end;
  },

  getCompleterTemplateData: function() {
    return Promise.resolve({
      statusMarkup: [
        {
          string: this.terminal.inputElement.value.replace(/ /g, '\u00a0'), 
          className: 'gcli-in-valid'
        }
      ],
      unclosedJs: false,
      directTabText: '',
      arrowTabText: '',
      emptyParameters: ''
    });
  },

  showIntro: function() {
  },

  exec: function(input) {
    throw new Error('Missing implementation of handleReturn() or exec() ' + this.name);
  }
};




function Languages() {
  
  this._registered = {};
}




Languages.prototype.add = function(language) {
  this._registered[language.name] = language;
};




Languages.prototype.remove = function(language) {
  var name = typeof language === 'string' ? language : language.name;
  delete this._registered[name];
};




Languages.prototype.getAll = function() {
  return Object.keys(this._registered).map(function(name) {
    return this._registered[name];
  }.bind(this));
};




Languages.prototype.createLanguage = function(name, terminal) {
  if (name == null) {
    name = Object.keys(this._registered)[0];
  }

  var language = (typeof name === 'string') ? this._registered[name] : name;
  if (!language) {
    console.error('Known languages: ' + Object.keys(this._registered).join(', '));
    throw new Error('Unknown language: \'' + name + '\'');
  }

  
  var newInstance = {};
  util.copyProperties(baseLanguage, newInstance);
  util.copyProperties(language, newInstance);

  if (typeof newInstance.constructor === 'function') {
    var reply = newInstance.constructor(terminal);
    return Promise.resolve(reply).then(function() {
      return newInstance;
    });
  }
  else {
    return Promise.resolve(newInstance);
  }
};

exports.Languages = Languages;
