





































var gIndex;
var gCommaIndex = "0";
var gSpaceIndex = "1";
var gOtherIndex = "2";


function Startup()
{
  if (!GetCurrentEditor())
  {
    window.close();
    return;
  }

  gDialog.sepRadioGroup      = document.getElementById("SepRadioGroup");
  gDialog.sepCharacterInput  = document.getElementById("SepCharacterInput");
  gDialog.deleteSepCharacter = document.getElementById("DeleteSepCharacter");
  gDialog.collapseSpaces     = document.getElementById("CollapseSpaces");

  
  gDialog.sepCharacterInput.value = gDialog.sepRadioGroup.getAttribute("character");

  gIndex = gDialog.sepRadioGroup.getAttribute("index");

  switch (gIndex)
  {
    case gCommaIndex:
    default:
      gDialog.sepRadioGroup.selectedItem = document.getElementById("comma");
      break;
    case gSpaceIndex:
      gDialog.sepRadioGroup.selectedItem = document.getElementById("space");
      break;
    case gOtherIndex:
      gDialog.sepRadioGroup.selectedItem = document.getElementById("other");
      break;
  }

  
  SelectCharacter(gIndex);

  SetWindowLocation();
}

function InputSepCharacter()
{
  var str = gDialog.sepCharacterInput.value;

  
  if (str.length > 1)
    str = str.slice(0,1);

  
  if (str == "<" || str == ">" || str == "&" || str == ";" || str == " ")
    str = "";

  gDialog.sepCharacterInput.value = str;
}

function SelectCharacter(radioGroupIndex)
{
  gIndex = radioGroupIndex;
  SetElementEnabledById("SepCharacterInput", gIndex == gOtherIndex);
  SetElementEnabledById("CollapseSpaces", gIndex == gSpaceIndex);
}

