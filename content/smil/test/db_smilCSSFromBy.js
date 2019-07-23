










































var _fromByTestLists =
{
  color: [
    new AnimTestcaseFromBy("rgb(10, 20, 30)", "currentColor",
                           { midComp: "rgb(35, 45, 55)",
                             toComp:  "rgb(60, 70, 80)"}),
    new AnimTestcaseFromBy("currentColor", "rgb(30, 20, 10)",
                           { fromComp: "rgb(50, 50, 50)",
                             midComp:  "rgb(65, 60, 55)",
                             toComp:   "rgb(80, 70, 60)"}),
  ],
  lengthPx: [
    new AnimTestcaseFromBy("1px", "10px", { midComp: "6px", toComp: "11px"}),
  ],
  opacity: [
    new AnimTestcaseFromBy("1", "-1", { midComp: "0.5", toComp: "0"}),
    new AnimTestcaseFromBy("0.4", "-0.6", { midComp: "0.1", toComp: "0"}),
    new AnimTestcaseFromBy("0.8", "-1.4", { midComp: "0.1", toComp: "0"},
                           "opacities with abs val >1 get clamped too early"),
    new AnimTestcaseFromBy("1.2", "-0.6", { midComp: "0.9", toComp: "0.6"},
                           "opacities with abs val >1 get clamped too early"),
  ],
};


var gFromByBundles =
[
  new TestcaseBundle(gPropList.fill,           _fromByTestLists.color),
  new TestcaseBundle(gPropList.font_size,      _fromByTestLists.lengthPx),
  new TestcaseBundle(gPropList.lighting_color, _fromByTestLists.color),
  new TestcaseBundle(gPropList.opacity,        _fromByTestLists.opacity,
                     "need support for float values"),
  new TestcaseBundle(gPropList.stroke_width,   _fromByTestLists.lengthPx),
];
