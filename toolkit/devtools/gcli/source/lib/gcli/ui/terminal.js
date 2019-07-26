















'use strict';

var promise = require('../util/promise');
var util = require('../util/util');
var domtemplate = require('../util/domtemplate');
var KeyEvent = require('../util/util').KeyEvent;
var host = require('../util/host');

var languages = require('../languages/languages');
var History = require('./history').History;

var FocusManager = require('./focus').FocusManager;

var RESOLVED = promise.resolve(undefined);




var resourcesPromise;





function Terminal() {
  throw new Error('Use Terminal.create().then(...) rather than new Terminal()');
}







Terminal.create = function(options) {
  if (resourcesPromise == null) {
    resourcesPromise = promise.all([
      host.staticRequire(module, './terminal.css'),
      host.staticRequire(module, './terminal.html')
    ]);
  }
  return resourcesPromise.then(function(resources) {
    var terminal = Object.create(Terminal.prototype);
    return terminal._init(options, resources[0], resources[1]);
  });
};





Terminal.prototype._init = function(options, terminalCss, terminalHtml) {
  this.document = options.document || document;
  this.options = options;

  this.focusManager = new FocusManager(this.document);
  this.onInputChange = util.createEvent('Terminal.onInputChange');

  
  this.rootElement = this.document.getElementById('gcli-root');
  if (!this.rootElement) {
    throw new Error('Missing element, id=gcli-root');
  }
  this.rootElement.terminal = this;

  
  var template = util.toDom(this.document, terminalHtml);

  
  
  
  var tooltipParent = template.querySelector('.gcli-tooltip');
  this.tooltipTemplate = tooltipParent.children[0];
  tooltipParent.removeChild(this.tooltipTemplate);

  
  
  var completerParent = template.querySelector('.gcli-in-complete');
  this.completerTemplate = completerParent.children[0];
  completerParent.removeChild(this.completerTemplate);
  
  util.removeWhitespace(this.completerTemplate, true);

  
  
  
  
  domtemplate.template(template, this, { stack: 'terminal.html' });
  while (template.hasChildNodes()) {
    this.rootElement.appendChild(template.firstChild);
  }

  if (terminalCss != null) {
    this.style = util.importCss(terminalCss, this.document, 'gcli-tooltip');
  }

  this.tooltipElement.classList.add('gcli-panel-hide');

  
  this.inputElement.focus();

  
  this.lastTabDownAt = 0;

  
  this.history = new History();
  this._scrollingThroughHistory = false;

  
  this._completed = RESOLVED;

  
  this._previousValue = undefined;

  
  this.fields = [];

  
  this.focus = this.focus.bind(this);
  this.onKeyDown = this.onKeyDown.bind(this);
  this.onKeyUp = this.onKeyUp.bind(this);
  this.onMouseUp = this.onMouseUp.bind(this);
  this.onOutput = this.onOutput.bind(this);

  this.rootElement.addEventListener('click', this.focus, false);

  
  this.inputElement.addEventListener('keydown', this.onKeyDown, false);
  this.inputElement.addEventListener('keyup', this.onKeyUp, false);

  
  this.inputElement.addEventListener('mouseup', this.onMouseUp, false);

  this.focusManager.onVisibilityChange.add(this.visibilityChanged, this);
  this.focusManager.addMonitoredElement(this.tooltipElement, 'tooltip');
  this.focusManager.addMonitoredElement(this.inputElement, 'input');

  this.onInputChange.add(this.updateCompletion, this);

  host.script.onOutput.add(this.onOutput);

  
  return this.switchLanguage(null).then(function() {
    return this;
  }.bind(this));
};




Terminal.prototype.destroy = function() {
  this.focusManager.removeMonitoredElement(this.inputElement, 'input');
  this.focusManager.removeMonitoredElement(this.tooltipElement, 'tooltip');
  this.focusManager.onVisibilityChange.remove(this.visibilityChanged, this);

  this.inputElement.removeEventListener('mouseup', this.onMouseUp, false);
  this.inputElement.removeEventListener('keydown', this.onKeyDown, false);
  this.inputElement.removeEventListener('keyup', this.onKeyUp, false);
  this.rootElement.removeEventListener('click', this.focus, false);

  this.language.destroy();
  this.history.destroy();
  this.focusManager.destroy();

  if (this.style) {
    this.style.parentNode.removeChild(this.style);
    this.style = undefined;
  }

  this.field.onFieldChange.remove(this.fieldChanged, this);
  this.field.destroy();

  this.onInputChange.remove(this.updateCompletion, this);

  
  util.clearElement(this.displayElement);

  this.focus = undefined;
  this.onMouseUp = undefined;
  this.onKeyDown = undefined;
  this.onKeyUp = undefined;

  this.rootElement = undefined;
  this.inputElement = undefined;
  this.promptElement = undefined;
  this.completeElement = undefined;
  this.tooltipElement = undefined;
  this.panelElement = undefined;
  this.displayElement = undefined;

  this.completerTemplate = undefined;
  this.tooltipTemplate = undefined;

  this.errorEle = undefined;
  this.descriptionEle = undefined;

  this.document = undefined;
};




