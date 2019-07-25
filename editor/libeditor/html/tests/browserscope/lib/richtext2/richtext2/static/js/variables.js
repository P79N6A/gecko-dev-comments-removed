























var INTERNAL_ERR           = 'INTERNAL ERROR: ';
var SETUP_EXCEPTION        = 'SETUP EXCEPTION: ';
var EXECUTION_EXCEPTION    = 'EXECUTION EXCEPTION: ';
var VERIFICATION_EXCEPTION = 'VERIFICATION EXCEPTION: ';

var SETUP_CONTAINER          = 'WHEN INITIALIZING TEST CONTAINER';
var SETUP_BAD_SELECTION_SPEC = 'BAD SELECTION SPECIFICATION IN TEST OR EXPECTATION STRING';
var SETUP_HTML               = 'WHEN SETTING TEST HTML';
var SETUP_SELECTION          = 'WHEN SETTING SELECTION';
var SETUP_NOCOMMAND          = 'NO COMMAND, GENERAL FUNCTION OR QUERY FUNCTION GIVEN';
var HTML_COMPARISON          = 'WHEN COMPARING OUTPUT HTML';


var SELMODIFY_UNSUPPORTED      = 'UNSUPPORTED selection.modify()';
var SELALLCHILDREN_UNSUPPORTED = 'UNSUPPORTED selection.selectAllChildren()';



var UNSUPPORTED = '<i>false</i> (UNSUPPORTED)';


var VALRESULT_NOT_RUN                = 0;  
var VALRESULT_SETUP_EXCEPTION        = 1;
var VALRESULT_EXECUTION_EXCEPTION    = 2;
var VALRESULT_VERIFICATION_EXCEPTION = 3;
var VALRESULT_UNSUPPORTED            = 4;
var VALRESULT_CANARY                 = 5;  
var VALRESULT_DIFF                   = 6;
var VALRESULT_ACCEPT                 = 7;  
var VALRESULT_EQUAL                  = 8;

var VALOUTPUT = [  
    {css: 'grey',        output: '???',    title: 'The test has not been run yet.'},                                            
    {css: 'exception',   output: 'EXC.',   title: 'Exception was thrown during setup.'},                                        
    {css: 'exception',   output: 'EXC.',   title: 'Exception was thrown during execution.'},                                    
    {css: 'exception',   output: 'EXC.',   title: 'Exception was thrown during result verification.'},                          
    {css: 'unsupported', output: 'UNS.',   title: 'Unsupported command or value'},                                              
    {css: 'canary',      output: 'CANARY', title: 'The command affected the contentEditable root element, or outside HTML.'},   
    {css: 'fail',        output: 'FAIL',   title: 'The result differs from the expectation(s).'},                               
    {css: 'accept',      output: 'ACC.',   title: 'The result is technically correct, but sub-optimal.'},                       
    {css: 'pass',        output: 'PASS',   title: 'The test result matches the expectation.'}                                   
]


var SELRESULT_NOT_RUN = 0;  
var SELRESULT_CANARY  = 1;  
var SELRESULT_DIFF    = 2;
var SELRESULT_NA      = 3;
var SELRESULT_ACCEPT  = 4;  
var SELRESULT_EQUAL   = 5;

var SELOUTPUT = [  
    {css: 'grey',   output: 'grey',   title: 'The test has not been run yet.'},                           
    {css: 'canary', output: 'CANARY', title: 'The selection escaped the contentEditable boundary!'},      
    {css: 'fail',   output: 'FAIL',   title: 'The selection differs from the expectation(s).'},           
    {css: 'na',     output: 'N/A',    title: 'The correctness of the selection could not be verified.'},  
    {css: 'accept', output: 'ACC.',   title: 'The selection is technically correct, but sub-optimal.'},   
    {css: 'pass',   output: 'PASS',   title: 'The selection matches the expectation.'}                    
];


var SELECTION_MARKERS = /[\[\]\{\}\|\^]/;



var ATTRNAME_SEL_START = 'bsselstart';
var ATTRNAME_SEL_END   = 'bsselend';


