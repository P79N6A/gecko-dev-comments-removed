



function parseQueryString() {
  let url = document.documentURI;
  let queryString = url.replace(/^about:tabcrashed?e=tabcrashed/, "");

  let titleMatch = queryString.match(/d=([^&]*)/);
  return titleMatch && titleMatch[1] ? decodeURIComponent(titleMatch[1]) : "";
}

document.title = parseQueryString();

function shouldSendReport() {
  if (!document.documentElement.classList.contains("crashDumpAvailable"))
    return false;
  return document.getElementById("sendReport").checked;
}

function sendEvent(message) {
  let event = new CustomEvent("AboutTabCrashedMessage", {
    bubbles: true,
    detail: {
      message,
      sendCrashReport: shouldSendReport(),
    },
  });

  document.dispatchEvent(event);
}

function closeTab() {
  sendEvent("closeTab");
}

function restoreTab() {
  sendEvent("restoreTab");
}

function restoreAll() {
  sendEvent("restoreAll");
}


var event = new CustomEvent("AboutTabCrashedLoad", {bubbles:true});
document.dispatchEvent(event);
