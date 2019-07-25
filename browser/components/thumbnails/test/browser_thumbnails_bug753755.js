






function runTests() {
  let url = "http://non.existant.url/";
  let file = PageThumbsStorage.getFileForURL(url);
  ok(!file.exists() && !file.parent.exists(), "file and path don't exist");

  let file = PageThumbsStorage.getFileForURL(url, {createPath: true});
  ok(!file.exists() && file.parent.exists(), "path exists, file doesn't");
}
