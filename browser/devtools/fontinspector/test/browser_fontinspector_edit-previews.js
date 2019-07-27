


"use strict";





const TEST_URI = BASE_URI + "browser_fontinspector.html";

add_task(function*() {
  let { inspector, fontInspector } = yield openFontInspectorForURL(TEST_URI);
  let viewDoc = fontInspector.chromeDoc;

  let previews = viewDoc.querySelectorAll("#all-fonts .font-preview");
  let initialPreviews = [...previews].map(p => p.src);

  info("Typing 'Abc' to check that the reference previews are correct.");
  yield updatePreviewText(fontInspector, "Abc");
  checkPreviewImages(viewDoc, initialPreviews, true);

  info("Typing something else to the preview box.");
  yield updatePreviewText(fontInspector, "The quick brown");
  checkPreviewImages(viewDoc, initialPreviews, false);

  info("Blanking the input to restore default previews.");
  yield updatePreviewText(fontInspector, "");
  checkPreviewImages(viewDoc, initialPreviews, true);
});













function checkPreviewImages(viewDoc, originalURIs, assertIdentical) {
  let previews = viewDoc.querySelectorAll("#all-fonts .font-preview");
  let newURIs = [...previews].map(p => p.src);

  is(newURIs.length, originalURIs.length,
    "The number of previews has not changed.");

  for (let i = 0; i < newURIs.length; ++i) {
    if (assertIdentical) {
      is(newURIs[i], originalURIs[i],
        `The preview image at index ${i} has stayed the same.`);
    } else {
      isnot(newURIs[i], originalURIs[i],
        `The preview image at index ${i} has changed.`);
    }
  }
}
