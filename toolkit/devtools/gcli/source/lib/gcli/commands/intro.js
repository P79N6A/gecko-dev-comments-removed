















'use strict';

var l10n = require('../util/l10n');
var intro = require('../ui/intro');

exports.items = [
  {
    item: 'converter',
    from: 'intro',
    to: 'view',
    exec: intro.createView
  },
  {
    item: 'command',
    name: 'intro',
    description: l10n.lookup('introDesc'),
    manual: l10n.lookup('introManual'),
    returnType: 'intro',
    exec: function(args, context) {
      
    }
  }
];