Terminal.prototype.switchLanguage = function(name) {
  if (this.language != null) {
    this.language.destroy();
  }

  return languages.createLanguage(name, this).then(function(language) {
    this._updateLanguage(language);
  }.bind(this));
};




Terminal.prototype.pushLanguage = function(name) {
  return languages.createLanguage(name, this).then(function(language) {
    this.origLanguage = this.language;
    this._updateLanguage(language);
  }.bind(this));
};




Terminal.prototype.popLanguage = function() {
  if (this.origLanguage == null) {
    return RESOLVED;
  }

  this._updateLanguage(this.origLanguage);
  this.origLanguage = undefined;

  return RESOLVED;
};




Terminal.prototype._updateLanguage = function(language) {
  this.language = language;

  if (this.language.proportionalFonts) {
    this.topElement.classList.remove('gcli-in-script');
  }
  else {
    this.topElement.classList.add('gcli-in-script');
  }

  this.language.updateHints();
  this.updateCompletion();
  this.promptElement.innerHTML = this.language.prompt;
};




Terminal.prototype.onOutput = function(ev) {
  console.log('onOutput', ev);

  var rowoutEle = this.document.createElement('pre');
  rowoutEle.classList.add('gcli-row-out');
  rowoutEle.classList.add('gcli-row-script');
  rowoutEle.setAttribute('aria-live', 'assertive');

  var output = '         // ';
  if (ev.level === 'warn') {
    output += '!';
  }
  else if (ev.level === 'error') {
    output += '✖';
  }
  else {
    output += '→';
  }

  output += ' ' + ev.arguments.map(function(arg) {
    return arg;
  }).join(',');

  rowoutEle.innerHTML = output;

  this.addElement(rowoutEle);
};




Terminal.prototype.onMouseUp = function(ev) {
  this.language.caretMoved(this.inputElement.selectionStart);
};








Terminal.prototype.setInput = function(str) {
  this._scrollingThroughHistory = false;
  return this._setInput(str);
};




Terminal.prototype._setInput = function(str) {
  this.inputElement.value = str;
  this._previousValue = this.inputElement.value;

  this._completed = this.language.handleInput(str);
  return this._completed;
};




Terminal.prototype.focus = function() {
  this.inputElement.focus();
  this.language.caretMoved(this.inputElement.selectionStart);
};





Terminal.prototype.onKeyDown = function(ev) {
  if (ev.keyCode === KeyEvent.DOM_VK_UP ||
      ev.keyCode === KeyEvent.DOM_VK_DOWN) {
    ev.preventDefault();
    return;
  }

  
  
  if (ev.keyCode === KeyEvent.DOM_VK_F1 ||
      ev.keyCode === KeyEvent.DOM_VK_ESCAPE ||
      ev.keyCode === KeyEvent.DOM_VK_UP ||
      ev.keyCode === KeyEvent.DOM_VK_DOWN) {
    return;
  }

  if (this.focusManager) {
    this.focusManager.onInputChange();
  }

  if (ev.keyCode === KeyEvent.DOM_VK_BACK_SPACE &&
      this.inputElement.value === '') {
    return this.popLanguage();
  }

  if (ev.keyCode === KeyEvent.DOM_VK_TAB) {
    this.lastTabDownAt = 0;
    if (!ev.shiftKey) {
      ev.preventDefault();
      
      
      this.lastTabDownAt = ev.timeStamp;
    }
    if (ev.metaKey || ev.altKey || ev.crtlKey) {
      if (this.document.commandDispatcher) {
        this.document.commandDispatcher.advanceFocus();
      }
      else {
        this.inputElement.blur();
      }
    }
  }
};






Terminal.prototype.onKeyUp = function(ev) {
  this.handleKeyUp(ev).then(null, util.errorHandler);
};






