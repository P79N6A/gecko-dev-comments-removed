



Components.utils.import('resource://mozmill/driver/mozmill.js');
let c = getBrowserController();

c.open('http://mozilla.org');
c.waitForPageLoad();


pep.performAction('open_blank_tab', function() {
  c.rootElement.keypress('t', {'accelKey': true});
});

pep.performAction('close_blank_tab', function() {
  c.rootElement.keypress('w', {'accelKey': true});
});
