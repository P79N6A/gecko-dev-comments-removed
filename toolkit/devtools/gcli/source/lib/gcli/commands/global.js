















'use strict';

var l10n = require('../util/l10n');

exports.items = [
  {
    
    item: 'type',
    name: 'global',
    parent: 'selection',
    remote: true,
    lookup: function(context) {
      var knownWindows = context.environment.window == null ?
                         [ ] : [ context.environment.window ];

      this.last = findWindows(knownWindows).map(function(window) {
        return { name: windowToString(window), value: window };
      });

      return this.last;
    }
  },
  {
    
    item: 'command',
    runAt: 'client',
    name: 'global',
    description: l10n.lookup('globalDesc'),
    params: [
      {
        name: 'window',
        type: 'global',
        description: l10n.lookup('globalWindowDesc'),
      }
    ],
    returnType: 'string',
    exec: function(args, context) {
      context.shell.global = args.window;
      return l10n.lookupFormat('globalOutput', [ windowToString(args.window) ]);
    }
  }
];

function windowToString(win) {
  return win.location ? win.location.href : 'NodeJS-Global';
}

function findWindows(knownWindows) {
  knownWindows.forEach(function(window) {
    addChildWindows(window, knownWindows);
  });
  return knownWindows;
}

function addChildWindows(win, knownWindows) {
  var iframes = win.document.querySelectorAll('iframe');
  [].forEach.call(iframes, function(iframe) {
    var iframeWin = iframe.contentWindow;
    if (knownWindows.indexOf(iframeWin) === -1) {
      knownWindows.push(iframeWin);
      addChildWindows(iframeWin, knownWindows);
    }
  });
}
