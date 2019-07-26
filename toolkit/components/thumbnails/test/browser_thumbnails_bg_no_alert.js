


function runTests() {
  let url = "data:text/html,<script>try { alert('yo!'); } catch (e) {}</script>";
  ok(!thumbnailExists(url), "Thumbnail file should not already exist.");

  let capturedURL = yield bgCapture(url);
  is(capturedURL, url, "Captured URL should be URL passed to capture.");
  ok(thumbnailExists(url),
     "Thumbnail file should exist even though it alerted.");
  removeThumbnail(url);
}
