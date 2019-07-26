















'use strict';

var l10n = require('../util/l10n');
var settings = require('../settings');
var Promise = require('../util/promise').Promise;




var prefsData = {
  item: 'converter',
  from: 'prefsData',
  to: 'view',
  exec: function(prefsData, conversionContext) {
    var prefList = new PrefList(prefsData, conversionContext);
    return {
      html:
        '<div ignore="${onLoad(__element)}">\n' +
        '  <!-- This is broken, and unimportant. Comment out for now\n' +
        '  <div class="gcli-pref-list-filter">\n' +
        '    ${l10n.prefOutputFilter}:\n' +
        '    <input onKeyUp="${onFilterChange}" value="${search}"/>\n' +
        '  </div>\n' +
        '  -->\n' +
        '  <table class="gcli-pref-list-table">\n' +
        '    <colgroup>\n' +
        '      <col class="gcli-pref-list-name"/>\n' +
        '      <col class="gcli-pref-list-value"/>\n' +
        '    </colgroup>\n' +
        '    <tr>\n' +
        '      <th>${l10n.prefOutputName}</th>\n' +
        '      <th>${l10n.prefOutputValue}</th>\n' +
        '    </tr>\n' +
        '  </table>\n' +
        '  <div class="gcli-pref-list-scroller">\n' +
        '    <table class="gcli-pref-list-table" save="${table}">\n' +
        '    </table>\n' +
        '  </div>\n' +
        '</div>\n',
      data: prefList,
      options: {
        blankNullUndefined: true,
        allowEval: true,
        stack: 'prefsData->view'
      },
      css:
        '.gcli-pref-list-scroller {\n' +
        '  max-height: 200px;\n' +
        '  overflow-y: auto;\n' +
        '  overflow-x: hidden;\n' +
        '  display: inline-block;\n' +
        '}\n' +
        '\n' +
        '.gcli-pref-list-table {\n' +
        '  width: 500px;\n' +
        '  table-layout: fixed;\n' +
        '}\n' +
        '\n' +
        '.gcli-pref-list-table tr > th {\n' +
        '  text-align: left;\n' +
        '}\n' +
        '\n' +
        '.gcli-pref-list-table tr > td {\n' +
        '  text-overflow: elipsis;\n' +
        '  word-wrap: break-word;\n' +
        '}\n' +
        '\n' +
        '.gcli-pref-list-name {\n' +
        '  width: 70%;\n' +
        '}\n' +
        '\n' +
        '.gcli-pref-list-command {\n' +
        '  display: none;\n' +
        '}\n' +
        '\n' +
        '.gcli-pref-list-row:hover .gcli-pref-list-command {\n' +
        '  /* \'pref list\' is a bit broken and unimportant. Band-aid follows */\n' +
        '  /* display: inline-block; */\n' +
        '}\n',
      cssId: 'gcli-pref-list'
    };
  }
};




var prefList = {
  item: 'command',
  name: 'pref list',
  description: l10n.lookup('prefListDesc'),
  manual: l10n.lookup('prefListManual'),
  params: [
    {
      name: 'search',
      type: 'string',
      defaultValue: null,
      description: l10n.lookup('prefListSearchDesc'),
      manual: l10n.lookup('prefListSearchManual')
    }
  ],
  returnType: 'prefsData',
  exec: function(args, context) {
    return new Promise(function(resolve, reject) {
      
      setTimeout(function() {
        var prefsData = {
          settings: settings.getAll(args.search),
          search: args.search
        };
        resolve(prefsData);
      }.bind(this), 10);
    });
  }
};




function PrefList(prefsData, conversionContext) {
  this.search = prefsData.search;
  this.settings = prefsData.settings;
  this.conversionContext = conversionContext;
}





PrefList.prototype.onLoad = function(element) {
  var table = element.querySelector('.gcli-pref-list-table');
  this.updateTable(table);
  return '';
};




PrefList.prototype.l10n = l10n.propertyLookup;




PrefList.prototype.updateTable = function(table) {
  var view = this.conversionContext.createView({
    html:
      '<table>\n' +
      '  <colgroup>\n' +
      '    <col class="gcli-pref-list-name"/>\n' +
      '    <col class="gcli-pref-list-value"/>\n' +
      '  </colgroup>\n' +
      '  <tr class="gcli-pref-list-row" foreach="setting in ${settings}">\n' +
      '    <td>${setting.name}</td>\n' +
      '    <td onclick="${onSetClick}" data-command="pref set ${setting.name} ">\n' +
      '      ${setting.value}\n' +
      '      [Edit]\n' +
      '    </td>\n' +
      '  </tr>\n' +
      '</table>\n',
    options: { blankNullUndefined: true, stack: 'prefsData#inner' },
    data: this
  });

  view.appendTo(table, true);
};

PrefList.prototype.onFilterChange = function(ev) {
  if (ev.target.value !== this.search) {
    this.search = ev.target.value;

    var root = ev.target.parentNode.parentNode;
    var table = root.querySelector('.gcli-pref-list-table');
    this.updateTable(table);
  }
};

PrefList.prototype.onSetClick = function(ev) {
  var typed = ev.currentTarget.getAttribute('data-command');
  this.conversionContext.update(typed);
};

exports.items = [ prefsData, prefList ];
