(function(global) {
  'use strict';
  
  
  
  
  global.rtcIdentityProvider.register({
    generateAssertion: function(payload, origin, usernameHint) {
      dump('idp: generateAssertion(' + payload + ')\n');
      return Promise.resolve({
        idp: { domain: 'example.com', protocol: 'idp.js' },
        assertion: 'bogus'
      });
    },

    validateAssertion: function(assertion, origin) {
      dump('idp: validateAssertion(' + assertion + ')\n');
      return Promise.resolve({
        identity: 'user@example.com',
        contents: 'bogus'
      });
    }
  });
}(this));
