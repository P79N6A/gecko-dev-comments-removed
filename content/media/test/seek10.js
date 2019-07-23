function test_seek10(v, seekTime, is, ok, finish) {






function startTest() {
  
  
  
  v.currentTime = v.duration * 0.9;
}

function done(evt) {
  evt.stopPropagation();
  ok(true, "We don't acutally test anything...");
  finish();
}

function seeking() {
  v.onerror = done;
  v.src = "not a valid video file.";
  v.load(); 
}

v.addEventListener("loadeddata", startTest, false);
v.addEventListener("seeking", seeking, false);

}
