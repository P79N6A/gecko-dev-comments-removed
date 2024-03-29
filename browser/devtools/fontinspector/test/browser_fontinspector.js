


"use strict";

const TEST_URI = BASE_URI + "browser_fontinspector.html";
const FONTS = [
  {name: "Ostrich Sans Medium", remote: true, url: BASE_URI + "ostrich-regular.ttf",
   format: "truetype", cssName: "bar"},
  {name: "Ostrich Sans Black", remote: true, url: BASE_URI + "ostrich-black.ttf",
   format: "", cssName: "bar"},
  {name: "Ostrich Sans Black", remote: true, url: BASE_URI + "ostrich-black.ttf",
   format: "", cssName: "bar"},
  {name: "Ostrich Sans Medium", remote: true, url: BASE_URI + "ostrich-regular.ttf",
   format: "", cssName: "barnormal"},
];

add_task(function*() {
  let { inspector, fontInspector } = yield openFontInspectorForURL(TEST_URI);
  ok(!!fontInspector, "Font inspector document is alive.");

  let viewDoc = fontInspector.chromeDoc;

  yield testBodyFonts(inspector, viewDoc);
  yield testDivFonts(inspector, viewDoc);
  yield testShowAllFonts(inspector, viewDoc);
});

function* testBodyFonts(inspector, viewDoc) {
  let s = viewDoc.querySelectorAll("#all-fonts > section");
  is(s.length, 5, "Found 5 fonts");

  for (let i = 0; i < FONTS.length; i++) {
    let section = s[i];
    let font = FONTS[i];
    is(section.querySelector(".font-name").textContent, font.name,
       "font " + i + " right font name");
    is(section.classList.contains("is-remote"), font.remote,
       "font " + i + " remote value correct");
    is(section.querySelector(".font-url").value, font.url,
       "font " + i + " url correct");
    is(section.querySelector(".font-format").hidden, !font.format,
       "font " + i + " format hidden value correct");
    is(section.querySelector(".font-format").textContent,
       font.format, "font " + i + " format correct");
    is(section.querySelector(".font-css-name").textContent,
       font.cssName, "font " + i + " css name correct");
  }

  
  let regSrc = s[0].querySelector(".font-preview").src;
  let boldSrc = s[1].querySelector(".font-preview").src;
  isnot(regSrc, boldSrc, "preview for bold font is different from regular");

  
  let localFontName = s[4].querySelector(".font-name").textContent;
  let localFontCSSName = s[4].querySelector(".font-css-name").textContent;

  
  
  ok((localFontName == "Arial") || (localFontName == "Liberation Sans"),
     "local font right font name");
  ok(s[4].classList.contains("is-local"), "local font is local");
  ok((localFontCSSName == "Arial") || (localFontCSSName == "Liberation Sans"),
     "Arial", "local font has right css name");
}

function* testDivFonts(inspector, viewDoc) {
  let updated = inspector.once("fontinspector-updated");
  yield selectNode("div", inspector);
  yield updated;

  let sections1 = viewDoc.querySelectorAll("#all-fonts > section");
  is(sections1.length, 1, "Found 1 font on DIV");
  is(sections1[0].querySelector(".font-name").textContent, "Ostrich Sans Medium",
    "The DIV font has the right name");
}

function* testShowAllFonts(inspector, viewDoc) {
  info("testing showing all fonts");

  let updated = inspector.once("fontinspector-updated");
  viewDoc.querySelector("#showall").click();
  yield updated;

  
  is(inspector.selection.nodeFront.nodeName, "DIV", "Show all fonts selected");
  let sections = viewDoc.querySelectorAll("#all-fonts > section");
  is(sections.length, 6, "Font inspector shows 6 fonts (1 from iframe)");
}
