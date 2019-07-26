















'use strict';

var strings = {};










exports.registerStringsSource = function(modulePath) {
  
  var additions = require(modulePath).root;
  Object.keys(additions).forEach(function(key) {
    if (strings[key]) {
      console.error('Key \'' + key + '\' (loaded from ' + modulePath + ') ' +
          'already exists. Ignoring.');
      return;
    }
    strings[key] = additions[key];
  }, this);
};






require('../nls/strings');
exports.registerStringsSource('../nls/strings');






exports.unregisterStringsSource = function(modulePath) {
  
  var additions = require(modulePath).root;
  Object.keys(additions).forEach(function(key) {
    delete strings[key];
  }, this);
};

















exports.getPreferredLocales = function() {
  var language = typeof navigator !== 'undefined' ?
      (navigator.language || navigator.userLanguage).toLowerCase() :
      'en-us';
  var parts = language.split('-');
  var reply = parts.map(function(part, index) {
    return parts.slice(0, parts.length - index).join('-');
  });
  reply.push('root');
  return reply;
};














exports.lookup = function(key) {
  var str = strings[key];
  if (str == null) {
    throw new Error('No i18n key: ' + key);
  }
  return str;
};









if (typeof Proxy !== 'undefined') {
  exports.propertyLookup = Proxy.create({
    get: function(rcvr, name) {
      return exports.lookup(name);
    }
  });
}
else {
  exports.propertyLookup = strings;
}











function swap(str, swaps) {
  return str.replace(/\{[^}]*\}/g, function(name) {
    name = name.slice(1, -1);
    if (swaps == null) {
      console.log('Missing swaps while looking up \'' + name + '\'');
      return '';
    }
    var replacement = swaps[name];
    if (replacement == null) {
      console.log('Can\'t find \'' + name + '\' in ' + JSON.stringify(swaps));
      replacement = '';
    }
    return replacement;
  });
}






















exports.lookupSwap = function(key, swaps) {
  var str = exports.lookup(key);
  return swap(str, swaps);
};





function format(str, swaps) {
  
  var index = 0;
  str = str.replace(/%S/g, function() {
    return swaps[index++];
  });
  
  str = str.replace(/%([0-9])\$S/g, function(match, idx) {
    return swaps[idx - 1];
  });
  return str;
}






















exports.lookupFormat = function(key, swaps) {
  var str = exports.lookup(key);
  return format(str, swaps);
};






































exports.lookupPlural = function(key, ord, swaps) {
  var index = getPluralRule().get(ord);
  var words = exports.lookup(key);
  var str = words[index];

  swaps = swaps || {};
  swaps.ord = ord;

  return swap(str, swaps);
};





function getPluralRule() {
  if (!pluralRule) {
    var lang = navigator.language || navigator.userLanguage;
    
    pluralRules.some(function(rule) {
      if (rule.locales.indexOf(lang) !== -1) {
        pluralRule = rule;
        return true;
      }
      return false;
    });

    
    if (!pluralRule) {
      console.error('Failed to find plural rule for ' + lang);
      pluralRule = pluralRules[0];
    }
  }

  return pluralRule;
}


















