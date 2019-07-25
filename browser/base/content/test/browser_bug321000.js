







































const kTestString = "  hello hello  \n  world\nworld  ";

var gTests = [

  { desc: "Urlbar strips newlines and surrounding whitespace",
    element: gURLBar,
    expected: kTestString.replace(/\s*\n\s*/g,'')
  },

  { desc: "Searchbar replaces newlines with spaces",
    element: document.getElementById('searchbar'),
    expected: kTestString.replace('\n',' ','g')
  },

];




function test() {
  waitForExplicitFinish();

  let cbHelper = Cc["@mozilla.org/widget/clipboardhelper;1"].
                 getService(Ci.nsIClipboardHelper);

  
  
  
  waitForClipboard(kTestString, function() { cbHelper.copyString(kTestString); },
                   next_test, finish);
}

function next_test() {
  if (gTests.length)
    test_paste(gTests.shift());
  else
    finish();
}

function test_paste(aCurrentTest) {
  var element = aCurrentTest.element;

  
  var inputListener = {
    test: aCurrentTest,
    handleEvent: function(event) {
      element.removeEventListener(event.type, this, false);

      is(element.value, this.test.expected, this.test.desc);

      
      element.value = "";
      setTimeout(next_test, 0);
    }
  }
  element.addEventListener("input", inputListener, false);

  
  window.focus();
  gBrowser.selectedBrowser.focus();

  
  info("About to focus " + element.id);
  element.addEventListener("focus", function() {
    element.removeEventListener("focus", arguments.callee, false);
    executeSoon(function() {
      
      
      info("Pasting into " + element.id);
      EventUtils.synthesizeKey("v", { accelKey: true });
    });
  }, false);
  element.focus();
}
