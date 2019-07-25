





var html5Exceptions = {
  "<!doctype html><keygen><frameset>" : true, 
  "<select><keygen>" : true, 
  "<!DOCTYPE html><body><keygen>A" : true, 
  "<!DOCTYPE html><math><mtext><p><i></p>a" : true, 
  "<!DOCTYPE html><table><tr><td><math><mtext><p><i></p>a" : true, 
  "<!DOCTYPE html><table><tr><td><math><mtext>\u0000a" : true, 
  "<!DOCTYPE html><math><mi>a\u0000b" : true, 
  "<!DOCTYPE html><math><mo>a\u0000b" : true, 
  "<!DOCTYPE html><math><mn>a\u0000b" : true, 
  "<!DOCTYPE html><math><ms>a\u0000b" : true, 
  "<!DOCTYPE html><math><mtext>a\u0000b" : true, 
}
