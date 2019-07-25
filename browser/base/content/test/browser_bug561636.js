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
  is(gInvalidFormPopup.firstChild.nodeValue,
     doc.getElementById('i').validationMessage.substring(0,256),
     "The panel should show the 256 first characters of the validationMessage");
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

      
      gBrowser.removeTab(gBrowser.selectedTab, {animate: false});

      
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

    
    gBrowser.removeTab(gBrowser.selectedTab, {animate: false});
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

    
    gBrowser.removeTab(gBrowser.selectedTab, {animate: false});
    executeSoon(test4);
  }, false);

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    gBrowser.contentDocument.getElementById('s').click();
  }, true);

  gBrowser.selectedTab = tab;
  gBrowser.selectedTab.linkedBrowser.loadURI(uri);
}




function test4()
{
  let uri = "data:text/html,<iframe name='t'></iframe><form target='t' action='data:text/html,'><input id='i'><input id='s' type='submit'></form>";
  let tab = gBrowser.addTab();

  gInvalidFormPopup.addEventListener("popupshown", function() {
    gInvalidFormPopup.removeEventListener("popupshown", arguments.callee, false);

    let doc = gBrowser.contentDocument;
    is(doc.activeElement, doc.getElementById('i'),
       "First invalid element should be focused");

    checkPopupShow();
    checkPopupMessage(doc);

    
    gBrowser.removeTab(gBrowser.selectedTab, {animate: false});
    executeSoon(test5);
  }, false);

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    let msg = "";
    for (let i=0; i<50; ++i) {
      msg += "abcde ";
    }
    
    gBrowser.contentDocument.getElementById('i').setCustomValidity(msg);
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

    EventUtils.synthesizeKey("a", {});

    executeSoon(function () {
      checkPopupHide();

      
      gBrowser.removeTab(gBrowser.selectedTab, {animate: false});
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

    doc.getElementById('i').blur();

    executeSoon(function () {
      checkPopupHide();

      
      gBrowser.removeTab(gBrowser.selectedTab, {animate: false});
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

    EventUtils.synthesizeKey("VK_TAB", {});

    executeSoon(function () {
      checkPopupHide();

      
      gBrowser.removeTab(gBrowser.selectedTab, {animate: false});
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

      
      gBrowser.removeTab(gBrowser.selectedTab, {animate: false});
      gBrowser.removeTab(gBrowser.selectedTab, {animate: false});
      executeSoon(test9);
    });
  }, false);

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    gBrowser.contentDocument.getElementById('s').click();
  }, true);

  gBrowser.selectedTab = tab;
  gBrowser.selectedTab.linkedBrowser.loadURI(uri);
}






function test9()
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
      gBrowser.removeTab(tab, {animate: false});

      
      executeSoon(finish);
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
