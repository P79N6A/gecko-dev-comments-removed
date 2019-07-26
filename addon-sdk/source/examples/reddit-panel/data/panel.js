












$(window).click(function (event) {
  var t = event.target;

  
  if (t.nodeName != "A")
    return;

  
  
  if ($(t).parents('#header').length || $(t).parents('.nextprev').length)
    return;

  
  event.stopPropagation();
  event.preventDefault();
  self.port.emit('click', t.toString());
});




$("body").css("background", "white");
