let testURL = "chrome://mochikit/content/browser/mobile/chrome/browser_FormAssistant.html";
let newTab = null;
let container = null;

function test() {
  
  waitForExplicitFinish();

  
  newTab = Browser.addTab(testURL, true);
  BrowserUI.closeAutoComplete(true);

  
  waitFor(onTabLoaded, function() { return newTab._loading == false;});
}

function onTabLoaded() {
  container = document.getElementById("form-helper-container");
  testMouseEvents();
}

function testMouseEvents() {
  
  
  
  EventUtils.sendMouseEvent({type: "click"}, "root", newTab.browser.contentWindow);
  is(FormHelper._open, false, "Form Assistant should stay closed");

  let element = newTab.browser.contentDocument.querySelector("*[tabindex='0']");
  EventUtils.synthesizeMouseForContent(element, 1, 1, {}, window);
  
  
  setTimeout (function() {
    ok(FormHelper._open, "Form Assistant should be open");
    testShowUIForElements();
  }, 1700);
};

function testShowUIForElements() {
  let doc = newTab.browser.contentDocument;

  ok(FormHelper.canShowUIFor(doc.querySelector("*[tabindex='1']")), "canShowUI for input type='text'");
  ok(FormHelper.canShowUIFor(doc.querySelector("*[tabindex='2']")), "canShowUI for input type='password'");
  is(FormHelper.canShowUIFor(doc.querySelector("*[tabindex='3']")), false, "!canShowUI for input type='submit'");
  is(FormHelper.canShowUIFor(doc.querySelector("*[tabindex='4']")), false, "!canShowUI for input type='file'");
  is(FormHelper.canShowUIFor(doc.querySelector("*[tabindex='5']")), false, "!canShowUI for input button type='submit'");
  is(FormHelper.canShowUIFor(doc.querySelector("*[tabindex='6']")), false, "!canShowUI for div@role='button'");
  is(FormHelper.canShowUIFor(doc.querySelector("*[tabindex='7']")), false, "!canShowUI for input type='image'");

  testTabIndexNavigation();
};

function testTabIndexNavigation() {
  
  let firstElement = newTab.browser.contentDocument.getElementById("root");
  FormHelper.open(firstElement);
  is(container.hidden, false, "Form Assistant should be open");

  FormHelper.goToPrevious();
  let element = newTab.browser.contentDocument.querySelector("*[tabindex='0']");
  isnot(FormHelper.getCurrentElement(), element, "Focus should not have changed");

  FormHelper.goToNext();
  element = newTab.browser.contentDocument.querySelector("*[tabindex='2']");
  is(FormHelper.getCurrentElement(), element, "Focus should be on element with tab-index : 2");

  FormHelper.goToPrevious();
  element = newTab.browser.contentDocument.querySelector("*[tabindex='1']");
  is(FormHelper.getCurrentElement(), element, "Focus should be on element with tab-index : 1");

  FormHelper.goToNext();
  FormHelper.goToNext();
  FormHelper.goToNext();
  FormHelper.goToNext();
  FormHelper.goToNext();
  FormHelper.goToNext();

  element = newTab.browser.contentDocument.querySelector("*[tabindex='7']");
  is(FormHelper.getCurrentElement(), element, "Focus should be on element with tab-index : 7");

  FormHelper.goToNext();
  element = newTab.browser.contentDocument.querySelector("*[tabindex='0']");
  is(FormHelper.getCurrentElement(), element, "Focus should be on element with tab-index : 0");

  FormHelper.goToNext();
  element = newTab.browser.contentDocument.getElementById("next");
  is(FormHelper.getCurrentElement(), element, "Focus should be on element with #id: next");

  FormHelper.goToNext();
  FormHelper.goToNext();
  FormHelper.goToNext();
  FormHelper.goToNext();

  element = newTab.browser.contentDocument.getElementById("last");
  is(FormHelper.getCurrentElement(), element, "Focus should be on element with #id: last");

  FormHelper.goToNext();
  is(FormHelper.getCurrentElement(), element, "Focus should be on element with #id: last");

  FormHelper.close();
  is(container.hidden, true, "Form Assistant should be close");

  
  Browser.closeTab(newTab);

  
  finish();
};

