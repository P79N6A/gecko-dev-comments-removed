


"use strict";




const TEST_URI = BASE_URI + "browser_fontinspector.html";

add_task(function*() {
  let { inspector, fontInspector } = yield openFontInspectorForURL(TEST_URI);
  let viewDoc = fontInspector.chromeDoc;

  info("Selecting a node that doesn't contain all document fonts.");
  yield selectNode(".normal-text", inspector);

  let normalTextNumPreviews =
    viewDoc.querySelectorAll("#all-fonts .font-preview").length;

  let onUpdated = inspector.once("fontinspector-updated");

  info("Clicking 'Select all' button.");
  viewDoc.getElementById("showall").click();

  info("Waiting for font-inspector to update.");
  yield onUpdated;

  let allFontsNumPreviews =
    viewDoc.querySelectorAll("#all-fonts .font-preview").length;

  
  
  
  isnot(allFontsNumPreviews, normalTextNumPreviews,
    "The .normal-text didn't show all fonts.");

  info("Editing the preview text.");
  yield updatePreviewText(fontInspector, "The quick brown");

  let numPreviews = viewDoc.querySelectorAll("#all-fonts .font-preview").length;
  is(numPreviews, allFontsNumPreviews,
    "All fonts are still shown after the preview text was edited.");
});
