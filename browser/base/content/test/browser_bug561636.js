var gInvalidFormPopup = document.getElementById('invalid-form-popup');
ok(gInvalidFormPopup,
   "The browser should have a popup to show when a form is invalid");

function checkPopupShow()
{
  ok(gInvalidFormPopup.state == 'showing' || gInvalidFormPopup.state == 'open',
     "The invalid form popup should be shown");
}

function checkPopupHide()
{
  ok(gInvalidFormPopup.state != 'showing' && gInvalidFormPopup.state != 'open',
     "The invalid form popup should not be shown");
}

function checkPopupMessage(doc)
{
  is(gInvalidFormPopup.firstChild.textContent,
     doc.getElementById('i').validationMessage,
     "The panel should show the message from validationMessage");
}

let gObserver = {
  QueryInterface : XPCOMUtils.generateQI([Ci.nsIFormSubmitObserver]),

  notifyInvalidSubmit : function (aFormElement, aInvalidElements)
  {
  }
};

function test()
{
  waitForExplicitFinish();

  test1();
}




function test1() {
  let uri = "data:text/html,<html><body><iframe name='t'></iframe><form target='t' action='data:text/html,'><input><input id='s' type='submit'></form></body></html>";
  let tab = gBrowser.addTab();

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);
    let doc = gBrowser.contentDocument;

    doc.getElementById('s').click();

    executeSoon(function() {
      checkPopupHide();

      
      gBrowser.removeTab(gBrowser.selectedTab);

      
      executeSoon(test2);
    });
  }, true);

  gBrowser.selectedTab = tab;
  gBrowser.selectedTab.linkedBrowser.loadURI(uri);
}





function test2()
{
  let uri = "data:text/html,<iframe name='t'></iframe><form target='t' action='data:text/html,'><input required id='i'><input id='s' type='submit'></form>";
  let tab = gBrowser.addTab();

  gInvalidFormPopup.addEventListener("popupshown", function() {
    gInvalidFormPopup.removeEventListener("popupshown", arguments.callee, false);

    let doc = gBrowser.contentDocument;
    is(doc.activeElement, doc.getElementById('i'),
       "First invalid element should be focused");

    checkPopupShow();
    checkPopupMessage(doc);

    
    gBrowser.removeTab(gBrowser.selectedTab);
    executeSoon(test3);
  }, false);

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    gBrowser.contentDocument.getElementById('s').click();
  }, true);

  gBrowser.selectedTab = tab;
  gBrowser.selectedTab.linkedBrowser.loadURI(uri);
}





function test3()
{
  let uri = "data:text/html,<iframe name='t'></iframe><form target='t' action='data:text/html,'><input><input id='i' required><input required><input id='s' type='submit'></form>";
  let tab = gBrowser.addTab();

  gInvalidFormPopup.addEventListener("popupshown", function() {
    gInvalidFormPopup.removeEventListener("popupshown", arguments.callee, false);

    let doc = gBrowser.contentDocument;
    is(doc.activeElement, doc.getElementById('i'),
       "First invalid element should be focused");

    checkPopupShow();
    checkPopupMessage(doc);

    
    gBrowser.removeTab(gBrowser.selectedTab);
    executeSoon(test4a);
  }, false);

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    gBrowser.contentDocument.getElementById('s').click();
  }, true);

  gBrowser.selectedTab = tab;
  gBrowser.selectedTab.linkedBrowser.loadURI(uri);
}





function test4a()
{
  let uri = "data:text/html,<iframe name='t'></iframe><form target='t' action='data:text/html,'><input id='i' required><input id='s' type='submit'></form>";
  let tab = gBrowser.addTab();

  gInvalidFormPopup.addEventListener("popupshown", function() {
    gInvalidFormPopup.removeEventListener("popupshown", arguments.callee, false);

    let doc = gBrowser.contentDocument;
    is(doc.activeElement, doc.getElementById('i'),
       "First invalid element should be focused");

    checkPopupShow();
    checkPopupMessage(doc);

    EventUtils.synthesizeKey("a", {});

    executeSoon(function () {
      checkPopupHide();

      
      gBrowser.removeTab(gBrowser.selectedTab);
      executeSoon(test4b);
    });
  }, false);

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    gBrowser.contentDocument.getElementById('s').click();
  }, true);

  gBrowser.selectedTab = tab;
  gBrowser.selectedTab.linkedBrowser.loadURI(uri);
}





