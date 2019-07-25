


var gTests = [];
var gCurrentTest = null;
var initialDragOffset = null;
var finalDragOffset = null;
let x = {};
let y = {};


function dragElement(element,x1,y1,x2,y2)
{
  EventUtils.synthesizeMouse(element, x1, y1, { type: "mousedown" });
  EventUtils.synthesizeMouse(element, x2, y2, { type: "mousemove" });
  EventUtils.synthesizeMouse(element, x2, y2, { type: "mouseup" });
}

function doubleClick(element,x,y)
{
  EventUtils.synthesizeMouse(element, x, y, {});
  EventUtils.synthesizeMouse(element, x, y, {});
}

function test() {
  
  
  waitForExplicitFinish();
  
  
  runNextTest();
}


function runNextTest() {
  
  if (gTests.length > 0) {
    gCurrentTest = gTests.shift();
    info(gCurrentTest.desc);
    gCurrentTest.run();
  }
  else {
    
    finish();
  }
}

gTests.push({
  desc: "Test basic panning of Preferences",
  _currenttab : null,
  _contentScrollbox : document.getElementById("controls-scrollbox")
    .boxObject.QueryInterface(Components.interfaces.nsIScrollBoxObject),
  _prefsScrollbox : document.getAnonymousElementByAttribute(document.getElementById("prefs-list"),
    "anonid", "main-box").boxObject.QueryInterface(Components.interfaces.nsIScrollBoxObject),

  run: function(){
    this._currenttab = Browser.addTab("about:blank",true);
    function handleEvent() {
      gCurrentTest._currenttab.browser.removeEventListener("load", handleEvent, true);
      gCurrentTest.onPageLoad();
    };
    this._currenttab.browser.addEventListener("load", handleEvent , true);
  },
 
  onPageLoad: function(){
    
    let controls = document.getElementById("controls-scrollbox");

    
		initialDragOffset = document.getElementById("tabs-container").getBoundingClientRect().width;
		finalDragOffset = initialDragOffset + document.getElementById("browser-controls")
      .getBoundingClientRect().width;

    gCurrentTest._contentScrollbox.getPosition(x,y);
    ok((x.value==initialDragOffset & y.value==0),"The right sidebar must be invisible",
      "Got "+x.value+" "+y.value+", expected " + initialDragOffset + ",0");

    
    let w = controls.clientWidth;
    let h = controls.clientHeight;
    dragElement(controls,w/2, h/2, w/4, h/2);

    
    gCurrentTest._contentScrollbox.getPosition(x,y);
    ok((x.value==finalDragOffset & y.value==0),"The right sidebar must be visible",
      "Got "+x.value+" "+y.value+", expected "+ finalDragOffset +",0");
  
    
    var prefsOpen = document.getElementById("tool-panel-open");
    is(prefsOpen.hidden, false, "Preferences open button must be visible");
    is(prefsOpen.checked, false, "Preferences open button must not be depressed");

    
    is(document.getElementById("panel-container").hidden,true, "Preferences panel is invisble");

    
    var prefsClick = document.getElementById("tool-panel-open");
    prefsClick.click();
    waitFor(gCurrentTest.onPrefsView, function() { return document.getElementById("panel-container").hidden == false; });
  },

  onPrefsView: function(){
    let prefsList = document.getElementById("prefs-list");
    let w = prefsList.clientWidth;
    let h = prefsList.clientHeight;

    
    var prefsContainer = document.getElementById("panel-container");
    is(prefsContainer.hidden, false, "Preferences panel must now be visble");

    
    is(document.getElementById("panel-container").hidden, false, "Preferences panel should be visible");

    
    is(document.getElementById("tool-addons").hidden, false, "Addons button must be visible");
    is(document.getElementById("tool-addons").checked, false, "Addons button must not be pressed");

    is(document.getElementById("tool-downloads").hidden, false, "Downloads button must be visible");
    is(document.getElementById("tool-downloads").checked, false, "Downloads button must not be pressed");

    is(document.getElementById("tool-preferences").hidden, false, "Preferences button must be visible");
    is(document.getElementById("tool-preferences").checked, true, "Preferences button must be pressed");

    
    is(document.getElementById("tool-panel-close").hidden, false, "Panel close button must be visible");
    is(document.getElementById("tool-panel-close").checked, false, "Panel close button must not be pressed");
    
    
    
    gCurrentTest._prefsScrollbox.getPosition(x,y);
    ok((x.value==0 & y.value==0),"The preferences pane should be visble","Got "+x.value+" "+y.value+", expected 0,0");

    
    dragElement(prefsList,w/2,h/2,w/2,h/4);

    
    gCurrentTest._prefsScrollbox.getPosition(x,y);
    ok((x.value==0 & y.value==104),"Preferences pane is panned up","Got "+x.value+" "+y.value+", expected 0,104");

    
    dragElement(prefsList,w/2,h/4,w/2,h/2);

    
    gCurrentTest._prefsScrollbox.getPosition(x,y);
    ok((x.value==0 & y.value==0),"Preferences pane is panned down","Got "+x.value+" "+y.value+", expected 0,0");

    
    
    dragElement(prefsList,w/2,h/2,w/4,h/2);

    gCurrentTest._prefsScrollbox.getPosition(x,y);
    ok((x.value==0 & y.value==0),"Preferences pane is not panned left","Got "+x.value+" "+y.value+", expected 0,0");

    
    dragElement(prefsList,w/4,h/2,w/2,h/2);

    gCurrentTest._prefsScrollbox.getPosition(x,y);
    ok((x.value==0 & y.value==0),"Preferences pane is not panned right","Got "+x.value+" "+y.value+", expected 0,0");

    
    var prefClose = document.getElementById("tool-panel-close");
    prefClose.click();
    waitFor(gCurrentTest.finish, function () { return document.getElementById("panel-container").hidden == true; });
  },

  finish: function(){
    
    is(document.getElementById("panel-container").hidden, true, "Preference pane is now invisible");

    
    gCurrentTest._contentScrollbox.getPosition(x,y);
    ok((x.value==finalDragOffset & y.value==0),"The right sidebar is still visible",
      "Got "+x.value+" "+y.value+", expected "+ finalDragOffset +",0");

    
    var prefsOpen = document.getElementById("tool-panel-open");
    is(prefsOpen.checked, false, "Preferences open button must not be depressed");

    Browser.closeTab(this._currenttab);
    runNextTest();    
  }
});
