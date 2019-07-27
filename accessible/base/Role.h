





#ifndef _role_h_
#define _role_h_







namespace mozilla {
namespace a11y {
namespace roles {

enum Role {
  


  NOTHING = 0,

  



  TITLEBAR = 1,

  




  MENUBAR = 2,

  



  SCROLLBAR = 3,

  




  GRIP = 4,

  


  SOUND = 5,

  


  CURSOR = 6,

  


  CARET = 7,

  






  ALERT = 8,

  




  WINDOW = 9,

  


  INTERNAL_FRAME = 10,

  



  MENUPOPUP = 11,

  




  MENUITEM = 12,

  


  TOOLTIP = 13,

  



  APPLICATION = 14,

  



  DOCUMENT = 15,

  






  PANE = 16,

  


  CHART = 17,

  



  DIALOG = 18,

  


  BORDER = 19,

  




  GROUPING = 20,

  




  SEPARATOR = 21,

  




  TOOLBAR = 22,

  





  STATUSBAR = 23,

  





  TABLE = 24,

  




  COLUMNHEADER = 25,

  



  ROWHEADER = 26,

  


  COLUMN = 27,

  


  ROW = 28,

  



  CELL = 29,

  




  LINK = 30,

  


  HELPBALLOON = 31,

  



  CHARACTER = 32,

  




  LIST = 33,

  


  LISTITEM = 34,

  




  OUTLINE = 35,

  



  OUTLINEITEM = 36,

  



  PAGETAB = 37,

  



  PROPERTYPAGE = 38,

  



  INDICATOR = 39,

  


  GRAPHIC = 40,

  




  STATICTEXT = 41,

  


  TEXT_LEAF = 42,

  



  PUSHBUTTON = 43,

  



  CHECKBUTTON = 44,
  
  






  RADIOBUTTON = 45,
  
  




  COMBOBOX = 46,

  


  DROPLIST = 47,

  




  PROGRESSBAR = 48,

  


  DIAL = 49,

  



  HOTKEYFIELD = 50,

  




  SLIDER = 51,

  




  SPINBUTTON = 52,

  


  DIAGRAM = 53,
  
  



  ANIMATION = 54,

  



  EQUATION = 55,

  


  BUTTONDROPDOWN = 56,

  


  BUTTONMENU = 57,

  


  BUTTONDROPDOWNGRID = 58,

  


  WHITESPACE = 59,

  



  PAGETABLIST = 60,

  


  CLOCK = 61,

  



  SPLITBUTTON = 62,

  




  IPADDRESS = 63,

  


  ACCEL_LABEL = 64,

  


  ARROW  = 65,

  



  CANVAS = 66,

  


  CHECK_MENU_ITEM = 67,

  


  COLOR_CHOOSER  = 68,

  


  DATE_EDITOR = 69,

  



  DESKTOP_ICON = 70,

  



  DESKTOP_FRAME = 71,

  




  DIRECTORY_PANE = 72,

  





  FILE_CHOOSER = 73,

  



  FONT_CHOOSER = 74,

  



  CHROME_WINDOW = 75,

  



  GLASS_PANE = 76,

  



  HTML_CONTAINER = 77,

  


  ICON = 78,

  


  LABEL = 79,

  






  LAYERED_PANE = 80,

  


  OPTION_PANE = 81,

  



  PASSWORD_TEXT = 82,

  



  POPUP_MENU = 83,

  


  RADIO_MENU_ITEM = 84,

  



  ROOT_PANE = 85,

  




  SCROLL_PANE = 86,

  




  SPLIT_PANE = 87,

  



  TABLE_COLUMN_HEADER = 88,

  



  TABLE_ROW_HEADER = 89,

  


  TEAR_OFF_MENU_ITEM = 90,

  


  TERMINAL = 91,

  


  TEXT_CONTAINER = 92,

  



  TOGGLE_BUTTON = 93,

  



  TREE_TABLE = 94,

  





  VIEWPORT = 95,

  


  HEADER = 96,

  


  FOOTER = 97,

  


  PARAGRAPH = 98,

  


  RULER = 99,

  




  AUTOCOMPLETE = 100,

  


  EDITBAR = 101,

  


  ENTRY = 102,

  


  CAPTION = 103,

  






  DOCUMENT_FRAME = 104,

  


  HEADING = 105,

  



  PAGE = 106,

  



  SECTION = 107,

  



  REDUNDANT_OBJECT = 108,

  



  FORM = 109,

  



  IME = 110,

  


  APP_ROOT = 111,

  



  PARENT_MENUITEM = 112,

  


  CALENDAR = 113,

  


  COMBOBOX_LIST = 114,

  


  COMBOBOX_OPTION = 115,

  


  IMAGE_MAP = 116,

  


  OPTION = 117,

  


  RICH_OPTION = 118,

  


  LISTBOX = 119,

  


  FLAT_EQUATION = 120,

  




  GRID_CELL = 121,

  


  EMBEDDED_OBJECT = 122,

  



  NOTE = 123,

  


  FIGURE = 124,

  


  CHECK_RICH_OPTION = 125,

  


  DEFINITION_LIST = 126,

  


  TERM = 127,

  


  DEFINITION = 128,

  


  KEY = 129,

  


  SWITCH = 130,

  


  MATHML_MATH = 131,

  


  MATHML_IDENTIFIER = 132,

  


  MATHML_NUMBER = 133,

  


  MATHML_OPERATOR = 134,

  


  MATHML_TEXT = 135,

  


  MATHML_STRING_LITERAL = 136,

  


  MATHML_GLYPH = 137,

  


  MATHML_ROW = 138,

  


  MATHML_FRACTION = 139,

  


  MATHML_SQUARE_ROOT = 140,

  


  MATHML_ROOT = 141,

  


  MATHML_FENCED = 142,

  


  MATHML_ENCLOSED = 143,

  


  MATHML_STYLE = 144,

  


  MATHML_SUB = 145,

  


  MATHML_SUP = 146,

  


  MATHML_SUB_SUP = 147,

  


  MATHML_UNDER = 148,

  


  MATHML_OVER = 149,

  


  MATHML_UNDER_OVER = 150,

  



  MATHML_MULTISCRIPTS = 151,

  


  MATHML_TABLE = 152,

  


  MATHML_LABELED_ROW = 153,

  


  MATHML_TABLE_ROW = 154,

  


  MATHML_CELL = 155,

  


  MATHML_ACTION = 156,

  


  MATHML_ERROR = 157,

  


  MATHML_STACK = 158,

  


  MATHML_LONG_DIVISION = 159,

  


  MATHML_STACK_GROUP = 160,

  


  MATHML_STACK_ROW = 161,

  


  MATHML_STACK_CARRIES = 162,

  


  MATHML_STACK_CARRY = 163,

  


  MATHML_STACK_LINE = 164,

  


  RADIO_GROUP = 165,

  



  TEXT = 166,

  LAST_ROLE = TEXT
};

} 

typedef enum mozilla::a11y::roles::Role role;

} 
} 

#endif
