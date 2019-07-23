




var id;
var ua = navigator.userAgent;

if (/Windows/.test(ua))
  id = "win";
else if (/Linux/.test(ua))
  id = "linux";
else if (/Mac OS X/.test(ua))
  id = "mac";

if (id)
  document.documentElement.setAttribute("id", id);
else
  document.documentElement.appendChild(
    document.createTextNode("Unrecognized platform")
  );
