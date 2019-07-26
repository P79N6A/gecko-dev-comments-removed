


const BOUNDARY_CHAR = nsIAccessibleText.BOUNDARY_CHAR;
const BOUNDARY_WORD_START = nsIAccessibleText.BOUNDARY_WORD_START;
const BOUNDARY_WORD_END = nsIAccessibleText.BOUNDARY_WORD_END;
const BOUNDARY_LINE_START = nsIAccessibleText.BOUNDARY_LINE_START;
const BOUNDARY_LINE_END = nsIAccessibleText.BOUNDARY_LINE_END;
const BOUNDARY_ATTRIBUTE_RANGE = nsIAccessibleText.BOUNDARY_ATTRIBUTE_RANGE;

const kTextEndOffset = nsIAccessibleText.TEXT_OFFSET_END_OF_TEXT;
const kCaretOffset = nsIAccessibleText.TEXT_OFFSET_CARET;

const kTodo = 1;
const kOk = 2;







function testCharacterCount(aIDs, aCount)
{
  for (var i = 0; i < aIDs.length; i++) {
    var textacc = getAccessible(aIDs[i], [nsIAccessibleText]);
    is(textacc.characterCount, aCount,
       "Wrong character count for " + prettyName(aIDs[i]));
  }
}









function testText(aIDs, aStartOffset, aEndOffset, aText)
{
  for (var i = 0; i < aIDs.length; i++)
  {
    var acc = getAccessible(aIDs[i], nsIAccessibleText);
    try {
      is(acc.getText(aStartOffset, aEndOffset), aText,
         "getText: wrong text between start and end offsets '" + aStartOffset +
         "', '" + aEndOffset + " for '" + prettyName(aIDs[i]) + "'");
    } catch (e) {
      ok(false,
         "getText fails between start and end offsets '" + aStartOffset +
         "', '" + aEndOffset + " for '" + prettyName(aIDs[i]) + "'");
    }
  }
}












function testPasswordText(aIDs, aStartOffset, aEndOffset, aText)
{
  for (var i = 0; i < aIDs.length; i++)
  {
    var acc = getAccessible(aIDs[i], nsIAccessibleText);
    try {
      isnot(acc.getText(aStartOffset, aEndOffset), aText,
         "getText: plain text between start and end offsets '" + aStartOffset +
         "', '" + aEndOffset + " for '" + prettyName(aIDs[i]) + "'");
    } catch (e) {
      ok(false,
         "getText fails between start and end offsets '" + aStartOffset +
         "', '" + aEndOffset + " for '" + prettyName(aIDs[i]) + "'");
    }
  }
}











function testCharAtOffset(aIDs, aOffset, aChar, aStartOffset, aEndOffset)
{
  var IDs = (aIDs instanceof Array) ? aIDs : [ aIDs ];
  for (var i = 0; i < IDs.length; i++) {
    var acc = getAccessible(IDs[i], nsIAccessibleText);
    testTextHelper(IDs[i], aOffset, BOUNDARY_CHAR,
                   aChar, aStartOffset, aEndOffset,
                   kOk, kOk, kOk,
                   acc.getTextAtOffset, "getTextAtOffset ");
  }
}















function testTextAtOffset(aOffset, aBoundaryType, aText,
                          aStartOffset, aEndOffset)
{
  
  if (arguments[5] instanceof Array) {
    var ids = arguments[5];
    for (var i = 0; i < ids.length; i++) {
      var acc = getAccessible(ids[i], nsIAccessibleText);
      testTextHelper(ids[i], aOffset, aBoundaryType,
                     aText, aStartOffset, aEndOffset,
                     kOk, kOk, kOk,
                     acc.getTextAtOffset, "getTextAtOffset ");
    }

    return;
  }

  for (var i = 5; i < arguments.length; i = i + 4) {
    var ID = arguments[i];
    var acc = getAccessible(ID, nsIAccessibleText);
    var toDoFlag1 = arguments[i + 1];
    var toDoFlag2 = arguments[i + 2];
    var toDoFlag3 = arguments[i + 3];

    testTextHelper(ID, aOffset, aBoundaryType,
                   aText, aStartOffset, aEndOffset,
                   toDoFlag1, toDoFlag2, toDoFlag3,
                   acc.getTextAtOffset, "getTextAtOffset ");
  }
}











function testCharAfterOffset(aIDs, aOffset, aChar, aStartOffset, aEndOffset)
{
  var IDs = (aIDs instanceof Array) ? aIDs : [ aIDs ];
  for (var i = 0; i < IDs.length; i++) {
    var acc = getAccessible(IDs[i], nsIAccessibleText);
    testTextHelper(IDs[i], aOffset, BOUNDARY_CHAR,
                   aChar, aStartOffset, aEndOffset,
                   kOk, kOk, kOk,
                   acc.getTextAfterOffset, "getTextAfterOffset ");
  }
}















