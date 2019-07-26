





Components.utils.import("resource://gre/modules/Services.jsm");

var gConsole, gConsoleBundle, gTextBoxEval, gEvaluator, gCodeToEvaluate;
var gFilter;



window.onload = function()
{
  gConsole = document.getElementById("ConsoleBox");
  gConsoleBundle = document.getElementById("ConsoleBundle");
  gTextBoxEval = document.getElementById("TextboxEval");
  gEvaluator = document.getElementById("Evaluator");
  gFilter = document.getElementById("Filter");
  
  updateSortCommand(gConsole.sortOrder);
  updateModeCommand(gConsole.mode);

  gEvaluator.addEventListener("load", loadOrDisplayResult, true);
}



function changeFilter()
{
  gConsole.filter = gFilter.value;

  document.persist("ConsoleBox", "filter");
}

function changeMode(aMode)
{
  switch (aMode) {
    case "Errors":
    case "Warnings":
    case "Messages":
      gConsole.mode = aMode;
      break;
    case "All":
      gConsole.mode = null;
  }
  
  document.persist("ConsoleBox", "mode");
}

function clearConsole()
{
  gConsole.clear();
}

function changeSortOrder(aOrder)
{
  updateSortCommand(gConsole.sortOrder = aOrder);
}

function updateSortCommand(aOrder)
{
  var orderString = aOrder == 'reverse' ? "Descend" : "Ascend";
  var bc = document.getElementById("Console:sort"+orderString);
  bc.setAttribute("checked", true);  

  orderString = aOrder == 'reverse' ? "Ascend" : "Descend";
  bc = document.getElementById("Console:sort"+orderString);
  bc.setAttribute("checked", false);
}

function updateModeCommand(aMode)
{
  
  
  var bc = document.getElementById("Console:mode" + aMode) ||
           document.getElementById("Console:modeAll");
  bc.setAttribute("checked", true);
}

function onEvalKeyPress(aEvent)
{
  if (aEvent.keyCode == 13)
    evaluateTypein();
}

function evaluateTypein()
{
  gCodeToEvaluate = gTextBoxEval.value;
  
  
  gEvaluator.contentWindow.location = "about:blank";
}

function loadOrDisplayResult()
{
  if (gCodeToEvaluate) {
    gEvaluator.contentWindow.location = "javascript: " +
                                        gCodeToEvaluate.replace(/%/g, "%25");
    gCodeToEvaluate = "";
    return;
  }

  var resultRange = gEvaluator.contentDocument.createRange();
  resultRange.selectNode(gEvaluator.contentDocument.documentElement);
  var result = resultRange.toString();
  if (result)
    Services.console.logStringMessage(result);
    
}
