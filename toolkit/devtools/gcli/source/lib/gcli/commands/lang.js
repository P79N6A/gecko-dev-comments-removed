















'use strict';

var l10n = require('../util/l10n');
var cli = require('../cli');

exports.items = [
  {
    
    item: 'type',
    name: 'language',
    parent: 'selection',
    lookup: function(context) {
      return context.system.languages.getAll().map(function(language) {
        return { name: language.name, value: language };
      });
    }
  },
  {
    
    item: 'command',
    name: 'lang',
    description: l10n.lookup('langDesc'),
    params: [
      {
        name: 'language',
        type: 'language'
      }
    ],
    returnType: 'view',
    exec: function(args, context) {
      var terminal = cli.getMapping(context).terminal;

      context.environment.window.setTimeout(function() {
        terminal.switchLanguage(args.language);
      }, 10);

      return {
        html:
          '<div class="gcli-section ${style}">' +
          '  ${langOutput}' +
          '</div>',
        data: {
          langOutput: l10n.lookupFormat('langOutput', [ args.language.name ]),
          style: args.language.proportionalFonts ? '' : 'gcli-row-script'
        }
      };
    }
  }
];
