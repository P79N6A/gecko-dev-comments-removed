















'use strict';

var util = require('../util/util');




function nodeFromDataToString(data, conversionContext) {
  var node = util.createElement(conversionContext.document, 'p');
  node.textContent = data.toString();
  return node;
}

exports.items = [
  {
    item: 'converter',
    from: 'string',
    to: 'dom',
    exec: nodeFromDataToString
  },
  {
    item: 'converter',
    from: 'number',
    to: 'dom',
    exec: nodeFromDataToString
  },
  {
    item: 'converter',
    from: 'boolean',
    to: 'dom',
    exec: nodeFromDataToString
  },
  {
    item: 'converter',
    from: 'undefined',
    to: 'dom',
    exec: function(data, conversionContext) {
      return util.createElement(conversionContext.document, 'span');
    }
  },
  {
    item: 'converter',
    from: 'json',
    to: 'view',
    exec: function(json, context) {
      var html = JSON.stringify(json, null, '&#160;').replace(/\n/g, '<br/>');
      return {
        html: '<pre>' + html + '</pre>'
      };
    }
  },
  {
    item: 'converter',
    from: 'number',
    to: 'string',
    exec: function(data) { return '' + data; }
  },
  {
    item: 'converter',
    from: 'boolean',
    to: 'string',
    exec: function(data) { return '' + data; }
  },
  {
    item: 'converter',
    from: 'undefined',
    to: 'string',
    exec: function(data) { return ''; }
  },
  {
    item: 'converter',
    from: 'json',
    to: 'string',
    exec: function(json, conversionContext) {
      return JSON.stringify(json, null, '  ');
    }
  }
];