function test4b()
{
  let uri = "data:text/html,<iframe name='t'></iframe><form target='t' action='data:text/html,'><input type='email' id='i' required><input id='s' type='submit'></form>";
  let tab = gBrowser.addTab();

  gInvalidFormPopup.addEventListener("popupshown", function() {
    gInvalidFormPopup.removeEventListener("popupshown", arguments.callee, false);

    let doc = gBrowser.contentDocument;
    is(doc.activeElement, doc.getElementById('i'),
       "First invalid element should be focused");

    checkPopupShow();
    checkPopupMessage(doc);

    EventUtils.synthesizeKey("a", {});

    executeSoon(function () {
      checkPopupShow();

      
      gBrowser.removeTab(gBrowser.selectedTab);
      executeSoon(test5);
    });
  }, false);

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    gBrowser.contentDocument.getElementById('s').click();
  }, true);

  gBrowser.selectedTab = tab;
  gBrowser.selectedTab.linkedBrowser.loadURI(uri);
}





function test5()
{
  let uri = "data:text/html,<iframe name='t'></iframe><form target='t' action='data:text/html,'><input id='i' required><input id='s' type='submit'></form>";
  let tab = gBrowser.addTab();

  gInvalidFormPopup.addEventListener("popupshown", function() {
    gInvalidFormPopup.removeEventListener("popupshown", arguments.callee, false);

    let doc = gBrowser.contentDocument;
    is(doc.activeElement, doc.getElementById('i'),
       "First invalid element should be focused");

    checkPopupShow();
    checkPopupMessage(doc);

    doc.getElementById('i').blur();

    executeSoon(function () {
      checkPopupHide();

      
      gBrowser.removeTab(gBrowser.selectedTab);
      executeSoon(test6);
    });
  }, false);

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    gBrowser.contentDocument.getElementById('s').click();
  }, true);

  gBrowser.selectedTab = tab;
  gBrowser.selectedTab.linkedBrowser.loadURI(uri);
}




function test6()
{
  let uri = "data:text/html,<iframe name='t'></iframe><form target='t' action='data:text/html,'><input id='i' required><input id='s' type='submit'></form>";
  let tab = gBrowser.addTab();

  gInvalidFormPopup.addEventListener("popupshown", function() {
    gInvalidFormPopup.removeEventListener("popupshown", arguments.callee, false);

    let doc = gBrowser.contentDocument;
    is(doc.activeElement, doc.getElementById('i'),
       "First invalid element should be focused");

    checkPopupShow();
    checkPopupMessage(doc);

    EventUtils.synthesizeKey("VK_TAB", {});

    executeSoon(function () {
      checkPopupHide();

      
      gBrowser.removeTab(gBrowser.selectedTab);
      executeSoon(test7);
    });
  }, false);

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    gBrowser.contentDocument.getElementById('s').click();
  }, true);

  gBrowser.selectedTab = tab;
  gBrowser.selectedTab.linkedBrowser.loadURI(uri);
}




