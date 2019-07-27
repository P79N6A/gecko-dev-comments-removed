















'use strict';

var util = require('../util/util');
var Promise = require('../util/promise').Promise;
var domtemplate = require('../util/domtemplate');
var host = require('../util/host');

var Status = require('../types/types').Status;
var cli = require('../cli');
var Requisition = require('../cli').Requisition;
var CommandAssignment = require('../cli').CommandAssignment;
var intro = require('../ui/intro');

var RESOLVED = Promise.resolve(true);





var Caret = exports.Caret = {
  


  NO_CHANGE: 0,

  


  SELECT_ALL: 1,

  


  TO_END: 2,

  



  TO_ARG_END: 3
};




var commandHtmlPromise;

var commandLanguage = exports.commandLanguage = {
  
  item: 'language',
  name: 'commands',
  prompt: ':',
  proportionalFonts: true,

  constructor: function(terminal) {
    this.terminal = terminal;
    this.document = terminal.document;
    this.focusManager = terminal.focusManager;

    var options = this.terminal.options;
    this.requisition = options.requisition;
    if (this.requisition == null) {
      if (options.environment == null) {
        options.environment = {};
        options.environment.document = options.document || this.document;
        options.environment.window = options.environment.document.defaultView;
      }

      this.requisition = new Requisition(terminal.system, options);
    }

    
    this.lastText = undefined;

    
    this._caretChange = null;

    
    this.assignment = this.requisition.getAssignmentAt(0);

    if (commandHtmlPromise == null) {
      commandHtmlPromise = host.staticRequire(module, './command.html');
    }

    return commandHtmlPromise.then(function(commandHtml) {
      this.commandDom = host.toDom(this.document, commandHtml);

      this.requisition.commandOutputManager.onOutput.add(this.outputted, this);
      var mapping = cli.getMapping(this.requisition.executionContext);
      mapping.terminal = this.terminal;

      this.requisition.onExternalUpdate.add(this.textChanged, this);

      return this;
    }.bind(this));
  },

  destroy: function() {
    var mapping = cli.getMapping(this.requisition.executionContext);
    delete mapping.terminal;

    this.requisition.commandOutputManager.onOutput.remove(this.outputted, this);
    this.requisition.onExternalUpdate.remove(this.textChanged, this);

    this.terminal = undefined;
    this.requisition = undefined;
    this.commandDom = undefined;
  },

  
  textChanged: function() {
    if (this.terminal == null) {
      return; 
    }

    if (this.terminal._caretChange == null) {
      
      
      
      this.terminal._caretChange = Caret.TO_ARG_END;
    }

    var newStr = this.requisition.toString();
    var input = this.terminal.getInputState();

    input.typed = newStr;
    this._processCaretChange(input);

    
    
    if (this.terminal.inputElement.value !== newStr) {
      this.terminal.inputElement.value = newStr;
    }
    this.terminal.onInputChange({ inputState: input });

    
    
    if (this.assignment.arg.text === this.lastText) {
      return;
    }

    this.lastText = this.assignment.arg.text;

    this.terminal.field.update();
    this.terminal.field.setConversion(this.assignment.conversion);
    util.setTextContent(this.terminal.descriptionEle, this.description);
  },

  
  
  caretMoved: function(start) {
    if (!this.requisition.isUpToDate()) {
      return;
    }
    var newAssignment = this.requisition.getAssignmentAt(start);
    if (newAssignment == null) {
      return;
    }

    if (this.assignment !== newAssignment) {
      if (this.assignment.param.type.onLeave) {
        this.assignment.param.type.onLeave(this.assignment);
      }

      
      
      
      var isNew = (this.assignment !== newAssignment);

      this.assignment = newAssignment;
      this.terminal.updateCompletion();

      if (isNew) {
        this.updateHints();
      }

      if (this.assignment.param.type.onEnter) {
        this.assignment.param.type.onEnter(this.assignment);
      }
    }
    else {
      if (this.assignment && this.assignment.param.type.onChange) {
        this.assignment.param.type.onChange(this.assignment);
      }
    }

    
    
    var error = (this.assignment.status === Status.ERROR);
    this.focusManager.setError(error);
  },

  
  updateHints: function() {
    this.lastText = this.assignment.arg.text;

    var field = this.terminal.field;
    if (field) {
      field.onFieldChange.remove(this.terminal.fieldChanged, this.terminal);
      field.destroy();
    }

    var fields = this.terminal.system.fields;
    field = this.terminal.field = fields.get(this.assignment.param.type, {
      document: this.terminal.document,
      requisition: this.requisition
    });

    this.focusManager.setImportantFieldFlag(field.isImportant);

    field.onFieldChange.add(this.terminal.fieldChanged, this.terminal);
    field.setConversion(this.assignment.conversion);

    
    this.terminal.errorEle = undefined;
    this.terminal.descriptionEle = undefined;

    var contents = this.terminal.tooltipTemplate.cloneNode(true);
    domtemplate.template(contents, this.terminal, {
      blankNullUndefined: true,
      stack: 'terminal.html#tooltip'
    });

    util.clearElement(this.terminal.tooltipElement);
    this.terminal.tooltipElement.appendChild(contents);
    this.terminal.tooltipElement.style.display = 'block';

    field.setMessageElement(this.terminal.errorEle);
  },

  


  handleUpArrow: function() {
    
    
    if (this.assignment.getStatus() === Status.VALID) {
      return this.requisition.increment(this.assignment).then(function() {
        this.textChanged();
        this.focusManager.onInputChange();
        return true;
      }.bind(this));
    }

    return Promise.resolve(false);
  },

  


  handleDownArrow: function() {
    if (this.assignment.getStatus() === Status.VALID) {
      return this.requisition.decrement(this.assignment).then(function() {
        this.textChanged();
        this.focusManager.onInputChange();
        return true;
      }.bind(this));
    }

    return Promise.resolve(false);
  },

  


  handleReturn: function(input) {
    
    if (this.requisition.status !== Status.VALID) {
      return Promise.resolve(false);
    }

    this.terminal.history.add(input);
    this.terminal.unsetChoice();

    return this.requisition.exec().then(function() {
      this.textChanged();
      return true;
    }.bind(this));
  },

  



  handleTab: function() {
    
    
    
    this.terminal._caretChange = Caret.TO_ARG_END;
    var inputState = this.terminal.getInputState();
    this._processCaretChange(inputState);

    this.terminal._previousValue = this.terminal.inputElement.value;

    
    
    
    var index = this.terminal.getChoiceIndex();
    return this.requisition.complete(inputState.cursor, index).then(function(updated) {
      
      if (!updated) {
        return RESOLVED;
      }
      this.textChanged();
      return this.terminal.unsetChoice();
    }.bind(this));
  },

  


  handleInput: function(value) {
    this.terminal._caretChange = Caret.NO_CHANGE;

    return this.requisition.update(value).then(function(updated) {
      
      if (!updated) {
        return RESOLVED;
      }
      this.textChanged();
      return this.terminal.unsetChoice();
    }.bind(this));
  },

  



  setCursor: function(cursor) {
    this._caretChange = Caret.NO_CHANGE;
    this._processCaretChange({
      typed: this.terminal.inputElement.value,
      cursor: cursor
    });
  },

  




  _processCaretChange: function(input) {
    var start, end;
    switch (this._caretChange) {
      case Caret.SELECT_ALL:
        start = 0;
        end = input.typed.length;
        break;

      case Caret.TO_END:
        start = input.typed.length;
        end = input.typed.length;
        break;

      case Caret.TO_ARG_END:
        
        
        
        start = input.cursor.start;
        do {
          start++;
        }
        while (start < input.typed.length && input.typed[start - 1] !== ' ');

        end = start;
        break;

      default:
        start = input.cursor.start;
        end = input.cursor.end;
        break;
    }

    start = (start > input.typed.length) ? input.typed.length : start;
    end = (end > input.typed.length) ? input.typed.length : end;

    var newInput = {
      typed: input.typed,
      cursor: { start: start, end: end }
    };

    if (this.terminal.inputElement.selectionStart !== start) {
      this.terminal.inputElement.selectionStart = start;
    }
    if (this.terminal.inputElement.selectionEnd !== end) {
      this.terminal.inputElement.selectionEnd = end;
    }

    this.caretMoved(start);

    this._caretChange = null;
    return newInput;
  },

  


  getCompleterTemplateData: function() {
    var input = this.terminal.getInputState();
    var start = input.cursor.start;
    var index = this.terminal.getChoiceIndex();

    return this.requisition.getStateData(start, index).then(function(data) {
      
      
      
      
      
      data.statusMarkup.forEach(function(member) {
        member.string = member.string.replace(/ /g, '\u00a0'); 
        member.className = 'gcli-in-' + member.status.toString().toLowerCase();
      }, this);

      return data;
    });
  },

  


  fieldChanged: function(ev) {
    this.requisition.setAssignment(this.assignment, ev.conversion.arg,
                                   { matchPadding: true }).then(function() {
      this.textChanged();
    }.bind(this));

    var isError = ev.conversion.message != null && ev.conversion.message !== '';
    this.focusManager.setError(isError);
  },

  


  outputted: function(ev) {
    if (ev.output.hidden) {
      return;
    }

    var template = this.commandDom.cloneNode(true);
    var templateOptions = { stack: 'terminal.html#outputView' };

    var context = this.requisition.conversionContext;
    var data = {
      onclick: context.update,
      ondblclick: context.updateExec,
      language: this,
      output: ev.output,
      promptClass: (ev.output.error ? 'gcli-row-error' : '') +
                   (ev.output.completed ? ' gcli-row-complete' : ''),
      
      rowinEle: null,
      rowoutEle: null,
      throbEle: null,
      promptEle: null
    };

    domtemplate.template(template, data, templateOptions);

    ev.output.promise.then(function() {
      var document = data.rowoutEle.ownerDocument;

      if (ev.output.completed) {
        data.promptEle.classList.add('gcli-row-complete');
      }
      if (ev.output.error) {
        data.promptEle.classList.add('gcli-row-error');
      }

      util.clearElement(data.rowoutEle);

      return ev.output.convert('dom', context).then(function(node) {
        this._linksToNewTab(node);
        data.rowoutEle.appendChild(node);

        var event = document.createEvent('Event');
        event.initEvent('load', true, true);
        event.addedElement = node;
        node.dispatchEvent(event);

        this.terminal.scrollToBottom();
        data.throbEle.style.display = ev.output.completed ? 'none' : 'block';
      }.bind(this));
    }.bind(this)).then(null, console.error);

    this.terminal.addElement(data.rowinEle);
    this.terminal.addElement(data.rowoutEle);
    this.terminal.scrollToBottom();

    this.focusManager.outputted();
  },

  



  _linksToNewTab: function(element) {
    var links = element.querySelectorAll('*[href]');
    for (var i = 0; i < links.length; i++) {
      links[i].setAttribute('target', '_blank');
    }
    return element;
  },

  


  showIntro: function() {
    intro.maybeShowIntro(this.requisition.commandOutputManager,
                         this.requisition.conversionContext);
  },
};






Object.defineProperty(commandLanguage, 'description', {
  get: function() {
    if (this.assignment == null || (
        this.assignment instanceof CommandAssignment &&
        this.assignment.value == null)) {
      return '';
    }

    return this.assignment.param.manual || this.assignment.param.description;
  },
  enumerable: true
});




Object.defineProperty(commandLanguage, 'message', {
  get: function() {
    return this.assignment.conversion.message;
  },
  enumerable: true
});

exports.items = [ commandLanguage ];
