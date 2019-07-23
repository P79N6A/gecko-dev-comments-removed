const Ci = Components.interfaces;
const Cc = Components.classes;

function testPaste(name, element, expected) {
  element.focus();
  EventUtils.synthesizeKey("v", { accelKey: true });
  is(element.value, expected, name);
}




function test() {
  var testString = "  hello hello  \n  world\nworld  ";
  
  Components.classes["@mozilla.org/widget/clipboardhelper;1"]
            .getService(Components.interfaces.nsIClipboardHelper)
            .copyString(testString);
  testPaste('urlbar strips newlines and surrounding whitespace', 
            document.getElementById('urlbar'),
            testString.replace(/\s*\n\s*/g,''));
  testPaste('searchbar replaces newlines with spaces', 
            document.getElementById('searchbar'),
            testString.replace('\n',' ','g'));
}
