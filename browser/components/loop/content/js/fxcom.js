



Components.utils.import("resource://gre/modules/Services.jsm");
const Ci = Components.interfaces;









var FirefoxCom = (function FirefoxComClosure() {
  return {
    


    requestSync: function(action, data) {
      if (action === 'getLocale')
        return this.getLocale();

      if (action === 'getStrings')
        return this.getStrings(data);

      console.error('[fxcom] Action' + action + ' is unknown');
      return null;
    },

    getLocale: function() {
      try {
        return Services.prefs.getCharPref('general.useragent.locale');
      } catch (x) {
        return 'en-US';
      }
    },

    getStrings: function(data) {
      try {
        return JSON.stringify(this.localizedStrings[data] || null);
      } catch (e) {
        console.error('[fxcom] Unable to retrive localized strings: ' + e);
      }
    },

    get localizedStrings() {
      var stringBundle =
        Services.strings.createBundle('chrome://browser/locale/loop/loop.properties');

      var map = {};
      var enumerator = stringBundle.getSimpleEnumeration();
      while (enumerator.hasMoreElements()) {
        var string = enumerator.getNext().QueryInterface(Ci.nsIPropertyElement);
        var key = string.key, property = 'textContent';
        var i = key.lastIndexOf('.');
        if (i >= 0) {
          property = key.substring(i + 1);
          key = key.substring(0, i);
        }
        if (!(key in map))
          map[key] = {};
        map[key][property] = string.value;
      }
      delete this.localizedStrings;
      return this.localizedStrings = map;
    }
  };
})();
