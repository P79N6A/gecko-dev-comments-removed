




































var body = document.documentElement.childNodes[1];
var center = body.childNodes[1];
var image = center.childNodes[1];

var images = new Array(10);
images[0] = "0.JPG";
images[1] = "1.JPG";
images[2] = "2.JPG";
images[3] = "3.JPG";
images[4] = "4.JPG";
images[5] = "5.JPG";
images[6] = "6.JPG";
images[7] = "7.JPG";
images[8] = "8.JPG";
images[9] = "9.JPG";

var index = 0;

function slideShow()
{
  image.setAttribute("SRC", images[index]);
  index++;
  if (index >= 10) {
    index = 0;
  }
}

var int1 = setInterval(slideShow, 500);
