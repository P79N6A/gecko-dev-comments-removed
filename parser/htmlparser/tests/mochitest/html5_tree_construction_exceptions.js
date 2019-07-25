





var html5Exceptions = {
  "<!doctype html><keygen><frameset>" : true, 
  "<select><keygen>" : true, 
  "<plaintext>\u0000filler\u0000text\u0000" : true, 
  "<body><svg><foreignObject>\u0000filler\u0000text" : true, 
  "<svg>\u0000</svg><frameset>" : true, 
  "<svg>\u0000 </svg><frameset>" : true, 
  "<option><span><option>" : true, 
  "<!doctype html><div><body><frameset>" : true, 
}
