var httpHostMain = 'w3c-test.org'; 
var httpHostAlias = 'www.w3c-test.org'; 
var httpsHostAlias = httpHostAlias; 
var httpPortAlias = 81; 
var httpsPortAlias = 8443; 

function crossOriginUrl(subdomain, relative_url) {
  var a = document.createElement("a");
  a.href = relative_url;
  return a.href.replace(location.href.replace("://", "://" + subdomain + "."));
}