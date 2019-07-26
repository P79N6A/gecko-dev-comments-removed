















'use strict';

var util = require('../util/util');





exports.items = [
  {
    item: 'converter',
    from: 'terminal',
    to: 'dom',
    createTextArea: function(text, conversionContext) {
      var node = util.createElement(conversionContext.document, 'textarea');
      node.classList.add('gcli-row-subterminal');
      node.readOnly = true;
      node.textContent = text;
      return node;
    },
    exec: function(data, conversionContext) {
      if (Array.isArray(data)) {
        var node = util.createElement(conversionContext.document, 'div');
        data.forEach(function(member) {
          node.appendChild(this.createTextArea(member, conversionContext));
        });
        return node;
      }
      return this.createTextArea(data);
    }
  },
  {
    item: 'converter',
    from: 'terminal',
    to: 'string',
    exec: function(data, conversionContext) {
      return Array.isArray(data) ? data.join('') : '' + data;
    }
  }
];
