

var gTests = [];
var gCurrentTest = null;

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
  desc: "Verify full toggle on Preferences",

  run: function(){
    
    document.getElementById("tool-panel-open").click();
    is(document.getElementById("panel-container").hidden, false, "Preferences should be visible");

    var contentRegion = document.getElementById("prefs-content");

    
    var imageRegion = document.getAnonymousElementByAttribute(contentRegion, "pref", "permissions.default.image");
    var imageValue  = imageRegion.value;
    var imageTitle  = document.getAnonymousElementByAttribute(imageRegion, "class", "preferences-title");
    var imageButton = document.getAnonymousElementByAttribute(imageRegion, "anonid", "input");
    imageButton.click();
    is(imageRegion.value, !imageValue, "Tapping on input control should change the value");
    imageTitle.click();
    is(imageRegion.value, imageValue, "Tapping on the title should change the value"); 
    imageRegion.click();
    is(imageRegion.value, !imageValue, "Tapping on the setting should change the value"); 

    BrowserUI.hidePanel();
    is(document.getElementById("panel-container").hidden, true, "Preferences panel should be closed");
    runNextTest();
  }
});

