var httpHostMain = '{{domains[]}}'; 
var httpHostAlias = '{{domains[www]}}'; 
var httpsHostAlias = httpHostAlias; 
var httpPortAlias = {{ports[http][0]}}; 


var httpsPortAlias = 8443;

function crossOriginUrl(subdomain, relative_url) {
  var a = document.createElement("a");
  a.href = relative_url;
  return a.href.replace(location.href.replace("://", "://" + subdomain + "."));
}