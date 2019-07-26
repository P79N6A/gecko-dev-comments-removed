



function parseQueryString() {
  let url = document.documentURI;
  let queryString = url.replace(/^about:tabcrashed?e=tabcrashed/, "");

  let urlMatch = queryString.match(/u=([^&]+)/);
  let url = urlMatch && urlMatch[1] ? decodeURIComponent(urlMatch[1]) : "";

  let titleMatch = queryString.match(/d=([^&]*)/);
  title = titleMatch && titleMatch[1] ? decodeURIComponent(titleMatch[1]) : "";

  return [url, title];
}

let [url, title] = parseQueryString();
document.title = title;
document.getElementById("tryAgain").setAttribute("url", url);
