





































Components.utils.import("resource://mozmill/driver/mozmill.js");
let controller = getBrowserController();

controller.open("http://google.com");
controller.waitForPageLoad();

let textbox = findElement.ID(controller.tabs.activeTab, 'lst-ib');
let button = findElement.Name(controller.tabs.activeTab, 'btnK');
pep.performAction('enter_text', function() {
  textbox.sendKeys('foobar');
  button.click();
});