var pluralRules = [
  



  {
    locales: [
      'fa', 'fa-ir',
      'id',
      'ja', 'ja-jp-mac',
      'ka',
      'ko', 'ko-kr',
      'th', 'th-th',
      'tr', 'tr-tr',
      'zh', 'zh-tw', 'zh-cn'
    ],
    numForms: 1,
    get: function(n) {
      return 0;
    }
  },

  











  {
    locales: [
      'af', 'af-za',
      'as', 'ast',
      'bg',
      'br',
      'bs', 'bs-ba',
      'ca',
      'cy', 'cy-gb',
      'da',
      'de', 'de-de', 'de-ch',
      'en', 'en-gb', 'en-us', 'en-za',
      'el', 'el-gr',
      'eo',
      'es', 'es-es', 'es-ar', 'es-cl', 'es-mx',
      'et', 'et-ee',
      'eu',
      'fi', 'fi-fi',
      'fy', 'fy-nl',
      'gl', 'gl-gl',
      'he',
     
      'hu', 'hu-hu',
      'hy', 'hy-am',
      'it', 'it-it',
      'kk',
      'ku',
      'lg',
      'mai',
     
      'ml', 'ml-in',
      'mn',
      'nb', 'nb-no',
      'no', 'no-no',
      'nl',
      'nn', 'nn-no',
      'no', 'no-no',
      'nb', 'nb-no',
      'nso', 'nso-za',
      'pa', 'pa-in',
      'pt', 'pt-pt',
      'rm', 'rm-ch',
     
      'si', 'si-lk',
     
      'son', 'son-ml',
      'sq', 'sq-al',
      'sv', 'sv-se',
      'vi', 'vi-vn',
      'zu', 'zu-za'
    ],
    numForms: 2,
    get: function(n) {
      return n != 1 ?
        1 :
        0;
    }
  },

  



  {
    locales: [
      'ak', 'ak-gh',
      'bn', 'bn-in', 'bn-bd',
      'fr', 'fr-fr',
      'gu', 'gu-in',
      'kn', 'kn-in',
      'mr', 'mr-in',
      'oc', 'oc-oc',
      'or', 'or-in',
            'pt-br',
      'ta', 'ta-in', 'ta-lk',
      'te', 'te-in'
    ],
    numForms: 2,
    get: function(n) {
      return n > 1 ?
        1 :
        0;
    }
  },

  



  {
    locales: [ 'lv' ],
    numForms: 3,
    get: function(n) {
      return n % 10 == 1 && n % 100 != 11 ?
        1 :
        n !== 0 ?
          2 :
          0;
    }
  },

  



  {
    locales: [ 'gd', 'gd-gb' ],
    numForms: 4,
    get: function(n) {
      return n == 1 || n == 11 ?
        0 :
        n == 2 || n == 12 ?
          1 :
          n > 0 && n < 20 ?
            2 :
            3;
    }
  },

  



  {
    locales: [ 'ro', 'ro-ro' ],
    numForms: 3,
    get: function(n) {
      return n == 1 ?
        0 :
        n === 0 || n % 100 > 0 && n % 100 < 20 ?
          1 :
          2;
    }
  },

  



  {
    locales: [ 'lt' ],
    numForms: 3,
    get: function(n) {
      return n % 10 == 1 && n % 100 != 11 ?
        0 :
        n % 10 >= 2 && (n % 100 < 10 || n % 100 >= 20) ?
          2 :
          1;
    }
  },

  




  {
    locales: [
      'be', 'be-by',
      'hr', 'hr-hr',
      'ru', 'ru-ru',
      'sr', 'sr-rs', 'sr-cs',
      'uk'
    ],
    numForms: 3,
    get: function(n) {
      return n % 10 == 1 && n % 100 != 11 ?
        0 :
        n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 10 || n % 100 >= 20) ?
          1 :
          2;
    }
  },

  



  {
    locales: [ 'cs', 'sk' ],
    numForms: 3,
    get: function(n) {
      return n == 1 ?
        0 :
        n >= 2 && n <= 4 ?
          1 :
          2;
    }
  },

  




  {
    locales: [ 'pl' ],
    numForms: 3,
    get: function(n) {
      return n == 1 ?
        0 :
        n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 10 || n % 100 >= 20) ?
          1 :
          2;
    }
  },

  




  {
    locales: [ 'sl' ],
    numForms: 4,
    get: function(n) {
      return n % 100 == 1 ?
        0 :
        n % 100 == 2 ?
          1 :
          n % 100 == 3 || n % 100 == 4 ?
            2 :
            3;
    }
  },

  



  {
    locales: [ 'ga-ie', 'ga-ie', 'ga', 'en-ie' ],
    numForms: 5,
    get: function(n) {
      return n == 1 ?
        0 :
        n == 2 ?
          1 :
          n >= 3 && n <= 6 ?
            2 :
            n >= 7 && n <= 10 ?
              3 :
              4;
    }
  },

  



  {
    locales: [ 'ar' ],
    numForms: 6,
    get: function(n) {
      return n === 0 ?
        5 :
        n == 1 ?
          0 :
          n == 2 ?
            1 :
            n % 100 >= 3 && n % 100 <= 10 ?
              2 :
              n % 100 >= 11 && n % 100 <= 99 ?
                3 :
                4;
    }
  },

  



  {
    locales: [ 'mt' ],
    numForms: 4,
    get: function(n) {
      return n == 1 ?
        0 :
        n === 0 || n % 100 > 0 && n % 100 <= 10 ?
          1 :
          n % 100 > 10 && n % 100 < 20 ?
            2 :
            3;
    }
  },

  



  {
    locales: [ 'mk', 'mk-mk' ],
    numForms: 3,
    get: function(n) {
      return n % 10 == 1 ?
        0 :
        n % 10 == 2 ?
          1 :
          2;
    }
  },

  



  {
    locales: [ 'is' ],
    numForms: 2,
    get: function(n) {
      return n % 10 == 1 && n % 100 != 11 ?
        0 :
        1;
    }
  }

  




];




var pluralRule;
