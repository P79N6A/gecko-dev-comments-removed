



'use strict';


(function(window) {
  var gLanguage = '';

  
  function getL10nData(key) {
    var response = FirefoxCom.requestSync('getStrings', key);
    var data = JSON.parse(response);
    if (!data)
      console.warn('[l10n] #' + key + ' missing for [' + gLanguage + ']');
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
    var data = getL10nData(key);
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

  window.addEventListener('DOMContentLoaded', function() {
    gLanguage = FirefoxCom.requestSync('getLocale', null);

    translateFragment();

    
    var evtObject = document.createEvent('Event');
    evtObject.initEvent('localized', false, false);
    evtObject.language = gLanguage;
    window.dispatchEvent(evtObject);
  });

  
  document.mozL10n = {
    
    get: translateString,

    
    getLanguage: function() { return gLanguage; },

    
    getDirection: function() {
      
      
      var rtlList = ['ar', 'he', 'fa', 'ps', 'ur'];
      return (rtlList.indexOf(gLanguage) >= 0) ? 'rtl' : 'ltr';
    }
  };
})(this);

