





var html5Exceptions = {
  "<!doctype html><keygen><frameset>" : true, 
  "<select><keygen>" : true, 
  "<math><mi><div><object><div><span></span></div></object></div></mi><mi>" : true, 
  "<plaintext>\u0000filler\u0000text\u0000" : true, 
  "<body><svg><foreignObject>\u0000filler\u0000text" : true, 
  "<svg>\u0000</svg><frameset>" : true, 
  "<svg>\u0000 </svg><frameset>" : true, 
  "<option><span><option>" : true, 
  "<math><annotation-xml encoding=\"application/xhtml+xml\"><div>" : true, 
  "<math><annotation-xml encoding=\"aPPlication/xhtmL+xMl\"><div>" : true, 
  "<math><annotation-xml encoding=\"text/html\"><div>" : true, 
  "<math><annotation-xml encoding=\"Text/htmL\"><div>" : true, 
}
