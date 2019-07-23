



































 
var gAcc = window.arguments[0];

var gInputArea = null;
var gOutputArea = null;

function load()
{
  gInputArea = document.getElementById("JSInputArea");
  gOutputArea = document.getElementById("JSOutputArea");
}
 
function execute()
{
  if (!gAcc)
    return;

  gOutputArea.value = "";

  var expr = gInputArea.value;
  try {
    var f = Function("accessible", expr);
    var result = f(gAcc);
  } catch (ex) {
      output(ex);
  }
}

function output(aValue)
{
  gOutputArea.value += aValue;
}
