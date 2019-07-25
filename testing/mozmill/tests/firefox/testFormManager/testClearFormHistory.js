




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['ModalDialogAPI','UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var sampleFormSite = "http://www-archive.mozilla.org/wallet/samples/sample9.html";
var fname = "John";
var lname = "Smith";

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();

  
  try {
    var formHistory = Cc["@mozilla.org/satchel/form-history;1"].
                      getService(Ci.nsIFormHistory2);
    formHistory.removeAllEntries();
  } catch (ex) {}
}




var testSaveFormInformation = function()
{
  
  controller.open(sampleFormSite);
  controller.waitForPageLoad();

  var firstName = new elementslib.Name(controller.tabs.activeTab, "ship_fname");
  var lastName = new elementslib.Name(controller.tabs.activeTab, "ship_lname");

  controller.type(firstName, fname);
  controller.type(lastName, lname);

  controller.click(new elementslib.Name(controller.tabs.activeTab, "SubmitButton"));
  controller.waitForPageLoad();
  controller.waitForElement(firstName, gTimeout);

  
  var popDownAutoCompList = new elementslib.ID(controller.window.document, "PopupAutoComplete");

  controller.type(firstName, fname.substring(0,2));
  controller.sleep(500);

  controller.assertProperty(popDownAutoCompList, "popupOpen", true);
  controller.keypress(firstName, "VK_DOWN", {});
  controller.click(popDownAutoCompList);
  controller.assertValue(firstName, fname);

  controller.type(lastName, lname.substring(0,2));
  controller.sleep(500);
  controller.assertProperty(popDownAutoCompList, "popupOpen", true);
  controller.keypress(lastName, "VK_DOWN", {});
  controller.click(popDownAutoCompList);
  controller.assertValue(lastName, lname);
}




var testClearFormHistory = function()
{
  var firstName = new elementslib.Name(controller.tabs.activeTab, "ship_fname");
  var lastName = new elementslib.Name(controller.tabs.activeTab, "ship_lname");

  
  var md = new ModalDialogAPI.modalDialog(clearHistoryHandler);
  md.start();

  controller.click(new elementslib.Elem(controller.menus["tools-menu"].sanitizeItem));

  
  var popDownAutoCompList = new elementslib.ID(controller.window.document, "PopupAutoComplete");

  controller.open(sampleFormSite);
  controller.waitForPageLoad();
  controller.waitForElement(firstName, gTimeout);

  controller.type(firstName,fname.substring(0,2));
  controller.sleep(500);
  controller.assertProperty(popDownAutoCompList, "popupOpen", false);

  controller.type(lastName, lname.substring(0,2));
  controller.sleep(500);
  controller.assertProperty(popDownAutoCompList, "popupOpen", false);
}




var clearHistoryHandler = function(controller)
{
  
  var checkBox = new elementslib.XPath(controller.window.document, "/*[name()='prefwindow']/*[name()='prefpane'][1]/*[name()='listbox'][1]/*[name()='listitem'][2]");
  controller.waitForElement(checkBox, gTimeout);
  controller.assertChecked(checkBox);

  var clearButton = new elementslib.Lookup(controller.window.document, '/id("SanitizeDialog")/anon({"anonid":"dlg-buttons"})/{"dlgtype":"accept"}');
  controller.waitThenClick(clearButton);
}






