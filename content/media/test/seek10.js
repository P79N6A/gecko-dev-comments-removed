function test_seek10(v, seekTime, is, ok, finish) {






function startTest() {
  
  
  
  v.currentTime = v.duration * 0.9;
}

function done(evt) {
  ok(true, "We don't acutally test anything...");
  finish();
}

function seeking() {
  ok(v.currentTime >= seekTime - 0.1, "Video currentTime should be around " + seekTime + ": " + v.currentTime);
  v.onerror = done;
  v.src = "not a valid video file.";
  v.load(); 
}

v.addEventListener("loadeddata", startTest, false);
v.addEventListener("seeking", seeking, false);

}