Terminal.prototype.handleKeyUp = function(ev) {
  if (this.focusManager && ev.keyCode === KeyEvent.DOM_VK_F1) {
    this.focusManager.helpRequest();
    return RESOLVED;
  }

  if (this.focusManager && ev.keyCode === KeyEvent.DOM_VK_ESCAPE) {
    if (this.focusManager.isTooltipVisible ||
        this.focusManager.isOutputVisible) {
      this.focusManager.removeHelp();
    }
    else if (this.inputElement.value === '') {
      return this.popLanguage();
    }
    return RESOLVED;
  }

  if (ev.keyCode === KeyEvent.DOM_VK_UP) {
    if (this.isMenuShowing) {
      return this.incrementChoice();
    }

    if (this.inputElement.value === '' || this._scrollingThroughHistory) {
      this._scrollingThroughHistory = true;
      return this._setInput(this.history.backward());
    }

    return this.language.handleUpArrow().then(function(handled) {
      if (!handled) {
        return this.incrementChoice();
      }
    }.bind(this));
  }

  if (ev.keyCode === KeyEvent.DOM_VK_DOWN) {
    if (this.isMenuShowing) {
      return this.decrementChoice();
    }

    if (this.inputElement.value === '' || this._scrollingThroughHistory) {
      this._scrollingThroughHistory = true;
      return this._setInput(this.history.forward());
    }

    return this.language.handleDownArrow().then(function(handled) {
      if (!handled) {
        return this.decrementChoice();
      }
    }.bind(this));
  }

  if (ev.keyCode === KeyEvent.DOM_VK_RETURN) {
    var input = this.inputElement.value;
    return this.language.handleReturn(input).then(function(handled) {
      if (!handled) {
        this._scrollingThroughHistory = false;

        if (!this.selectChoice()) {
          this.focusManager.setError(true);
        }
        else {
          return this.popLanguage();
        }
      }
      else {
        return this.popLanguage();
      }
    }.bind(this));
  }

  if (ev.keyCode === KeyEvent.DOM_VK_TAB && !ev.shiftKey) {
    
    
    var hasContents = (this.inputElement.value.length > 0);

    
    
    
    
    
    
    if (hasContents && this.lastTabDownAt + 1000 > ev.timeStamp) {
      this._completed = this.language.handleTab();
    }
    else {
      this._completed = RESOLVED;
    }

    this.lastTabDownAt = 0;
    this._scrollingThroughHistory = false;

    return this._completed;
  }

  if (this._previousValue === this.inputElement.value) {
    return RESOLVED;
  }

  var value = this.inputElement.value;
  this._scrollingThroughHistory = false;
  this._previousValue = this.inputElement.value;

  this._completed = this.language.handleInput(value);
  return this._completed;
};




Terminal.prototype.getChoiceIndex = function() {
  return this.field && this.field.menu ? this.field.menu.getChoiceIndex() : 0;
};




Terminal.prototype.unsetChoice = function() {
  if (this.field && this.field.menu) {
    this.field.menu.unsetChoice();
  }
  return this.updateCompletion();
};




Terminal.prototype.incrementChoice = function() {
  if (this.field && this.field.menu) {
    this.field.menu.incrementChoice();
  }
  return this.updateCompletion();
};




Terminal.prototype.decrementChoice = function() {
  if (this.field && this.field.menu) {
    this.field.menu.decrementChoice();
  }
  return this.updateCompletion();
};




Terminal.prototype.getInputState = function() {
  var input = {
    typed: this.inputElement.value,
    cursor: {
      start: this.inputElement.selectionStart,
      end: this.inputElement.selectionEnd
    }
  };

  
  
  if (input.typed == null) {
    input = { typed: '', cursor: { start: 0, end: 0 } };
  }

  return input;
};




Terminal.prototype.updateCompletion = function() {
  return this.language.getCompleterTemplateData().then(function(data) {
    var template = this.completerTemplate.cloneNode(true);
    domtemplate.template(template, data, { stack: 'terminal.html#completer' });

    util.clearElement(this.completeElement);
    while (template.hasChildNodes()) {
      this.completeElement.appendChild(template.firstChild);
    }
  }.bind(this));
};




Object.defineProperty(Terminal.prototype, 'isMenuShowing', {
  get: function() {
    return this.focusManager.isTooltipVisible &&
           this.field != null &&
           this.field.menu != null;
  },
  enumerable: true
});






Terminal.prototype.selectChoice = function(ev) {
  if (this.field && this.field.selectChoice) {
    return this.field.selectChoice();
  }
  return false;
};




Terminal.prototype.fieldChanged = function(ev) {
  this.language.fieldChanged(ev);

  
  
  
  this.document.defaultView.setTimeout(function() {
    this.focus();
  }.bind(this), 10);
};




Terminal.prototype.visibilityChanged = function(ev) {
  if (!this.panelElement) {
    return;
  }

  if (ev.tooltipVisible) {
    this.tooltipElement.classList.remove('gcli-panel-hide');
  }
  else {
    this.tooltipElement.classList.add('gcli-panel-hide');
  }
  this.scrollToBottom();
};




Terminal.prototype.addElement = function(element) {
  this.displayElement.insertBefore(element, this.topElement);
};




Terminal.prototype.clear = function() {
  while (this.displayElement.hasChildNodes()) {
    if (this.displayElement.firstChild === this.topElement) {
      break;
    }
    this.displayElement.removeChild(this.displayElement.firstChild);
  }
};




Terminal.prototype.scrollToBottom = function() {
  
  
  
  var scrollHeight = Math.max(this.displayElement.scrollHeight,
                              this.displayElement.clientHeight);
  this.displayElement.scrollTop =
                      scrollHeight - this.displayElement.clientHeight;
};

exports.Terminal = Terminal;
