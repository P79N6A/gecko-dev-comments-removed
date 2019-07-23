







































const Ci = Components.interfaces;
const Cc = Components.classes;

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

  
  info("About to put a string in clipboard");
  Cc["@mozilla.org/widget/clipboardhelper;1"].getService(Ci.nsIClipboardHelper)
                                             .copyString(kTestString);

  
  
  setTimeout(poll_clipboard, 100);
}

var runCount = 0;
function poll_clipboard() {
  
  if (++runCount > 50) {
    
    ok(false, "Timed out while polling clipboard for pasted data");
    
    finish_test();
    return;
  }

  info("Polling clipboard cycle " + runCount);
  var clip = Cc["@mozilla.org/widget/clipboard;1"].
             getService(Ci.nsIClipboard);
  var trans = Cc["@mozilla.org/widget/transferable;1"].
              createInstance(Ci.nsITransferable);
  trans.addDataFlavor("text/unicode");
  var str = new Object();
  try {
    
    clip.getData(trans, clip.kGlobalClipboard);
    trans.getTransferData("text/unicode", str, {});
    str = str.value.QueryInterface(Ci.nsISupportsString);
  }
  catch(ex) {}

  if (kTestString == str) {
    next_test();
  }
  else
    setTimeout(poll_clipboard, 100);
}

function next_test() {
  if (gTests.length) {
    var currentTest = gTests.shift();
    test_paste(currentTest);
  }
  else {
    
    
    
    Cc["@mozilla.org/widget/clipboardhelper;1"].getService(Ci.nsIClipboardHelper)
                                               .copyString("");
    finish();
  }
}

function test_paste(aCurrentTest) {
  var element = aCurrentTest.element;

  
  var inputListener = {
    test: aCurrentTest,
    handleEvent: function(event) {
      var element = event.target;
      element.removeEventListener("input", this, false);

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
