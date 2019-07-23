const Ci = Components.interfaces;
const Cc = Components.classes;

const kUrlBarElm = document.getElementById('urlbar');
const kSearchBarElm = document.getElementById('searchbar');
const kTestString = "  hello hello  \n  world\nworld  ";

function testPaste(name, element, expected) {
  element.focus();
  listener.expected = expected;
  listener.name = name;
  EventUtils.synthesizeKey("v", { accelKey: true });
}

var listener = {
  expected: "",
  name: "",
  handleEvent: function(event) {
    var element = event.target;
    is(element.value, this.expected, this.name);
    switch (element) {
      case kUrlBarElm:
        continue_test();
      case kSearchBarElm:
        finish_test();
    }
  }
}




function test() {
  waitForExplicitFinish();

  
  kUrlBarElm.addEventListener("input", listener, true);
  kSearchBarElm.addEventListener("input", listener, true);

  
  Components.classes["@mozilla.org/widget/clipboardhelper;1"]
            .getService(Components.interfaces.nsIClipboardHelper)
            .copyString(kTestString);
  testPaste('urlbar strips newlines and surrounding whitespace', 
            kUrlBarElm,
            kTestString.replace(/\s*\n\s*/g,''));
}

function continue_test() {
  testPaste('searchbar replaces newlines with spaces', 
            kSearchBarElm,
            kTestString.replace('\n',' ','g'));
}

function finish_test() {
  kUrlBarElm.removeEventListener("input", listener, true);
  kSearchBarElm.removeEventListener("input", listener, true);
  
  kUrlBarElm.value="";
  kSearchBarElm.value="";
  finish();
}
