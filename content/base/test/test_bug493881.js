





SimpleTest.waitForExplicitFinish();

var legacyProps = ["fgColor", "bgColor", "linkColor", "vlinkColor", "alinkColor"];
var testColors = ["blue", "silver", "green", "orange", "red"];
var rgbTestColors = ["rgb(255, 0, 0)", "rgb(192, 192, 192)", "rgb(0, 128, 0)", "rgb(255, 165, 0)", "rgb(255, 0, 0)"];
var idPropList = [ {id: "plaintext", prop: "color"},
                   {id: "plaintext", prop: "background-color"},
                   {id: "nonvisitedlink", prop: "color"},
                   {id: "visitedlink", prop: "color"} ];
var initialValues = [];

function setAndTestProperty(prop, color) {
  var initial = document[prop];
  document[prop] = color;
  is(document[prop], initial, "document[" + prop + "] not ignored before body");
  return initial;
}





for (var i = 0; i < legacyProps.length; i++) {
  initialValues[i] = setAndTestProperty(legacyProps[i], testColors[i]);
}




addLoadEvent( function() {
  
  for (var i = 0; i < legacyProps.length; i++) {
    is(document[legacyProps[i]], initialValues[i], "document[" + legacyProps[i] + "] altered after body load");
  }
  
  
  
  for (i = 0; i < idPropList.length; i++) {
    var style = window.getComputedStyle(document.getElementById(idPropList[i].id), null);
    var color = style.getPropertyValue(idPropList[i].prop);
    idPropList[i].initialComputedColor = color;
    isnot(color, rgbTestColors[i], "element rendered using before-body style");
  }
  
  
  
  
  
  for (var i = 0; i < legacyProps.length; i++) {
    document[legacyProps[i]] = undefined;
    is(document[legacyProps[i]], "undefined", 
      "Unexpected value of " + legacyProps[i] + " after setting to undefined");
  }
  
  
  
  for (i = 0; i < idPropList.length; i++) {
    var style = window.getComputedStyle(document.getElementById(idPropList[i].id), null);
    var color = style.getPropertyValue(idPropList[i].prop);
    is(color, idPropList[i].initialComputedColor, 
      "element's style changed by setting legacy prop to undefined");
  }

  
  setTimeout(SimpleTest.finish, 0);
});
