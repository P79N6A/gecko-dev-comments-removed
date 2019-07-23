




































var body = document.documentElement.childNodes[1];
var para = body.childNodes[1];
var img1 = para.childNodes[5];
var tbody = document.getElementsByTagName("TBODY")[0];
var tparent = tbody.childNodes[0].childNodes[0];
var img2 = tparent.childNodes[0].childNodes[0];

var pos = 0;

function rotate(p, c) {
    p.removeChild(c);
    var child = p.childNodes[pos++];
    if (pos > p.childNodes.length) {
      pos = 0;
    }  
    p.insertBefore(c, child);
}

function rotate2() {
  tparent.childNodes[0].removeChild(img2);
  if (tparent.nextSibling != null) {
    tparent = tparent.nextSibling;
  }
  else if (tparent.parentNode.nextSibling != null) {
    tparent = tparent.parentNode.nextSibling.childNodes[0];
  }
  else {
    tparent = tbody.childNodes[0].childNodes[0];
  }
  tparent.childNodes[0].insertBefore(img2, tparent.childNodes[0].childNodes[0]);
}

var int1 = setInterval(rotate, 1000, para, img1);
var int2 = setInterval(rotate2, 1000)