function testTextAfterOffset(aOffset, aBoundaryType,
                             aText, aStartOffset, aEndOffset)
{
  
  if (arguments[5] instanceof Array) {
    var ids = arguments[5];
    for (var i = 0; i < ids.length; i++) {
      var acc = getAccessible(ids[i], nsIAccessibleText);
      testTextHelper(ids[i], aOffset, aBoundaryType,
                     aText, aStartOffset, aEndOffset,
                     kOk, kOk, kOk,
                     acc.getTextAfterOffset, "getTextAfterOffset ");
    }

    return;
  }

  
  for (var i = 5; i < arguments.length; i = i + 4) {
    var ID = arguments[i];
    var acc = getAccessible(ID, nsIAccessibleText);
    var toDoFlag1 = arguments[i + 1];
    var toDoFlag2 = arguments[i + 2];
    var toDoFlag3 = arguments[i + 3];

    testTextHelper(ID, aOffset, aBoundaryType,
                   aText, aStartOffset, aEndOffset,
                   toDoFlag1, toDoFlag2, toDoFlag3, 
                   acc.getTextAfterOffset, "getTextAfterOffset ");
  }
}











function testCharBeforeOffset(aIDs, aOffset, aChar, aStartOffset, aEndOffset)
{
  var IDs = (aIDs instanceof Array) ? aIDs : [ aIDs ];
  for (var i = 0; i < IDs.length; i++) {
    var acc = getAccessible(IDs[i], nsIAccessibleText);
    testTextHelper(IDs[i], aOffset, BOUNDARY_CHAR,
                   aChar, aStartOffset, aEndOffset,
                   kOk, kOk, kOk,
                   acc.getTextBeforeOffset, "getTextBeforeOffset ");
  }
}















function testTextBeforeOffset(aOffset, aBoundaryType,
                              aText, aStartOffset, aEndOffset)
{
  
  if (arguments[5] instanceof Array) {
    var ids = arguments[5];
    for (var i = 0; i < ids.length; i++) {
      var acc = getAccessible(ids[i], nsIAccessibleText);
      testTextHelper(ids[i], aOffset, aBoundaryType,
                     aText, aStartOffset, aEndOffset,
                     kOk, kOk, kOk,
                     acc.getTextBeforeOffset, "getTextBeforeOffset ");
    }

    return;
  }

  for (var i = 5; i < arguments.length; i = i + 4) {
    var ID = arguments[i];
    var acc = getAccessible(ID, nsIAccessibleText);
    var toDoFlag1 = arguments[i + 1];
    var toDoFlag2 = arguments[i + 2];
    var toDoFlag3 = arguments[i + 3];

    testTextHelper(ID, aOffset, aBoundaryType,
                   aText, aStartOffset, aEndOffset,
                   toDoFlag1, toDoFlag2, toDoFlag3,
                   acc.getTextBeforeOffset, "getTextBeforeOffset ");
  }
}








function testWordCount(aElement, aCount, aToDoFlag)
{
  var isFunc = (aToDoFlag == kTodo) ? todo_is : is;
  var acc = getAccessible(aElement, nsIAccessibleText);
  var startOffsetObj = {}, endOffsetObj = {};
  var length = acc.characterCount;
  var offset = 0;
  var wordCount = 0;
  while (true) {
    var text = acc.getTextAtOffset(offset, BOUNDARY_WORD_START,
                                   startOffsetObj, endOffsetObj);
    if (offset >= length)
      break;

    wordCount++;
    offset = endOffsetObj.value;
  }
  isFunc(wordCount, aCount,
        "wrong words count for '" + acc.getText(0, -1) + "': " + wordCount +
        " in " + prettyName(aElement));
}









function testWordAt(aElement, aWordIndex, aText, aToDoFlag)
{
  var isFunc = (aToDoFlag == kTodo) ? todo_is : is;
  var acc = getAccessible(aElement, nsIAccessibleText);

  var textLength = acc.characterCount;
  var wordIdx = aWordIndex;
  var startOffsetObj = { value: 0 }, endOffsetObj = { value: 0 };
  for (offset = 0; offset < textLength; offset = endOffsetObj.value) {
    acc.getTextAtOffset(offset, BOUNDARY_WORD_START,
                        startOffsetObj, endOffsetObj);

    wordIdx--;
    if (wordIdx < 0)
      break;
  }

  if (wordIdx >= 0) {
    ok(false,
       "the given word index '" + aWordIndex + "' exceeds words amount in " +
       prettyName(aElement));

    return;
  }

  var startWordOffset = startOffsetObj.value;
  var endWordOffset = endOffsetObj.value;

  
  acc.getTextAtOffset(endOffsetObj.value, BOUNDARY_WORD_END,
                      startOffsetObj, endOffsetObj);
  if (startOffsetObj.value != textLength)
    endWordOffset = startOffsetObj.value

  if (endWordOffset <= startWordOffset) {
    todo(false,
         "wrong start and end offset for word at index '" + aWordIndex + "': " +
         " of text '" + acc.getText(0, -1) + "' in " + prettyName(aElement));

    return;
  }

  text = acc.getText(startWordOffset, endWordOffset);
  isFunc(text, aText,  "wrong text for word at index '" + aWordIndex + "': " +
         " of text '" + acc.getText(0, -1) + "' in " + prettyName(aElement));
}