function onAccept()
{
  var sepCharacter = "";
  switch ( gIndex )
  {
    case gCommaIndex:
      sepCharacter = ",";
      break;
    case gSpaceIndex:
      sepCharacter = " ";
      break;
    case gOtherIndex:
      sepCharacter = gDialog.sepCharacterInput.value.slice(0,1);
      break;
  }

  var editor = GetCurrentEditor();
  var str;
  try {
    str = editor.outputToString("text/html", kOutputLFLineBreak | kOutputSelectionOnly);
  } catch (e) {}
  if (!str)
  {
    SaveWindowLocation();
    return true;
  }

  
  str = str.replace(/\u00a0/g, " ");

  
  str = str.replace(/\s*<\/p>\s*/g, "");

  
  
  
  str = str.replace(/\s*<p>\s*|\s*<br>\s*/g, "<br>");

  
  str = str.replace(/^(<br>)+/, "");

  
  str = str.replace(/(<br>)+$/, "");

  
  
  

  
  str = str.replace(/^\s+|\s+$/, "");

  
  
  
  var stack = [];
  var start;
  var end;
  var searchStart = 0;
  var listSeparator = "";
  var listItemSeparator = "";
  var endList = false;

  do {
    start = str.indexOf("<", searchStart);

    if (start >= 0)
    {
      end = str.indexOf(">", start+1);
      if (end > start)
      {
        var tagContent = TrimString(str.slice(start+1, end));

        if ( /^ol|^ul|^dl/.test(tagContent) )
        {
          
          
          str = str.slice(0, start) + listSeparator + str.slice(end+1);
          if (listSeparator == "")
            listSeparator = "<br>";
          
          
          listItemSeparator = "";
        }
        else if ( /^li|^dt|^dd/.test(tagContent) )
        {
          
          if (endList)
            listItemSeparator = "<br>";

          
          str = str.slice(0, start) + listItemSeparator + str.slice(end+1);

          if (endList || listItemSeparator == "")
            listItemSeparator = sepCharacter;

          endList = false;
        }
        else 
        {
          
          endList = /^\/ol|^\/ul|^\/dl/.test(tagContent);
          if ( endList || /^\/li|^\/dt|^\/dd/.test(tagContent) )
          {
            
            str = str.slice(0, start) + str.slice(end+1);
          }
          else
          {
            
            stack.push(tagContent);
           
            
            start++;
            str = str.slice(0, start) + str.slice(end);
          }
        }
      }
      searchStart = start + 1;
    }
  } while (start >= 0);

  
  var replaceString;
  if (gDialog.deleteSepCharacter.checked)
  {
    replaceString = "";
  }  
  else
  {
    
    
    replaceString = sepCharacter;
  }

  replaceString += "<td>"; 

  if (sepCharacter.length > 0)
  {
    var tempStr = sepCharacter;
    var regExpChars = ".!@#$%^&*-+[]{}()\|\\\/";
    if (regExpChars.indexOf(sepCharacter) >= 0)
      tempStr = "\\" + sepCharacter;

    if (gIndex == gSpaceIndex)
    {
      
      
      if (gDialog.collapseSpaces.checked)
          tempStr = "\\s+"
        else
          tempStr = "\\s";
    }
    var pattern = new RegExp(tempStr, "g");
    str = str.replace(pattern, replaceString);
  }

  
  searchStart = 0;
  var stackIndex = 0;
  do {
    start = str.indexOf("<", searchStart);
    end = start + 1;
    if (start >= 0 && str.charAt(end) == ">")
    {
      
      str = str.slice(0, end) + stack[stackIndex++] + str.slice(end);
    }
    searchStart = end;

  } while (start >= 0);

  
  str = str.replace(/\s*<br>\s*/g, "</tr>\n<tr><td>");

  
  
  
  str = "<table border=\"1\" width=\"100%\" cellpadding=\"2\" cellspacing=\"2\">\n<tr><td>" + str + "</tr>\n</table>\n";

  editor.beginTransaction();
  
  
  var nodeBeforeTable = null;
  var nodeAfterTable = null;
  try {
    editor.deleteSelection(0);

    var anchorNodeBeforeInsert = editor.selection.anchorNode;
    var offset = editor.selection.anchorOffset;
    if (anchorNodeBeforeInsert.nodeType == Node.TEXT_NODE)
    {
      
      nodeBeforeTable = anchorNodeBeforeInsert.previousSibling;
      nodeAfterTable = anchorNodeBeforeInsert;
    }
    else
    {
      
      if (offset > 0)
        nodeBeforeTable = anchorNodeBeforeInsert.childNodes.item(offset - 1);

      nodeAfterTable = anchorNodeBeforeInsert.childNodes.item(offset);
    }
  
    editor.insertHTML(str);
  } catch (e) {}

  var table = null;
  if (nodeAfterTable)
  {
    var previous = nodeAfterTable.previousSibling;
    if (previous && previous.nodeName.toLowerCase() == "table")
      table = previous;
  }
  if (!table && nodeBeforeTable)
  {
    var next = nodeBeforeTable.nextSibling;
    if (next && next.nodeName.toLowerCase() == "table")
      table = next;
  }

  if (table)
  {
    
    var prefs = GetPrefs();
    var firstRow;
    try {
      if (prefs && prefs.getBoolPref("editor.table.maintain_structure") )
        editor.normalizeTable(table);

      firstRow = editor.getFirstRow(table);
    } catch(e) {}

    
    if (firstRow)
    {
      var node2 = firstRow.firstChild;
      do {
        if (node2.nodeName.toLowerCase() == "td" ||
            node2.nodeName.toLowerCase() == "th")
        {
          try { 
            editor.selection.collapse(node2, 0);
          } catch(e) {}
          break;
        }
        node2 = node.nextSibling;
      } while (node2);
    }
  }

  editor.endTransaction();

  
  gDialog.sepRadioGroup.setAttribute("index", gIndex);
  if (gIndex == gOtherIndex)
    gDialog.sepRadioGroup.setAttribute("character", sepCharacter);

  SaveWindowLocation();
  return true;
}
