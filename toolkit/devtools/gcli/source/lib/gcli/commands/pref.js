















'use strict';

var l10n = require('../util/l10n');

exports.items = [
  {
    
    item: 'command',
    name: 'pref',
    description: l10n.lookup('prefDesc'),
    manual: l10n.lookup('prefManual')
  },
  {
    
    item: 'command',
    name: 'pref show',
    runAt: 'client',
    description: l10n.lookup('prefShowDesc'),
    manual: l10n.lookup('prefShowManual'),
    params: [
      {
        name: 'setting',
        type: 'setting',
        description: l10n.lookup('prefShowSettingDesc'),
        manual: l10n.lookup('prefShowSettingManual')
      }
    ],
    exec: function(args, context) {
      return l10n.lookupFormat('prefShowSettingValue',
                               [ args.setting.name, args.setting.value ]);
    }
  },
  {
    
    item: 'command',
    name: 'pref set',
    runAt: 'client',
    description: l10n.lookup('prefSetDesc'),
    manual: l10n.lookup('prefSetManual'),
    params: [
      {
        name: 'setting',
        type: 'setting',
        description: l10n.lookup('prefSetSettingDesc'),
        manual: l10n.lookup('prefSetSettingManual')
      },
      {
        name: 'value',
        type: 'settingValue',
        description: l10n.lookup('prefSetValueDesc'),
        manual: l10n.lookup('prefSetValueManual')
      }
    ],
    exec: function(args, context) {
      args.setting.value = args.value;
    }
  },
  {
    
    item: 'command',
    name: 'pref reset',
    runAt: 'client',
    description: l10n.lookup('prefResetDesc'),
    manual: l10n.lookup('prefResetManual'),
    params: [
      {
        name: 'setting',
        type: 'setting',
        description: l10n.lookup('prefResetSettingDesc'),
        manual: l10n.lookup('prefResetSettingManual')
      }
    ],
    exec: function(args, context) {
      args.setting.setDefault();
    }
  }
];