function testWords(aElement, aWords, aToDoFlag)
{
  if (aToDoFlag == null)
    aToDoFlag = kOk;

  testWordCount(aElement, aWords.length, aToDoFlag);

  for (var i = 0; i < aWords.length; i++) {
    testWordAt(aElement, i, aWords[i], aToDoFlag);
  }
}






function cleanTextSelections(aID)
{
  var acc = getAccessible(aID, [nsIAccessibleText]);

  while (acc.selectionCount > 0)
    acc.removeSelection(0);
}









function testTextAddSelection(aID, aStartOffset, aEndOffset, aSelectionsCount)
{
  var acc = getAccessible(aID, [nsIAccessibleText]);
  var text = acc.getText(0, -1);

  acc.addSelection(aStartOffset, aEndOffset);

  ok(acc.selectionCount, aSelectionsCount,
     text + ": failed to add selection from offset '" + aStartOffset +
     "' to offset '" + aEndOffset + "': selectionCount after");
}









function testTextRemoveSelection(aID, aSelectionIndex, aSelectionsCount)
{
  var acc = getAccessible(aID, [nsIAccessibleText]);
  var text = acc.getText(0, -1);

  acc.removeSelection(aSelectionIndex);

  ok(acc.selectionCount, aSelectionsCount, 
     text + ": failed to remove selection at index '" + 
     aSelectionIndex + "': selectionCount after");
}











function testTextSetSelection(aID, aStartOffset, aEndOffset,
                              aSelectionIndex, aSelectionsCount)
{
  var acc = getAccessible(aID, [nsIAccessibleText]);
  var text = acc.getText(0, -1);

  acc.setSelectionBounds(aSelectionIndex, aStartOffset, aEndOffset);
 
  is(acc.selectionCount, aSelectionsCount, 
     text + ": failed to set selection at index '" + 
     aSelectionIndex + "': selectionCount after");
}







function testTextSelectionCount(aID, aCount)
{
  var acc = getAccessible(aID, [nsIAccessibleText]);
  var text = acc.getText(0, -1);

  is(acc.selectionCount, aCount, text + ": wrong selectionCount: ");
}









function testTextGetSelection(aID, aStartOffset, aEndOffset, aSelectionIndex)
{
  var acc = getAccessible(aID, [nsIAccessibleText]);
  var text = acc.getText(0, -1);

  var startObj = {}, endObj = {};
  acc.getSelectionBounds(aSelectionIndex, startObj, endObj);

  is(startObj.value, aStartOffset, text + ": wrong start offset for index '" +
     aSelectionIndex + "'");
  is(endObj.value, aEndOffset, text + ": wrong end offset for index '" +
     aSelectionIndex + "'");
}




function testTextHelper(aID, aOffset, aBoundaryType,
                        aText, aStartOffset, aEndOffset,
                        aToDoFlag1, aToDoFlag2, aToDoFlag3,
                        aTextFunc, aTextFuncName)
{
  var exceptionFlag = aToDoFlag1 == undefined ||
                      aToDoFlag2 == undefined ||
                      aToDoFlag3 == undefined;

  var startMsg = aTextFuncName + "(" + boundaryToString(aBoundaryType) + "): ";
  var endMsg = ", id: " + prettyName(aID) + ";";

  try {
    var startOffsetObj = {}, endOffsetObj = {};
    var text = aTextFunc(aOffset, aBoundaryType,
                         startOffsetObj, endOffsetObj);

    if (exceptionFlag) {
      ok(false, startMsg + "no expected failure at offset " + aOffset + endMsg);
      return;
    }

    var isFunc1 = (aToDoFlag1 == kTodo) ? todo_is : is;
    var isFunc2 = (aToDoFlag2 == kTodo) ? todo_is : is;
    var isFunc3 = (aToDoFlag3 == kTodo) ? todo_is : is;

    isFunc1(text, aText,
            startMsg + "wrong text, offset: " + aOffset + endMsg);
    isFunc2(startOffsetObj.value, aStartOffset,
            startMsg + "wrong start offset, offset: " + aOffset + endMsg);
    isFunc3(endOffsetObj.value, aEndOffset,
            startMsg + "wrong end offset, offset: " + aOffset + endMsg);

  } catch (e) {
    var okFunc = exceptionFlag ? todo : ok;
    okFunc(false, startMsg + "failed at offset " + aOffset + endMsg);
  }
}

function boundaryToString(aBoundaryType)
{
  switch (aBoundaryType) {
    case BOUNDARY_CHAR:
      return "char";
    case BOUNDARY_WORD_START:
      return "word start";
    case BOUNDARY_WORD_END:
      return "word end";
    case BOUNDARY_LINE_START:
      return "line start";
    case BOUNDARY_LINE_END:
      return "line end";
    case BOUNDARY_ATTRIBUTE_RANGE:
      return "attr range";
  }
}
