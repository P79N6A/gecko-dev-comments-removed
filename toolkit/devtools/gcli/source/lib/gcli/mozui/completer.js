















'use strict';

var util = require('../util/util');
var host = require('../util/host');
var domtemplate = require('../util/domtemplate');

var completerHtml =
  '<description\n' +
  '    xmlns="http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul">\n' +
  '  <loop foreach="member in ${statusMarkup}">\n' +
  '    <label class="${member.className}" value="${member.string}"></label>\n' +
  '  </loop>\n' +
  '  <label class="gcli-in-ontab" value="${directTabText}"/>\n' +
  '  <label class="gcli-in-todo" foreach="param in ${emptyParameters}" value="${param}"/>\n' +
  '  <label class="gcli-in-ontab" value="${arrowTabText}"/>\n' +
  '  <label class="gcli-in-closebrace" if="${unclosedJs}" value="}"/>\n' +
  '</description>\n';










function Completer(components) {
  this.requisition = components.requisition;
  this.input = { typed: '', cursor: { start: 0, end: 0 } };
  this.choice = 0;

  this.element = components.element;
  this.element.classList.add('gcli-in-complete');
  this.element.setAttribute('tabindex', '-1');
  this.element.setAttribute('aria-live', 'polite');

  this.document = this.element.ownerDocument;

  this.inputter = components.inputter;

  this.inputter.onInputChange.add(this.update, this);
  this.inputter.onAssignmentChange.add(this.update, this);
  this.inputter.onChoiceChange.add(this.update, this);

  this.autoResize = components.autoResize;
  if (this.autoResize) {
    this.inputter.onResize.add(this.resized, this);

    var dimensions = this.inputter.getDimensions();
    if (dimensions) {
      this.resized(dimensions);
    }
  }

  this.template = host.toDom(this.document, completerHtml);
  
  util.removeWhitespace(this.template, true);

  this.update();
}




Completer.prototype.destroy = function() {
  this.inputter.onInputChange.remove(this.update, this);
  this.inputter.onAssignmentChange.remove(this.update, this);
  this.inputter.onChoiceChange.remove(this.update, this);

  if (this.autoResize) {
    this.inputter.onResize.remove(this.resized, this);
  }

  this.document = undefined;
  this.element = undefined;
  this.template = undefined;
  this.inputter = undefined;
};




Completer.prototype.resized = function(ev) {
  this.element.style.top = ev.top + 'px';
  this.element.style.height = ev.height + 'px';
  this.element.style.lineHeight = ev.height + 'px';
  this.element.style.left = ev.left + 'px';
  this.element.style.width = ev.width + 'px';
};




Completer.prototype.update = function(ev) {
  this.choice = (ev && ev.choice != null) ? ev.choice : 0;

  this._getCompleterTemplateData().then(function(data) {
    if (this.template == null) {
      return; 
    }

    var template = this.template.cloneNode(true);
    domtemplate.template(template, data, { stack: 'completer.html' });

    util.clearElement(this.element);
    while (template.hasChildNodes()) {
      this.element.appendChild(template.firstChild);
    }
  }.bind(this));
};




Completer.prototype._getCompleterTemplateData = function() {
  var input = this.inputter.getInputState();
  var start = input.cursor.start;

  return this.requisition.getStateData(start, this.choice).then(function(data) {
    
    
    
    
    
    data.statusMarkup.forEach(function(member) {
      member.string = member.string.replace(/ /g, '\u00a0'); 
      member.className = 'gcli-in-' + member.status.toString().toLowerCase();
    }, this);

    return data;
  });
};

exports.Completer = Completer;
