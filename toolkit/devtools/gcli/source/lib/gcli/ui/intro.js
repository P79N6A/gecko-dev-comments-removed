















'use strict';

var l10n = require('../util/l10n');
var settings = require('../settings');
var Output = require('../cli').Output;
var view = require('./view');




exports.items = [
  {
    item: 'setting',
    name: 'hideIntro',
    type: 'boolean',
    description: l10n.lookup('hideIntroDesc'),
    defaultValue: false
  }
];




exports.maybeShowIntro = function(commandOutputManager, conversionContext) {
  var hideIntro = settings.getSetting('hideIntro');
  if (hideIntro.value) {
    return;
  }

  var output = new Output();
  output.type = 'view';
  commandOutputManager.onOutput({ output: output });

  var viewData = this.createView(null, conversionContext, true);

  output.complete({ isTypedData: true, type: 'view', data: viewData });
};




exports.createView = function(ignoreArgs, conversionContext, showHideButton) {
  return view.createView({
    html:
      '<div save="${mainDiv}">\n' +
      '  <p>${l10n.introTextOpening3}</p>\n' +
      '\n' +
      '  <p>\n' +
      '    ${l10n.introTextCommands}\n' +
      '    <span class="gcli-out-shortcut" onclick="${onclick}"\n' +
      '        ondblclick="${ondblclick}"\n' +
      '        data-command="help">help</span>${l10n.introTextKeys2}\n' +
      '    <code>${l10n.introTextF1Escape}</code>.\n' +
      '  </p>\n' +
      '\n' +
      '  <button onclick="${onGotIt}"\n' +
      '      if="${showHideButton}">${l10n.introTextGo}</button>\n' +
      '</div>',
    options: { stack: 'intro.html' },
    data: {
      l10n: l10n.propertyLookup,
      onclick: conversionContext.update,
      ondblclick: conversionContext.updateExec,
      showHideButton: showHideButton,
      onGotIt: function(ev) {
        var hideIntro = settings.getSetting('hideIntro');
        hideIntro.value = true;
        this.mainDiv.style.display = 'none';
      }
    }
  });
};
