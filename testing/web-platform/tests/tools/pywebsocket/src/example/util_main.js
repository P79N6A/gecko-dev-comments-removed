







var logBox = null;
var queuedLog = '';

var summaryBox = null;

function queueLog(log) {
  queuedLog += log + '\n';
}

function addToLog(log) {
  logBox.value += queuedLog;
  queuedLog = '';
  logBox.value += log + '\n';
  logBox.scrollTop = 1000000;
}

function addToSummary(log) {
  summaryBox.value += log + '\n';
  summaryBox.scrollTop = 1000000;
}




function measureValue(value) {
}

function getIntFromInput(id) {
  return parseInt(document.getElementById(id).value);
}

function getStringFromRadioBox(name) {
  var list = document.getElementById('benchmark_form')[name];
  for (var i = 0; i < list.length; ++i)
    if (list.item(i).checked)
      return list.item(i).value;
  return undefined;
}
function getBoolFromCheckBox(id) {
  return document.getElementById(id).checked;
}

function getIntArrayFromInput(id) {
  var strArray = document.getElementById(id).value.split(',');
  return strArray.map(function(str) { return parseInt(str, 10); });
}

function onMessage(message) {
  if (message.data.type === 'addToLog')
    addToLog(message.data.data);
  else if (message.data.type === 'addToSummary')
    addToSummary(message.data.data);
  else if (message.data.type === 'measureValue')
    measureValue(message.data.data);
}
