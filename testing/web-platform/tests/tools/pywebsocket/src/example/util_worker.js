







function workerAddToLog(text) {
  postMessage({type: 'addToLog', data: text});
}

function workerAddToSummary(text) {
  postMessage({type: 'addToSummary', data: text});
}

function workerMeasureValue(value) {
  postMessage({type: 'measureValue', data: value});
}
