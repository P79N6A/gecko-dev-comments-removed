function test()
{
  waitForExplicitFinish();
  var frame = document.getElementById("customizeToolbarSheetIFrame");
  frame.addEventListener("load", testCustomizeFrameLoadedPre, true);

  document.getElementById("cmd_CustomizeToolbars").doCommand();
}

function testCustomizeFrameLoadedPre(){
  
  
  
  
  executeSoon(testCustomizeFrameLoaded);
}

function testCustomizeFrameLoaded()
{
  var panel = document.getElementById("customizeToolbarSheetPopup");
  panel.addEventListener("popuphidden", testCustomizePopupHidden, false);

  var frame = document.getElementById("customizeToolbarSheetIFrame");
  frame.removeEventListener("load", testCustomizeFrameLoadedPre, true);

  var menu = document.getElementById("bookmarksMenuPopup");
  ok("getResult" in menu, "menu has binding");

  var framedoc = document.getElementById("customizeToolbarSheetIFrame").contentDocument;
  var b = framedoc.getElementById("donebutton");

  b.focus();
  framedoc.getElementById("donebutton").doCommand();
}
  
function testCustomizePopupHidden()
{
  var panel = document.getElementById("customizeToolbarSheetPopup");
  panel.removeEventListener("popuphidden", testCustomizePopupHidden, false);
  is(document.activeElement, document.documentElement, "focus after customize done");

  finish();
}
