



function parseQueryString() {
  let url = document.documentURI;
  let queryString = url.replace(/^about:tabcrashed?e=tabcrashed/, "");

  let titleMatch = queryString.match(/d=([^&]*)/);
  return titleMatch && titleMatch[1] ? decodeURIComponent(titleMatch[1]) : "";
}

document.title = parseQueryString();

addEventListener("DOMContentLoaded", () => {
  let tryAgain = document.getElementById("tryAgain");
  let sendCrashReport = document.getElementById("checkSendReport");

  tryAgain.addEventListener("click", () => {
    let event = new CustomEvent("AboutTabCrashedTryAgain", {
      bubbles: true,
      detail: {
        sendCrashReport: sendCrashReport.checked,
      },
    });

    document.dispatchEvent(event);
  });
});


var event = new CustomEvent("AboutTabCrashedLoad", {bubbles:true});
document.dispatchEvent(event);
