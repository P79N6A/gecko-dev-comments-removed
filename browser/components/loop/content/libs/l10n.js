


'use strict';






(function(window) {
  var gL10nDetails;
  var gLanguage = '';

  
  function getL10nData(key, num) {
    var response = gL10nDetails.getStrings(key);
    var data = JSON.parse(response);
    if (!data)
      console.warn('[l10n] #' + key + ' missing for [' + gLanguage + ']');
    if (num !== undefined) {
      for (var prop in data) {
        data[prop] = gL10nDetails.getPluralForm(num, data[prop]);
      }
    }
    return data;
  }

  
  function substArguments(text, args) {
    if (!args)
      return text;

    return text.replace(/\{\{\s*(\w+)\s*\}\}/g, function(all, name) {
      return name in args ? args[name] : '{{' + name + '}}';
    });
  }

  
  function translateString(key, args, fallback) {
    if (args && args.num) {
      var num = args && args.num;
      delete args.num;
    }
    var data = getL10nData(key, num);
    if (!data && fallback)
      data = {textContent: fallback};
    if (!data)
      return '{{' + key + '}}';
    return substArguments(data.textContent, args);
  }

  
  function translateElement(element) {
    if (!element || !element.dataset)
      return;

    
    var key = element.dataset.l10nId;
    var data = getL10nData(key);
    if (!data)
      return;

    
    
    var args;
    if (element.dataset.l10nArgs) try {
      args = JSON.parse(element.dataset.l10nArgs);
    } catch (e) {
      console.warn('[l10n] could not parse arguments for #' + key + '');
    }

    
    
    for (var k in data)
      element[k] = substArguments(data[k], args);
  }


  
  function translateFragment(element) {
    element = element || document.querySelector('html');

    
    var children = element.querySelectorAll('*[data-l10n-id]');
    var elementCount = children.length;
    for (var i = 0; i < elementCount; i++)
      translateElement(children[i]);

    
    if (element.dataset.l10nId)
      translateElement(element);
  }

  
  document.mozL10n = {
    






    initialize: function(l10nDetails) {
      gL10nDetails = l10nDetails;
      gLanguage = gL10nDetails.locale;

      translateFragment();
    },

    
    get: translateString,

    
    getLanguage: function() { return gLanguage; },

    
    getDirection: function() {
      
      
      var rtlList = ['ar', 'he', 'fa', 'ps', 'ur'];
      return (rtlList.indexOf(gLanguage) >= 0) ? 'rtl' : 'ltr';
    },

    
    translate: translateFragment
  };
})(this);
