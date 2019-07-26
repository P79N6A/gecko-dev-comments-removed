



function test() {
  gURLBar.focus();
  gURLBar.inputField.value = "https://example.com/";
  gURLBar.selectionStart = 4;
  gURLBar.selectionEnd = 5;
  goDoCommand("cmd_cut");
  is(gURLBar.inputField.value, "http://example.com/", "location bar value after cutting 's' from https");
  gURLBar.handleRevert();
}
