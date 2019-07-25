





































Components.utils.import('resource://mozmill/driver/mozmill.js');
let c = getBrowserController();

c.open('http://mozilla.org');
c.waitForPageLoad();


let page = findElement.ID(c.tabs.activeTab, "home");
pep.performAction('open_blank_tab', function() {
  page.keypress('t', {'ctrlKey': true});
});

pep.performAction('close_blank_tab', function() {
  page.keypress('w', {'ctrlKey': true});
});
