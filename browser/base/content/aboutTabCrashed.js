



function parseQueryString() {
  let url = document.documentURI;
  let queryString = url.replace(/^about:tabcrashed?e=tabcrashed/, "");

  let titleMatch = queryString.match(/d=([^&]*)/);
  return titleMatch && titleMatch[1] ? decodeURIComponent(titleMatch[1]) : "";
}

document.title = parseQueryString();


var event = new CustomEvent("AboutTabCrashedLoad", {bubbles:true});
document.dispatchEvent(event);
