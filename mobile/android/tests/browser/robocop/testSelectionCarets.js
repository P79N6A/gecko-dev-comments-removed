



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");
Cu.import('resource://gre/modules/Geometry.jsm');

const SELECTION_CARETS_PREF = "selectioncaret.enabled";
const TOUCH_CARET_PREF = "touchcaret.enabled";
const TEST_URL = "http://mochi.test:8888/tests/robocop/testSelectionCarets.html";



const LONGPRESS_EVENT = "testSelectionCarets:Longpress";
const STATUS_UPDATE_EVENT = "ActionBar:UpdateState";


const TAB_CHANGE_EVENT = "testSelectionCarets:TabChange";
const TAB_STOP_EVENT = "STOP";

const gChromeWin = Services.wm.getMostRecentWindow("navigator:browser");









function do_promiseTabChangeEvent(tabId, eventType) {
  return new Promise(resolve => {
    let observer = (subject, topic, data) => {
      let message = JSON.parse(data);

      if (message.event === eventType && message.tabId === tabId) {
        Services.obs.removeObserver(observer, TAB_CHANGE_EVENT);
        resolve(data);
      }
    }

    Services.obs.addObserver(observer, TAB_CHANGE_EVENT, false);
  });
}





function isInputOrTextarea(element) {
  return ((element instanceof Ci.nsIDOMHTMLInputElement) ||
          (element instanceof Ci.nsIDOMHTMLTextAreaElement));
}




function elementSelection(element) {
  return (isInputOrTextarea(element)) ?
    element.editor.selection :
    element.ownerDocument.defaultView.getSelection();
}




function selectElementFirstChar(doc, element) {
  if (isInputOrTextarea(element)) {
    element.setSelectionRange(0, 1);
    return;
  }

  
  let range = doc.createRange();
  range.setStart(element.firstChild, 0);
  range.setEnd(element.firstChild, 1);

  let selection = elementSelection(element);
  selection.removeAllRanges();
  selection.addRange(range);
}







function getFirstCharPressPoint(doc, element, expected) {
  
  selectElementFirstChar(doc, element);

  
  let selection = elementSelection(element);
  is(selection.toString(), expected, "Selected char should match expected char.");

  
  let rect = selection.getRangeAt(0).getBoundingClientRect();
  let r = new Point(rect.left + (rect.width / 2), rect.bottom - (rect.height / 3));

  return r;
}










function do_promiseLongPressResult(midPoint) {
  return new Promise(resolve => {
    let observer = (subject, topic, data) => {
      let ActionBarHandler = gChromeWin.ActionBarHandler;
      if (topic === STATUS_UPDATE_EVENT) {
        let text = ActionBarHandler._getSelectedText();
        if (text !== "") {
          
          Services.obs.removeObserver(observer, STATUS_UPDATE_EVENT);
          resolve({
            focusedElement: ActionBarHandler._targetElement,
            text: text,
          });
        }
      }
    };

    
    Services.obs.addObserver(observer, STATUS_UPDATE_EVENT, false);
    Messaging.sendRequestForResult({
      type: LONGPRESS_EVENT,
      x: midPoint.x,
      y: midPoint.y,
    });
  });
}




add_task(function* testSelectionCarets() {
  
  
  
  let BrowserApp = gChromeWin.BrowserApp;
  yield do_promiseTabChangeEvent(BrowserApp.selectedTab.id, TAB_STOP_EVENT);

  
  Services.prefs.setBoolPref(SELECTION_CARETS_PREF, true);
  Services.prefs.setBoolPref(TOUCH_CARET_PREF, true);

  
  let browser = BrowserApp.addTab(TEST_URL).browser;
  let tab = BrowserApp.getTabForBrowser(browser);
  yield do_promiseTabChangeEvent(tab.id, TAB_STOP_EVENT);

  do_register_cleanup(function cleanup() {
    Services.prefs.clearUserPref(SELECTION_CARETS_PREF);
    Services.prefs.clearUserPref(TOUCH_CARET_PREF);
    BrowserApp.closeTab(tab);
  });

  
  let doc = browser.contentDocument;
  let ce_LTR_elem = doc.getElementById("LTRcontenteditable");
  let tc_LTR_elem = doc.getElementById("LTRtextContent");
  let i_LTR_elem = doc.getElementById("LTRinput");
  let ta_LTR_elem = doc.getElementById("LTRtextarea");

  let ce_RTL_elem = doc.getElementById("RTLcontenteditable");
  let tc_RTL_elem = doc.getElementById("RTLtextContent");
  let i_RTL_elem = doc.getElementById("RTLinput");
  let ta_RTL_elem = doc.getElementById("RTLtextarea");

  
  let ce_LTR_midPoint = getFirstCharPressPoint(doc, ce_LTR_elem, "F");
  let tc_LTR_midPoint = getFirstCharPressPoint(doc, tc_LTR_elem, "O");
  let i_LTR_midPoint = getFirstCharPressPoint(doc, i_LTR_elem, "T");
  let ta_LTR_midPoint = getFirstCharPressPoint(doc, ta_LTR_elem, "W");

  let ce_RTL_midPoint = getFirstCharPressPoint(doc, ce_RTL_elem, "א");
  let tc_RTL_midPoint = getFirstCharPressPoint(doc, tc_RTL_elem, "ת");
  let i_RTL_midPoint = getFirstCharPressPoint(doc, i_RTL_elem, "ל");
  let ta_RTL_midPoint = getFirstCharPressPoint(doc, ta_RTL_elem, "ה");


  
  
  let result = yield do_promiseLongPressResult(ce_LTR_midPoint);
  is(result.focusedElement, ce_LTR_elem, "Focused element should match expected.");
  is(result.text, "Find", "Selected text should match expected text.");

  result = yield do_promiseLongPressResult(tc_LTR_midPoint);
  is(result.focusedElement, null, "No focused element is expected.");
  is(result.text, "Open", "Selected text should match expected text.");

  result = yield do_promiseLongPressResult(i_LTR_midPoint);
  is(result.focusedElement, i_LTR_elem, "Focused element should match expected.");
  is(result.text, "Type", "Selected text should match expected text.");

  result = yield do_promiseLongPressResult(ta_LTR_midPoint);
  is(result.focusedElement, ta_LTR_elem, "Focused element should match expected.");
  is(result.text, "Words", "Selected text should match expected text.");

  
  
  result = yield do_promiseLongPressResult(ce_RTL_midPoint);
  is(result.focusedElement, ce_RTL_elem, "Focused element should match expected.");
  is(result.text, "איפה", "Selected text should match expected text.");

  result = yield do_promiseLongPressResult(tc_RTL_midPoint);
  is(result.focusedElement, null, "No focused element is expected.");
  is(result.text, "תן", "Selected text should match expected text.");

  result = yield do_promiseLongPressResult(i_RTL_midPoint);
  is(result.focusedElement, i_RTL_elem, "Focused element should match expected.");
  is(result.text, "לרוץ", "Selected text should match expected text.");

  result = yield do_promiseLongPressResult(ta_RTL_midPoint);
  is(result.focusedElement, ta_RTL_elem, "Focused element should match expected.");
  is(result.text, "הספר", "Selected text should match expected text.");

  ok(true, "Finished all tests.");
});

run_next_test();