function test7()
{
  let uri = "data:text/html,<iframe name='t'></iframe><form target='t' action='data:text/html,'><input id='i' required><input id='s' type='submit'></form>";
  let tab = gBrowser.addTab();

  gInvalidFormPopup.addEventListener("popupshown", function() {
    gInvalidFormPopup.removeEventListener("popupshown", arguments.callee, false);

    let doc = gBrowser.contentDocument;
    is(doc.activeElement, doc.getElementById('i'),
       "First invalid element should be focused");

    checkPopupShow();
    checkPopupMessage(doc);

    
    gBrowser.selectedTab  = gBrowser.addTab("about:blank", {skipAnimation: true});

    executeSoon(function() {
      checkPopupHide();

      
      gBrowser.removeTab(gBrowser.selectedTab);
      gBrowser.removeTab(gBrowser.selectedTab);
      executeSoon(test8);
    });
  }, false);

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    gBrowser.contentDocument.getElementById('s').click();
  }, true);

  gBrowser.selectedTab = tab;
  gBrowser.selectedTab.linkedBrowser.loadURI(uri);
}






function test8()
{
  let uri = "data:text/html,<iframe name='t'></iframe><form target='t' action='data:text/html,'><input id='i' required><input id='s' type='submit'></form>";
  let tab = gBrowser.addTab();

  gObserver.notifyInvalidSubmit = function() {
    executeSoon(function() {
      let doc = tab.linkedBrowser.contentDocument;
      isnot(doc.activeElement, doc.getElementById('i'),
            "We should not focus the invalid element when the form is submitted in background");

      checkPopupHide();

      
      Services.obs.removeObserver(gObserver, "invalidformsubmit");
      gObserver.notifyInvalidSubmit = function () {};
      gBrowser.removeTab(tab);

      
      executeSoon(test9);
    });
  };

  Services.obs.addObserver(gObserver, "invalidformsubmit", false);

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    isnot(gBrowser.selectedTab, tab,
          "This tab should have been loaded in background");

    tab.linkedBrowser.contentDocument.getElementById('s').click();
  }, true);

  tab.linkedBrowser.loadURI(uri);
}




function test9()
{
  let uri = "data:text/html,<iframe name='t'></iframe><form target='t' action='data:text/html,'><input x-moz-errormessage='foo' required id='i'><input id='s' type='submit'></form>";
  let tab = gBrowser.addTab();

  gInvalidFormPopup.addEventListener("popupshown", function() {
    gInvalidFormPopup.removeEventListener("popupshown", arguments.callee, false);

    let doc = gBrowser.contentDocument;
    is(doc.activeElement, doc.getElementById('i'),
       "First invalid element should be focused");

    checkPopupShow();

    is(gInvalidFormPopup.firstChild.textContent, "foo",
       "The panel should show the author defined error message");

    
    gBrowser.removeTab(gBrowser.selectedTab);
    executeSoon(test10);
  }, false);

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    gBrowser.contentDocument.getElementById('s').click();
  }, true);

  gBrowser.selectedTab = tab;
  gBrowser.selectedTab.linkedBrowser.loadURI(uri);
}




function test10()
{
  let uri = "data:text/html,<iframe name='t'></iframe><form target='t' action='data:text/html,'><input type='email' required id='i'><input id='s' type='submit'></form>";
  let tab = gBrowser.addTab();

  gInvalidFormPopup.addEventListener("popupshown", function() {
    gInvalidFormPopup.removeEventListener("popupshown", arguments.callee, false);

    let doc = gBrowser.contentDocument;
    let input = doc.getElementById('i');
    is(doc.activeElement, input, "First invalid element should be focused");

    checkPopupShow();

    is(gInvalidFormPopup.firstChild.textContent, input.validationMessage,
       "The panel should show the current validation message");

    input.addEventListener('input', function() {
      input.removeEventListener('input', arguments.callee, false);

      executeSoon(function() {
        
	
        is(gInvalidFormPopup.firstChild.textContent, input.validationMessage,
           "The panel should show the current validation message");

        
        gBrowser.removeTab(gBrowser.selectedTab);
        executeSoon(finish);
      });
    }, false);

    EventUtils.synthesizeKey('f', {});
  }, false);

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    gBrowser.contentDocument.getElementById('s').click();
  }, true);

  gBrowser.selectedTab = tab;
  gBrowser.selectedTab.linkedBrowser.loadURI(uri);
}
