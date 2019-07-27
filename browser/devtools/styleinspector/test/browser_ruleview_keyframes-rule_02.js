



"use strict";




const TEST_URI = TEST_URL_ROOT + "doc_keyframeanimation.html";

let test = asyncTest(function*() {
  yield addTab(TEST_URI);

  let {toolbox, inspector, view} = yield openRuleView();

  yield testPacman(inspector, view);
  yield testBoxy(inspector, view);
});

function* testPacman(inspector, view) {
  info("Test content in the keyframes rule of #pacman");

  let {
    rules,
    element,
    elementStyle
  } = yield getKeyframeRules("#pacman", inspector, view);

  info("Test text properties for Keyframes #pacman");

  is
  (
    convertTextPropsToString(rules.keyframeRules[0].textProps),
    "left: 750px",
    "Keyframe pacman (100%) property is correct"
  );

  

  

  
  
  

  
  

  
  
  
  
  
  

  
  
}

function* testBoxy(inspector, view) {
  info("Test content in the keyframes rule of #boxy");

  let {
    rules,
    element,
    elementStyle
  } = yield getKeyframeRules("#boxy", inspector, view);

  info("Test text properties for Keyframes #boxy");

  is
  (
    convertTextPropsToString(rules.keyframeRules[0].textProps),
    "background-color: blue",
    "Keyframe boxy (10%) property is correct"
  );

  is
  (
    convertTextPropsToString(rules.keyframeRules[1].textProps),
    "background-color: green",
    "Keyframe boxy (20%) property is correct"
  );

  is
  (
    convertTextPropsToString(rules.keyframeRules[2].textProps),
    "opacity: 0",
    "Keyframe boxy (100%) property is correct"
  );
}

function convertTextPropsToString(textProps) {
  return textProps.map(t => t.name + ": " + t.value).join("; ");
}

function* getKeyframeRules(selector, inspector, view) {
  let element = getNode(selector);

  yield selectNode(element, inspector);
  let elementStyle = view._elementStyle;

  let rules = {
    elementRules: elementStyle.rules.filter(rule => !rule.keyframes),
    keyframeRules: elementStyle.rules.filter(rule => rule.keyframes)
  };

  return {rules, element, elementStyle};
}
