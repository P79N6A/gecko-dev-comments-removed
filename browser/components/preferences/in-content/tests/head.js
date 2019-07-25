


function is_hidden(aElement) {
  var style = aElement.ownerDocument.defaultView.getComputedStyle(aElement, "");
  if (style.display == "none")
    return true;
  if (style.visibility != "visible")
    return true;

  
  if (aElement.parentNode != aElement.ownerDocument)
    return is_hidden(aElement.parentNode);

  return false;
}

function is_element_visible(aElement, aMsg) {
  isnot(aElement, null, "Element should not be null, when checking visibility");
  ok(!is_hidden(aElement), aMsg);
}

function is_element_hidden(aElement, aMsg) {
  isnot(aElement, null, "Element should not be null, when checking visibility");
  ok(is_hidden(aElement), aMsg);
}