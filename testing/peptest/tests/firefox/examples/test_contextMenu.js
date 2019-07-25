











































Components.utils.import("resource://mozmill/driver/mozmill.js");
let c = getBrowserController();


c.open("http://mozilla.org");
c.waitForPageLoad();


let page = findElement.ID(c.tabs.activeTab, 'header');



pep.performAction('content_reload', function() {
  page.rightClick();
  page.keypress('r');
});
c.waitForPageLoad();

c.open("http://google.com");
c.waitForPageLoad();

page = findElement.ID(c.tabs.activeTab, 'main');

pep.performAction('content_back', function() {
  page.rightClick();
  page.keypress('b');
});

c.sleep(100);

page = findElement.ID(c.tabs.activeTab, 'home');

pep.performAction('content_scroll', function() {
  page.rightClick();
  for (let i = 0; i < 15; ++i) {
    page.keypress('VK_DOWN');
    
    c.sleep(10);
  }
});


let bar = findElement.ID(c.window.document, "appmenu-toolbar-button");
bar.click();
pep.performAction('chrome_menu', function() {
  bar.rightClick();
  bar.keypress('m');
});

pep.performAction('chrome_addon', function() {
  bar.rightClick();
  bar.keypress('a');
});

pep.performAction('chrome_scroll', function() {
  bar.rightClick();
  for (let i = 0; i < 15; ++i) {
    page.keypress('VK_DOWN');
    
    c.sleep(10);
  }
});

