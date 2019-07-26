















'use strict';

var util = require('../util/util');






exports.items = [
  {
    item: 'converter',
    from: 'html',
    to: 'dom',
    exec: function(html, conversionContext) {
      var div = util.createElement(conversionContext.document, 'div');
      div.innerHTML = html;
      return div;
    }
  },
  {
    item: 'converter',
    from: 'html',
    to: 'string',
    exec: function(html, conversionContext) {
      var div = util.createElement(conversionContext.document, 'div');
      div.innerHTML = html;
      return div.textContent;
    }
  }
];
