function test()
{
  const kPrefName_AutoScroll = "general.autoScroll";
  Services.prefs.setBoolPref(kPrefName_AutoScroll, true);

  gBrowser.selectedTab = gBrowser.addTab();

  const kNoKeyEvents   = 0;
  const kKeyDownEvent  = 1;
  const kKeyPressEvent = 2;
  const kKeyUpEvent    = 4;
  const kAllKeyEvents  = 7;

  var expectedKeyEvents;
  var dispatchedKeyEvents;
  var key;
  var root;

  


  function sendChar(aChar)
  {
    key = aChar;
    dispatchedKeyEvents = kNoKeyEvents;
    EventUtils.sendChar(key, gBrowser.contentWindow);
    is(dispatchedKeyEvents, expectedKeyEvents,
       "unexpected key events were dispatched or not dispatched: " + key);
  }

  


  function sendKey(aKey)
  {
    key = aKey;
    dispatchedKeyEvents = kNoKeyEvents;
    EventUtils.sendKey(key, gBrowser.contentWindow);
    is(dispatchedKeyEvents, expectedKeyEvents,
       "unexpected key events were dispatched or not dispatched: " + key);
  }

  function onKey(aEvent)
  {
    if (aEvent.target != root && aEvent.target != root.ownerDocument.body) {
      ok(false, "unknown target: " + aEvent.target.tagName);
      return;
    }

    var keyFlag;
    switch (aEvent.type) {
      case "keydown":
        keyFlag = kKeyDownEvent;
        break;
      case "keypress":
        keyFlag = kKeyPressEvent;
        break;
      case "keyup":
        keyFlag = kKeyUpEvent;
        break;
      default:
        ok(false, "Unknown events: " + aEvent.type);
        return;
    }
    dispatchedKeyEvents |= keyFlag;
    is(keyFlag, expectedKeyEvents & keyFlag, aEvent.type + " fired: " + key);
  }

  waitForExplicitFinish();
  gBrowser.selectedBrowser.addEventListener("pageshow", onLoad, false);
  var dataUri = 'data:text/html,<body style="height:10000px;"></body>';
  gBrowser.loadURI(dataUri);

  function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("pageshow", onLoad, false);
    waitForFocus(onFocus, content);
  }

  function onFocus() {
    var doc = gBrowser.contentDocument;

    root = doc.documentElement;
    root.addEventListener("keydown", onKey, true);
    root.addEventListener("keypress", onKey, true);
    root.addEventListener("keyup", onKey, true);

    
    expectedKeyEvents = kAllKeyEvents;
    sendChar("A");

    
    EventUtils.synthesizeMouse(root, 10, 10, { button: 1 },
                               gBrowser.contentWindow);

    
    
    executeSoon(continueTest);
  }

  function continueTest() {
    
    expectedKeyEvents = kNoKeyEvents;
    sendChar("A");
    sendKey("DOWN");
    sendKey("RETURN");
    sendKey("RETURN");
    sendKey("HOME");
    sendKey("END");
    sendKey("TAB");
    sendKey("RETURN");

    
    
    
    expectedKeyEvents = kKeyUpEvent;
    sendKey("ESCAPE");

    
    expectedKeyEvents = kAllKeyEvents;
    sendChar("A");

    root.removeEventListener("keydown", onKey, true);
    root.removeEventListener("keypress", onKey, true);
    root.removeEventListener("keyup", onKey, true);

    
    if (Services.prefs.prefHasUserValue(kPrefName_AutoScroll))
      Services.prefs.clearUserPref(kPrefName_AutoScroll);

    
    gBrowser.removeCurrentTab();

    finish();
  }
}
