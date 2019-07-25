





































const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  controller = mozmill.getBrowserController();

  containerString = '/id("main-window")/id("browser-bottombox")/id("FindToolbar")' +
                    '/anon({"anonid":"findbar-container"})';
  findBar = new elementslib.Lookup(controller.window.document, containerString);
  findBarTextField = new elementslib.Lookup(controller.window.document,
                                            containerString + '/anon({"anonid":"findbar-textbox"})');
  findBarNextButton = new elementslib.Lookup(controller.window.document,
                                             containerString + '/anon({"anonid":"find-next"})');
  findBarPrevButton = new elementslib.Lookup(controller.window.document,
                                             containerString + '/anon({"anonid":"find-previous"})');
  findBarCloseButton = new elementslib.Lookup(controller.window.document,
                                              containerString + '/anon({"anonid":"find-closebutton"})');
}

var teardownModule = function(module) {
  try {
     
    controller.keypress(null, "f", {accelKey:true});

    
    controller.keypress(findBarTextField, 'VK_DELETE', {});

    
    controller.click(findBarCloseButton);
  } catch(e) {
  }
}





var testFindInPage = function() {
  var searchTerm = "mozilla";
  var comparator = Ci.nsIDOMRange.START_TO_START;
  var tabContent = controller.tabs.activeTabWindow;

  
  controller.open("http://www.mozilla.org");
  controller.waitForPageLoad();

  
  controller.keypress(null, "f", {accelKey:true});
  controller.sleep(gDelay);

  
  controller.waitForElement(findBar, gTimeout);

  
  controller.type(findBarTextField, searchTerm);
  controller.keypress(null, "VK_RETURN", {});
  controller.sleep(gDelay);

  
  
  var selectedText = tabContent.getSelection();
  controller.assertJS("subject.selectedText == subject.searchTerm",
                      {selectedText: selectedText.toString().toLowerCase(), searchTerm: searchTerm});

  
  var range = selectedText.getRangeAt(0);

  
  controller.click(findBarNextButton);
  controller.sleep(gDelay);

  selectedText = tabContent.getSelection();
  controller.assertJS("subject.selectedText == subject.searchTerm",
                      {selectedText: selectedText.toString().toLowerCase(), searchTerm: searchTerm});

  
  controller.assertJS("subject.isNextResult == true",
                      {isNextResult: selectedText.getRangeAt(0).compareBoundaryPoints(comparator, range) != 0});

  
  controller.click(findBarPrevButton);
  controller.sleep(gDelay);

  selectedText = tabContent.getSelection();
  controller.assertJS("subject.selectedText == subject.searchTerm",
                      {selectedText: selectedText.toString().toLowerCase(), searchTerm: searchTerm});

  
  controller.assertJS("subject.isFirstResult == true",
                      {isFirstResult: selectedText.getRangeAt(0).compareBoundaryPoints(comparator, range) == 0});
}





