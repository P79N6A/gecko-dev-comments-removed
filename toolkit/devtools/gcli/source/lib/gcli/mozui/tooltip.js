















'use strict';

var util = require('../util/util');
var host = require('../util/host');
var domtemplate = require('../util/domtemplate');

var CommandAssignment = require('../cli').CommandAssignment;

var tooltipHtml =
  '<div class="gcli-tt" aria-live="polite">\n' +
  '  <div class="gcli-tt-description" save="${descriptionEle}">${description}</div>\n' +
  '  ${field.element}\n' +
  '  <div class="gcli-tt-error" save="${errorEle}">${assignment.conversion.message}</div>\n' +
  '  <div class="gcli-tt-highlight" save="${highlightEle}"></div>\n' +
  '</div>';














function Tooltip(options, components) {
  this.inputter = components.inputter;
  this.requisition = components.requisition;
  this.focusManager = components.focusManager;

  this.element = components.element;
  this.element.classList.add(options.tooltipClass || 'gcli-tooltip');
  this.document = this.element.ownerDocument;

  this.panelElement = components.panelElement;
  if (this.panelElement) {
    this.panelElement.classList.add('gcli-panel-hide');
    this.focusManager.onVisibilityChange.add(this.visibilityChanged, this);
  }
  this.focusManager.addMonitoredElement(this.element, 'tooltip');

  
  this.fields = [];

  this.template = host.toDom(this.document, tooltipHtml);
  this.templateOptions = { blankNullUndefined: true, stack: 'tooltip.html' };

  this.inputter.onChoiceChange.add(this.choiceChanged, this);
  this.inputter.onAssignmentChange.add(this.assignmentChanged, this);

  
  this.assignment = undefined;
  this.assignmentChanged({ assignment: this.inputter.assignment });

  
  this.lastText = undefined;
}




Tooltip.prototype.destroy = function() {
  this.inputter.onAssignmentChange.remove(this.assignmentChanged, this);
  this.inputter.onChoiceChange.remove(this.choiceChanged, this);

  if (this.panelElement) {
    this.focusManager.onVisibilityChange.remove(this.visibilityChanged, this);
  }
  this.focusManager.removeMonitoredElement(this.element, 'tooltip');

  if (this.style) {
    this.style.parentNode.removeChild(this.style);
    this.style = undefined;
  }

  this.field.onFieldChange.remove(this.fieldChanged, this);
  this.field.destroy();

  this.lastText = undefined;
  this.assignment = undefined;

  this.errorEle = undefined;
  this.descriptionEle = undefined;
  this.highlightEle = undefined;

  this.document = undefined;
  this.element = undefined;
  this.panelElement = undefined;
  this.template = undefined;
};




Object.defineProperty(Tooltip.prototype, 'isMenuShowing', {
  get: function() {
    return this.focusManager.isTooltipVisible &&
           this.field != null &&
           this.field.menu != null;
  },
  enumerable: true
});




Tooltip.prototype.assignmentChanged = function(ev) {
  
  
  
  if (this.assignment === ev.assignment) {
    return;
  }

  this.assignment = ev.assignment;
  this.lastText = this.assignment.arg.text;

  if (this.field) {
    this.field.onFieldChange.remove(this.fieldChanged, this);
    this.field.destroy();
  }

  this.field = this.requisition.system.fields.get(this.assignment.param.type, {
    document: this.document,
    requisition: this.requisition
  });

  this.focusManager.setImportantFieldFlag(this.field.isImportant);

  this.field.onFieldChange.add(this.fieldChanged, this);
  this.field.setConversion(this.assignment.conversion);

  
  this.errorEle = undefined;
  this.descriptionEle = undefined;
  this.highlightEle = undefined;

  var contents = this.template.cloneNode(true);
  domtemplate.template(contents, this, this.templateOptions);
  util.clearElement(this.element);
  this.element.appendChild(contents);
  this.element.style.display = 'block';

  this.field.setMessageElement(this.errorEle);

  this._updatePosition();
};




Tooltip.prototype.choiceChanged = function(ev) {
  if (this.field && this.field.menu) {
    var conversion = this.assignment.conversion;
    var context = this.requisition.executionContext;
    conversion.constrainPredictionIndex(context, ev.choice).then(function(choice) {
      this.field.menu._choice = choice;
      this.field.menu._updateHighlight();
    }.bind(this)).then(null, util.errorHandler);
  }
};






Tooltip.prototype.selectChoice = function(ev) {
  if (this.field && this.field.selectChoice) {
    return this.field.selectChoice();
  }
  return false;
};




Tooltip.prototype.fieldChanged = function(ev) {
  this.requisition.setAssignment(this.assignment, ev.conversion.arg,
                                 { matchPadding: true });

  var isError = ev.conversion.message != null && ev.conversion.message !== '';
  this.focusManager.setError(isError);

  
  
  
  this.document.defaultView.setTimeout(function() {
    this.inputter.focus();
  }.bind(this), 10);
};




Tooltip.prototype.textChanged = function() {
  
  
  if (this.assignment.arg.text === this.lastText) {
    return;
  }

  this.lastText = this.assignment.arg.text;

  this.field.setConversion(this.assignment.conversion);
  util.setTextContent(this.descriptionEle, this.description);

  this._updatePosition();
};




Tooltip.prototype._updatePosition = function() {
  var dimensions = this.getDimensionsOfAssignment();

  
  if (this.panelElement) {
    this.panelElement.style.left = (dimensions.start * 10) + 'px';
  }

  this.focusManager.updatePosition(dimensions);
};






Tooltip.prototype.getDimensionsOfAssignment = function() {
  var before = '';
  var assignments = this.requisition.getAssignments(true);
  for (var i = 0; i < assignments.length; i++) {
    if (assignments[i] === this.assignment) {
      break;
    }
    before += assignments[i].toString();
  }
  before += this.assignment.arg.prefix;

  var startChar = before.length;
  before += this.assignment.arg.text;
  var endChar = before.length;

  return { start: startChar, end: endChar };
};






Object.defineProperty(Tooltip.prototype, 'description', {
  get: function() {
    if (this.assignment instanceof CommandAssignment &&
            this.assignment.value == null) {
      return '';
    }

    return this.assignment.param.manual || this.assignment.param.description;
  },
  enumerable: true
});




Tooltip.prototype.visibilityChanged = function(ev) {
  if (!this.panelElement) {
    return;
  }

  if (ev.tooltipVisible) {
    this.panelElement.classList.remove('gcli-panel-hide');
  }
  else {
    this.panelElement.classList.add('gcli-panel-hide');
  }
};

exports.Tooltip = Tooltip;
