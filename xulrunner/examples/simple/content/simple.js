




































function onButtonClick() {
  var textbox = document.getElementById("textbox");

  var contractid = (textbox.value % 2 == 0) ?
      "@test.mozilla.org/simple-test;1?impl=js" :
      "@test.mozilla.org/simple-test;1?impl=c++";

  var test = Components.classes[contractid].
      createInstance(Components.interfaces.nsISimpleTest);

  textbox.value = test.add(textbox.value, 1);
}
