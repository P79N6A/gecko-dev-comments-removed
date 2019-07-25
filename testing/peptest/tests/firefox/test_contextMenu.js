










































Components.utils.import("resource://mozmill/driver/mozmill.js");
let c = getBrowserController();


c.open("http://mozilla.org");
c.waitForPageLoad();




pep.performAction('content_reload', function() {
  
  
  c.rootElement.rightClick();
  c.rootElement.keypress('r');
});
c.waitForPageLoad();

c.open("http://mozillians.org");
c.waitForPageLoad();


pep.performAction('content_back', function() {
  c.rootElement.rightClick();
  c.rootElement.keypress('b');
});

c.sleep(500);


page = findElement.ID(c.tabs.activeTab, 'home');

pep.performAction('content_scroll', function() {
  page.rightClick();
  for (let i = 0; i < 15; ++i) {
    page.keypress('VK_DOWN');
    
    c.sleep(10);
  }
});


let bar = findElement.ID(c.window.document, "toolbar-menubar");
bar.click();
pep.performAction('chrome_navigation', function() {
  bar.rightClick();
  bar.keypress('n');
});

pep.performAction('chrome_addon', function() {
  bar.rightClick();
  bar.keypress('a');
});

pep.performAction('chrome_scroll', function() {
  bar.rightClick();
  for (let i = 0; i < 15; ++i) {
    bar.keypress('VK_DOWN');
    
    c.sleep(10);
  }
});