var DOM_NODE_TYPE_ELEMENT = 1;
var DOM_NODE_TYPE_TEXT    = 3;
var DOM_NODE_TYPE_COMMENT = 8;


var PARAM_DESCRIPTION           = 'desc';
var PARAM_PAD                   = 'pad';
var PARAM_EXECCOMMAND           = 'command';
var PARAM_FUNCTION              = 'function';
var PARAM_QUERYCOMMANDSUPPORTED = 'qcsupported';
var PARAM_QUERYCOMMANDENABLED   = 'qcenabled';
var PARAM_QUERYCOMMANDINDETERM  = 'qcindeterm';
var PARAM_QUERYCOMMANDSTATE     = 'qcstate';
var PARAM_QUERYCOMMANDVALUE     = 'qcvalue';
var PARAM_VALUE                 = 'value';
var PARAM_EXPECTED              = 'expected';
var PARAM_EXPECTED_OUTER        = 'expOuter';
var PARAM_ACCEPT                = 'accept';
var PARAM_ACCEPT_OUTER          = 'accOuter';
var PARAM_CHECK_ATTRIBUTES      = 'checkAttrs';
var PARAM_CHECK_STYLE           = 'checkStyle';
var PARAM_CHECK_CLASS           = 'checkClass';
var PARAM_CHECK_ID              = 'checkID';
var PARAM_STYLE_WITH_CSS        = 'styleWithCSS';


var IDOUT_TR         = '_:TR:';   
var IDOUT_TESTID     = '_:tid';   
var IDOUT_COMMAND    = '_:cmd';   
var IDOUT_VALUE      = '_:val';   
var IDOUT_CHECKATTRS = '_:att';   
var IDOUT_CHECKSTYLE = '_:sty';   
var IDOUT_CONTAINER  = '_:cnt:';  
var IDOUT_STATUSVAL  = '_:sta:';  
var IDOUT_STATUSSEL  = '_:sel:';  
var IDOUT_PAD        = '_:pad';   
var IDOUT_EXPECTED   = '_:exp';   
var IDOUT_ACTUAL     = '_:act:';  


var OUTSTR_YES = '&#x25CF;'; 
var OUTSTR_NO  = '&#x25CB;'; 
var OUTSTR_NA  = '-';


var HTMLTAG_BODY  = 'B:';
var HTMLTAG_OUTER = 'O:';
var HTMLTAG_INNER = 'I:';


var CANARY = 'CAN<br>ARY';



var containers = [
  { id:       'dM',
    iframe:   null,
    win:      null,
    doc:      null,
    body:     null,
    editor:   null,
    tagOpen:  '<body>',
    tagClose: '</body>',
    editorID: null,
    canary:   '',
  },
  { id:       'body',
    iframe:   null,
    win:      null,
    doc:      null,
    body:     null,
    editor:   null,
    tagOpen:  '<body contenteditable="true">',
    tagClose: '</body>',
    editorID: null,
    canary:   ''
  },
  { id:       'div',
    iframe:   null,
    win:      null,
    doc:      null,
    body:     null,
    editor:   null,
    tagOpen:  '<div contenteditable="true" id="editor-div">',
    tagClose: '</div>',
    editorID: 'editor-div',
    canary:   CANARY
  }
];


var win = null;     
var doc = null;     
var body = null;    
var editor = null;  
var sel = null;     


var emitFlagsForCanary = { 
    emitAttrs:         true,
    emitStyle:         true,
    emitClass:         true,
    emitID:            true,
    lowercase:         true,
    canonicalizeUnits: true
};
var emitFlagsForOutput = { 
    emitAttrs:         true,
    emitStyle:         true,
    emitClass:         true,
    emitID:            true,
    lowercase:         false,
    canonicalizeUnits: false
};


var colorShades = ['Lo', 'Hi'];


var testClassIDs = ['Finalized', 'RFC', 'Proposed'];
var testClassCount = testClassIDs.length;


var results = {
    count: 0,
    score: 0
};


var beacon = [];


var True = true;
var False = false;
