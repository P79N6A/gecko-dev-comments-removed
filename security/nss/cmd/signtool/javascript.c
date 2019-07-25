



































#include "signtool.h"
#include <prmem.h>
#include <prio.h>
#include <prenv.h>

static int	javascript_fn(char *relpath, char *basedir, char *reldir,
char *filename, void *arg);
static int	extract_js (char *filename);
static int	copyinto (char *from, char *to);
static PRStatus ensureExists (char *base, char *path);
static int	make_dirs(char *path, PRInt32 file_perms);

static char	*jartree = NULL;
static int	idOrdinal;
static PRBool dumpParse = PR_FALSE;

static char	*event_handlers[] = {
    "onAbort",
    "onBlur",
    "onChange",
    "onClick",
    "onDblClick",
    "onDragDrop",
    "onError",
    "onFocus",
    "onKeyDown",
    "onKeyPress",
    "onKeyUp",
    "onLoad",
    "onMouseDown",
    "onMouseMove",
    "onMouseOut",
    "onMouseOver",
    "onMouseUp",
    "onMove",
    "onReset",
    "onResize",
    "onSelect",
    "onSubmit",
    "onUnload"
};


static int	num_handlers = 23;










int
InlineJavaScript(char *dir, PRBool recurse)
{
    jartree = dir;
    if (verbosity >= 0) {
	PR_fprintf(outputFD, "\nGenerating inline signatures from HTML files in: %s\n",
	     dir);
    }
    if (PR_GetEnv("SIGNTOOL_DUMP_PARSE")) {
	dumpParse = PR_TRUE;
    }

    return foreach(dir, "", javascript_fn, recurse, PR_FALSE ,
         		(void * )NULL);

}






static int	javascript_fn
(char *relpath, char *basedir, char *reldir, char *filename, void *arg)
{
    char	fullname [FNSIZE];

    

    if (!(PL_strcaserstr(filename, ".htm") == filename + strlen(filename) -
        4) && 
        !(PL_strcaserstr(filename, ".html") == filename + strlen(filename) -
        5) && 
        !(PL_strcaserstr(filename, ".shtml") == filename + strlen(filename)
        -6)) {
	return 0;
    }

    


    if (PL_strcaserstr(filename, ".arc") == filename + strlen(filename) - 4)
	return 0;

    if (verbosity >= 0) {
	PR_fprintf(outputFD, "Processing HTML file: %s\n", relpath);
    }

    

    

    if (PL_strcaserstr(reldir, ".arc") == reldir + strlen(reldir) - 4)
	return 0;

    sprintf (fullname, "%s/%s", basedir, relpath);
    return extract_js (fullname);
}







typedef enum {
    TEXT_HTML_STATE = 0,
    SCRIPT_HTML_STATE
} 


HTML_STATE ;

typedef enum {
    
    START_STATE,

    
    GET_ATT_STATE,

    
    PRE_ATT_WS_STATE,

    
    POST_ATT_WS_STATE,

    
    PRE_VAL_WS_STATE,

    
    GET_VALUE_STATE,

    
    GET_QUOTED_VAL_STATE,

    
    DONE_STATE,

    
    ERR_STATE
} 


TAG_STATE ;

typedef struct AVPair_Str {
    char	*attribute;
    char	*value;
    unsigned int	valueLine; 
    struct AVPair_Str *next;
} AVPair;

typedef enum {
    APPLET_TAG,
    SCRIPT_TAG,
    LINK_TAG,
    STYLE_TAG,
    COMMENT_TAG,
    OTHER_TAG
} 


TAG_TYPE ;

typedef struct {
    TAG_TYPE type;
    AVPair * attList;
    AVPair * attListTail;
    char	*text;
} TagItem;

typedef enum {
    TAG_ITEM,
    TEXT_ITEM
} 


ITEM_TYPE ;

typedef struct HTMLItem_Str {
    unsigned int	startLine;
    unsigned int	endLine;
    ITEM_TYPE type;
    union {
	TagItem *tag;
	char	*text;
    } item;
    struct HTMLItem_Str *next;
} HTMLItem;

typedef struct {
    PRFileDesc *fd;
    PRInt32 curIndex;
    PRBool IsEOF;
#define FILE_BUFFER_BUFSIZE 512
    char	buf[FILE_BUFFER_BUFSIZE];
    PRInt32 startOffset;
    PRInt32 maxIndex;
    unsigned int	lineNum;
} FileBuffer;






static HTMLItem*CreateTextItem(char *text, unsigned int startline,
unsigned int endline);
static HTMLItem*CreateTagItem(TagItem*ti, unsigned int startline,
unsigned int endline);
static TagItem*ProcessTag(FileBuffer*fb, char **errStr);
static void	DestroyHTMLItem(HTMLItem *item);
static void	DestroyTagItem(TagItem*ti);
static TAG_TYPE GetTagType(char *att);
static FileBuffer*FB_Create(PRFileDesc*fd);
static int	FB_GetChar(FileBuffer *fb);
static PRInt32 FB_GetPointer(FileBuffer *fb);
static PRInt32 FB_GetRange(FileBuffer *fb, PRInt32 start, PRInt32 end,
char **buf);
static unsigned int	FB_GetLineNum(FileBuffer *fb);
static void	FB_Destroy(FileBuffer *fb);
static void	PrintTagItem(PRFileDesc *fd, TagItem *ti);
static void	PrintHTMLStream(PRFileDesc *fd, HTMLItem *head);





static HTMLItem*
CreateTextItem(char *text, unsigned int startline, unsigned int endline)
{
    HTMLItem * item;

    item = PR_Malloc(sizeof(HTMLItem));
    if (!item) {
	return NULL;
    }

    item->type = TEXT_ITEM;
    item->item.text = text;
    item->next = NULL;
    item->startLine = startline;
    item->endLine = endline;

    return item;
}






static HTMLItem*
CreateTagItem(TagItem*ti, unsigned int startline, unsigned int endline)
{
    HTMLItem * item;

    item = PR_Malloc(sizeof(HTMLItem));
    if (!item) {
	return NULL;
    }

    item->type = TAG_ITEM;
    item->item.tag = ti;
    item->next = NULL;
    item->startLine = startline;
    item->endLine = endline;

    return item;
}


static PRBool
isAttChar(int c)
{
    return (isalnum(c) || c == '/' || c == '-');
}






static TagItem*
ProcessTag(FileBuffer*fb, char **errStr)
{
    TAG_STATE state;
    PRInt32 startText, startID, curPos;
    PRBool firstAtt;
    int	curchar;
    TagItem * ti = NULL;
    AVPair * curPair = NULL;
    char	quotechar = '\0';
    unsigned int	linenum;
    unsigned int	startline;

    state = START_STATE;

    startID = FB_GetPointer(fb);
    startText = startID;
    firstAtt = PR_TRUE;

    ti = (TagItem * ) PR_Malloc(sizeof(TagItem));
    if (!ti) 
	out_of_memory();
    ti->type = OTHER_TAG;
    ti->attList = NULL;
    ti->attListTail = NULL;
    ti->text = NULL;

    startline = FB_GetLineNum(fb);

    while (state != DONE_STATE && state != ERR_STATE) {
	linenum = FB_GetLineNum(fb);
	curchar = FB_GetChar(fb);
	if (curchar == EOF) {
	    *errStr = PR_smprintf(
	        "line %d: Unexpected end-of-file while parsing tag starting at line %d.\n",
	         linenum, startline);
	    state = ERR_STATE;
	    continue;
	}

	switch (state) {
	case START_STATE:
	    if (curchar == '!') {
		








		PRBool inComment = PR_FALSE;
		short	hyphenCount = 0; 

		while (1) {
		    linenum = FB_GetLineNum(fb);
		    curchar = FB_GetChar(fb);
		    if (curchar == EOF) {
			
			*errStr = PR_smprintf(
    "line %d: Unexpected end-of-file inside comment starting at line %d.\n",
  						linenum, startline);
			state = ERR_STATE;
			break;
		    }
		    if (curchar == '-') {
			if (hyphenCount == 1) {
			    
			    inComment = !inComment;
			    hyphenCount = 0;
			} else {
			    
			    hyphenCount = 1;
			}
		    } else if (curchar == '>') {
			if (!inComment) {
			    
			    state = DONE_STATE;
			    break;
			} else {
			    

			    hyphenCount = 0;
			}
		    } else {
			hyphenCount = 0;
		    }
		}
		ti->type = COMMENT_TAG;
		break;
	    }
	    
	case GET_ATT_STATE:
	    if (isspace(curchar) || curchar == '=' || curchar
	        == '>') {
		
		curPos = FB_GetPointer(fb) - 2;
		if (curPos >= startID) {
		    
		    curPair = (AVPair * )PR_Malloc(sizeof(AVPair));
		    if (!curPair) 
			out_of_memory();
		    curPair->value = NULL;
		    curPair->next = NULL;
		    FB_GetRange(fb, startID, curPos,
		        &curPair->attribute);

		    
		    if (ti->attListTail) {
			ti->attListTail->next = curPair;
			ti->attListTail = curPair;
		    } else {
			ti->attList = ti->attListTail =
			    curPair;
		    }

		    

		    if (firstAtt) {
			ti->type = GetTagType(curPair->attribute);
			startText = FB_GetPointer(fb)
			    -1;
			firstAtt = PR_FALSE;
		    }
		} else {
		    if (curchar == '=') {
			

			*errStr = PR_smprintf("line %d: Malformed tag starting at line %d.\n",
			     linenum, startline);
			state = ERR_STATE;
			break;
		    }
		}

		
		if (curchar == '=') {
		    startID = FB_GetPointer(fb);
		    state = PRE_VAL_WS_STATE;
		} else if (curchar == '>') {
		    state = DONE_STATE;
		} else if (curPair) {
		    state = POST_ATT_WS_STATE;
		} else {
		    state = PRE_ATT_WS_STATE;
		}
	    } else if (isAttChar(curchar)) {
		
		state = GET_ATT_STATE;
	    } else {
		
		*errStr = PR_smprintf("line %d: Bogus chararacter '%c' in tag.\n",
		     			linenum, curchar);
		state = ERR_STATE;
		break;
	    }
	    break;
	case PRE_ATT_WS_STATE:
	    if (curchar == '>') {
		state = DONE_STATE;
	    } else if (isspace(curchar)) {
		
	    } else if (isAttChar(curchar)) {
		
		startID = FB_GetPointer(fb) - 1;
		state = GET_ATT_STATE;
	    } else {
		
		*errStr = PR_smprintf("line %d: Bogus character '%c' in tag.\n",
		     			linenum, curchar);
		state = ERR_STATE;
		break;
	    }
	    break;
	case POST_ATT_WS_STATE:
	    if (curchar == '>') {
		state = DONE_STATE;
	    } else if (isspace(curchar)) {
		
	    } else if (isAttChar(curchar)) {
		
		startID = FB_GetPointer(fb) - 1;
		state = GET_ATT_STATE;
	    } else if (curchar == '=') {
		

		state = PRE_VAL_WS_STATE;
	    } else {
		
		*errStr = PR_smprintf("line %d: Bogus character '%c' in tag.\n",
		     					linenum, curchar);
		state = ERR_STATE;
		break;
	    }
	    break;
	case PRE_VAL_WS_STATE:
	    if (curchar == '>') {
		
		*errStr = PR_smprintf(
		    "line %d: End of tag while waiting for value.\n",
		     linenum);
		state = ERR_STATE;
		break;
	    } else if (isspace(curchar)) {
		
		break;
	    } else {
		

		startID = FB_GetPointer(fb) - 1;
		state = GET_VALUE_STATE;
	    }
	    
	case GET_VALUE_STATE:
	    if (isspace(curchar) || curchar == '>') {
		
		curPos = FB_GetPointer(fb) - 2;
		if (curPos >= startID) {
		    
		    FB_GetRange(fb, startID, curPos,
		        &curPair->value);
		    curPair->valueLine = linenum;
		} else {
		    
		}
		if (isspace(curchar)) {
		    state = PRE_ATT_WS_STATE;
		} else {
		    state = DONE_STATE;
		}
	    } else if (curchar == '\"' || curchar == '\'') {
		
		startID = FB_GetPointer(fb);
		state = GET_QUOTED_VAL_STATE;
		PORT_Assert(quotechar == '\0');
		quotechar = curchar; 
	    } else {
		
	    }
	    break;
	case GET_QUOTED_VAL_STATE:
	    PORT_Assert(quotechar != '\0');
	    if (curchar == quotechar) {
		
		curPos = FB_GetPointer(fb) - 2;
		if (curPos >= startID) {
		    
		    FB_GetRange(fb, startID, curPos,
		        &curPair->value);
		    curPair->valueLine = linenum;
		} else {
		    
		}
		state = GET_ATT_STATE;
		quotechar = '\0';
		startID = FB_GetPointer(fb);
	    } else {
		
	    }
	    break;
	case DONE_STATE:
	case ERR_STATE:
	default:
	    ; 
	}
    }

    if (state == DONE_STATE) {
	
	curPos = FB_GetPointer(fb) - 1;
	FB_GetRange(fb, startText, curPos, &ti->text);

	
	return ti;
    }

    
    DestroyTagItem(ti);
    return NULL;
}






static void	
DestroyHTMLItem(HTMLItem *item)
{
    if (item->type == TAG_ITEM) {
	DestroyTagItem(item->item.tag);
    } else {
	if (item->item.text) {
	    PR_Free(item->item.text);
	}
    }
}






static void	
DestroyTagItem(TagItem*ti)
{
    AVPair * temp;

    if (ti->text) {
	PR_Free(ti->text); 
	ti->text = NULL;
    }

    while (ti->attList) {
	temp = ti->attList;
	ti->attList = ti->attList->next;

	if (temp->attribute) {
	    PR_Free(temp->attribute); 
	    temp->attribute = NULL;
	}
	if (temp->value) {
	    PR_Free(temp->value); 
	    temp->value = NULL;
	}
	PR_Free(temp);
    }

    PR_Free(ti);
}






static TAG_TYPE
GetTagType(char *att)
{
    if (!PORT_Strcasecmp(att, "APPLET")) {
	return APPLET_TAG;
    }
    if (!PORT_Strcasecmp(att, "SCRIPT")) {
	return SCRIPT_TAG;
    }
    if (!PORT_Strcasecmp(att, "LINK")) {
	return LINK_TAG;
    }
    if (!PORT_Strcasecmp(att, "STYLE")) {
	return STYLE_TAG;
    }
    return OTHER_TAG;
}






static FileBuffer*
FB_Create(PRFileDesc*fd)
{
    FileBuffer * fb;
    PRInt32 amountRead;
    PRInt32 storedOffset;

    fb = (FileBuffer * ) PR_Malloc(sizeof(FileBuffer));
    fb->fd = fd;
    storedOffset = PR_Seek(fd, 0, PR_SEEK_CUR);
    PR_Seek(fd, 0, PR_SEEK_SET);
    fb->startOffset = 0;
    amountRead = PR_Read(fd, fb->buf, FILE_BUFFER_BUFSIZE);
    if (amountRead == -1) 
	goto loser;
    fb->maxIndex = amountRead - 1;
    fb->curIndex = 0;
    fb->IsEOF = (fb->curIndex > fb->maxIndex) ? PR_TRUE : PR_FALSE;
    fb->lineNum = 1;

    PR_Seek(fd, storedOffset, PR_SEEK_SET);
    return fb;
loser:
    PR_Seek(fd, storedOffset, PR_SEEK_SET);
    PR_Free(fb);
    return NULL;
}






static int	
FB_GetChar(FileBuffer *fb)
{
    PRInt32 storedOffset;
    PRInt32 amountRead;
    int	retval = -1;

    if (fb->IsEOF) {
	return EOF;
    }

    storedOffset = PR_Seek(fb->fd, 0, PR_SEEK_CUR);

    retval = (unsigned char) fb->buf[fb->curIndex++];
    if (retval == '\n') 
	fb->lineNum++;

    if (fb->curIndex > fb->maxIndex) {
	

	fb->startOffset += fb->maxIndex + 1;
	PR_Seek(fb->fd, fb->startOffset, PR_SEEK_SET);
	amountRead = PR_Read(fb->fd, fb->buf, FILE_BUFFER_BUFSIZE);
	if (amountRead == -1)  
	    goto loser;
	fb->maxIndex = amountRead - 1;
	fb->curIndex = 0;
    }

    fb->IsEOF = (fb->curIndex > fb->maxIndex) ? PR_TRUE : PR_FALSE;

loser:
    PR_Seek(fb->fd, storedOffset, PR_SEEK_SET);
    return retval;
}







static unsigned int	
FB_GetLineNum(FileBuffer *fb)
{
    return fb->lineNum;
}







static PRInt32
FB_GetPointer(FileBuffer *fb)
{
    return fb->startOffset + fb->curIndex;
}







static PRInt32
FB_GetRange(FileBuffer *fb, PRInt32 start, PRInt32 end, char **buf)
{
    PRInt32 amountRead;
    PRInt32 storedOffset;

    *buf = PR_Malloc(end - start + 2);
    if (*buf == NULL) {
	return 0;
    }

    storedOffset = PR_Seek(fb->fd, 0, PR_SEEK_CUR);
    PR_Seek(fb->fd, start, PR_SEEK_SET);
    amountRead = PR_Read(fb->fd, *buf, end - start + 1);
    PR_Seek(fb->fd, storedOffset, PR_SEEK_SET);
    if (amountRead == -1) {
	PR_Free(*buf);
	*buf = NULL;
	return 0;
    }

    (*buf)[end-start+1] = '\0';
    return amountRead;
}







static void	
FB_Destroy(FileBuffer *fb)
{
    if (fb) {
	PR_Free(fb);
    }
}







static void	
PrintTagItem(PRFileDesc *fd, TagItem *ti)
{
    AVPair * pair;

    PR_fprintf(fd, "TAG:\n----\nType: ");
    switch (ti->type) {
    case APPLET_TAG:
	PR_fprintf(fd, "applet\n");
	break;
    case SCRIPT_TAG:
	PR_fprintf(fd, "script\n");
	break;
    case LINK_TAG:
	PR_fprintf(fd, "link\n");
	break;
    case STYLE_TAG:
	PR_fprintf(fd, "style\n");
	break;
    case COMMENT_TAG:
	PR_fprintf(fd, "comment\n");
	break;
    case OTHER_TAG:
    default:
	PR_fprintf(fd, "other\n");
	break;
    }

    PR_fprintf(fd, "Attributes:\n");
    for (pair = ti->attList; pair; pair = pair->next) {
	PR_fprintf(fd, "\t%s=%s\n", pair->attribute,
	    pair->value ? pair->value : "");
    }
    PR_fprintf(fd, "Text:%s\n", ti->text ? ti->text : "");

    PR_fprintf(fd, "---End of tag---\n");
}







static void	
PrintHTMLStream(PRFileDesc *fd, HTMLItem *head)
{
    while (head) {
	if (head->type == TAG_ITEM) {
	    PrintTagItem(fd, head->item.tag);
	} else {
	    PR_fprintf(fd, "\nTEXT:\n-----\n%s\n-----\n\n", head->item.text);
	}
	head = head->next;
    }
}







static int	
SaveInlineScript(char *text, char *id, char *basedir, char *archiveDir)
{
    char	*filename = NULL;
    PRFileDesc * fd = NULL;
    int	retval = -1;
    PRInt32 writeLen;
    char	*ilDir = NULL;

    if (!text || !id || !archiveDir) {
	return - 1;
    }

    if (dumpParse) {
	PR_fprintf(outputFD, "SaveInlineScript: text=%s, id=%s, \n"
	    "basedir=%s, archiveDir=%s\n",
	    text, id, basedir, archiveDir);
    }

    
    if (ensureExists(basedir, archiveDir) != PR_SUCCESS) {
	PR_fprintf(errorFD,
	    "ERROR: Unable to create archive directory %s.\n", archiveDir);
	errorCount++;
	return - 1;
    }

    
    ilDir = PR_smprintf("%s/inlineScripts", archiveDir);
    scriptdir = "inlineScripts";
    if (ensureExists(basedir, ilDir) != PR_SUCCESS) {
	PR_fprintf(errorFD,
	    "ERROR: Unable to create directory %s.\n", ilDir);
	errorCount++;
	return - 1;
    }

    filename = PR_smprintf("%s/%s/%s", basedir, ilDir, id);

    
    if (PR_Access(filename, PR_ACCESS_EXISTS) == PR_SUCCESS) {
	PR_fprintf(errorFD,
	    "warning: file \"%s\" already exists--will overwrite.\n",
	     			filename);
	warningCount++;
	if (rm_dash_r(filename)) {
	    PR_fprintf(errorFD, "ERROR: Unable to delete %s.\n", filename);
	    errorCount++;
	    goto finish;
	}
    }

    
    fd = PR_Open(filename, PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, 0777);
    if (!fd) {
	PR_fprintf(errorFD, "ERROR: Unable to create file \"%s\".\n",
	     			filename);
	errorCount++;
	goto finish;
    }
    writeLen = strlen(text);
    if ( PR_Write(fd, text, writeLen) != writeLen) {
	PR_fprintf(errorFD, "ERROR: Unable to write to file \"%s\".\n",
	     			filename);
	errorCount++;
	goto finish;
    }

    retval = 0;
finish:
    if (filename) {
	PR_smprintf_free(filename);
    }
    if (ilDir) {
	PR_smprintf_free(ilDir);
    }
    if (fd) {
	PR_Close(fd);
    }
    return retval;
}







static int	
SaveUnnamableScript(char *text, char *basedir, char *archiveDir,
char *HTMLfilename)
{
    char	*id = NULL;
    char	*ext = NULL;
    char	*start = NULL;
    int	retval = -1;

    if (!text || !archiveDir || !HTMLfilename) {
	return - 1;
    }

    if (dumpParse) {
	PR_fprintf(outputFD, "SaveUnnamableScript: text=%s, basedir=%s,\n"
	    "archiveDir=%s, filename=%s\n", text, basedir, archiveDir,
	     			HTMLfilename);
    }

    
    ext = PL_strrchr(HTMLfilename, '.');
    if (ext) {
	*ext = '\0';
    }
    for (start = HTMLfilename; strpbrk(start, "/\\"); 
         start = strpbrk(start, "/\\") + 1)
	;
    if (*start == '\0') 
	start = HTMLfilename;
    id = PR_smprintf("_%s%d", start, idOrdinal++);
    if (ext) {
	*ext = '.';
    }

    
    retval = SaveInlineScript(text, id, basedir, archiveDir);

    PR_Free(id);

    return retval;
}







static int	
SaveSource(char *src, char *codebase, char *basedir, char *archiveDir)
{
    char	*from = NULL, *to = NULL;
    int	retval = -1;
    char	*arcDir = NULL;

    if (!src || !archiveDir) {
	return - 1;
    }

    if (dumpParse) {
	PR_fprintf(outputFD, "SaveSource: src=%s, codebase=%s, basedir=%s,\n"
	    "archiveDir=%s\n", src, codebase, basedir, archiveDir);
    }

    if (codebase) {
	arcDir = PR_smprintf("%s/%s/%s/", basedir, codebase, archiveDir);
    } else {
	arcDir = PR_smprintf("%s/%s/", basedir, archiveDir);
    }

    if (codebase) {
	from = PR_smprintf("%s/%s/%s", basedir, codebase, src);
	to = PR_smprintf("%s%s", arcDir, src);
    } else {
	from = PR_smprintf("%s/%s", basedir, src);
	to = PR_smprintf("%s%s", arcDir, src);
    }

    if (make_dirs(to, 0777)) {
	PR_fprintf(errorFD,
	    "ERROR: Unable to create archive directory %s.\n", archiveDir);
	errorCount++;
	goto finish;
    }

    retval = copyinto(from, to);
finish:
    if (from) 
	PR_Free(from);
    if (to) 
	PR_Free(to);
    if (arcDir) 
	PR_Free(arcDir);
    return retval;
}







char	*
TagTypeToString(TAG_TYPE type)
{
    switch (type) {
    case APPLET_TAG:
	return "APPLET";
    case SCRIPT_TAG:
	return "SCRIPT";
    case LINK_TAG:
	return "LINK";
    case STYLE_TAG:
	return "STYLE";
    default:
	break;
    }
    return "unknown";
}







static int	
extract_js(char *filename)
{
    PRFileDesc * fd = NULL;
    FileBuffer * fb = NULL;
    HTMLItem * head = NULL;
    HTMLItem * tail = NULL;
    HTMLItem * curitem = NULL;
    HTMLItem * styleList	= NULL;
    HTMLItem * styleListTail	= NULL;
    HTMLItem * entityList	= NULL;
    HTMLItem * entityListTail	= NULL;
    TagItem * tagp = NULL;
    char	*text = NULL;
    char	*tagerr = NULL;
    char	*archiveDir = NULL;
    char	*firstArchiveDir = NULL;
    char	*basedir = NULL;
    PRInt32    textStart;
    PRInt32    curOffset;
    HTML_STATE state;
    int	       curchar;
    int	       retval = -1;
    unsigned int linenum, startLine;

    
    idOrdinal = 0;

    



    fd = PR_Open(filename, PR_RDONLY, 0);
    if (!fd) {
	PR_fprintf(errorFD, "Unable to open %s for reading.\n", filename);
	errorCount++;
	return - 1;
    }

    
     {
	char	*cp;

	basedir = PL_strdup(filename);

	
	while ( (cp = PL_strprbrk(basedir, "/\\")) == 
	    (basedir + strlen(basedir) - 1)) {
	    *cp = '\0';
	}

	

	cp = PL_strprbrk(basedir, "/\\");
	if (cp) {
	    *cp = '\0';
	}
    }

    state = TEXT_HTML_STATE;

    fb = FB_Create(fd);

    textStart = 0;
    startLine = 0;
    while (linenum = FB_GetLineNum(fb), (curchar = FB_GetChar(fb)) !=
        EOF) {
	switch (state) {
	case TEXT_HTML_STATE:
	    if (curchar == '<') {
		


		
		curOffset = FB_GetPointer(fb) - 2;
		if (curOffset >= textStart) {
		    if (FB_GetRange(fb, textStart, curOffset,
		         &text) != 
		        curOffset - textStart + 1)  {
			PR_fprintf(errorFD,
			    "Unable to read from %s.\n",
			     filename);
			errorCount++;
			goto loser;
		    }
		    




		    curitem = CreateTextItem(text, startLine,
		         linenum);
		    text = NULL;
		    if (tail == NULL) {
			head = tail = curitem;
		    } else {
			tail->next = curitem;
			tail = curitem;
		    }
		}

		
		tagp = ProcessTag(fb, &tagerr);
		if (!tagp) {
		    if (tagerr) {
			PR_fprintf(errorFD, "Error in file %s: %s\n",
						  filename, tagerr);
			errorCount++;
		    } else {
			PR_fprintf(errorFD,
			    "Error in file %s, in tag starting at line %d\n",
						  filename, linenum);
			errorCount++;
		    }
		    goto loser;
		}
		
		curitem = CreateTagItem(tagp, linenum, FB_GetLineNum(fb));
		if (tail == NULL) {
		    head = tail = curitem;
		} else {
		    tail->next = curitem;
		    tail = curitem;
		}

		
		if (tagp->type == SCRIPT_TAG) {
		    state = SCRIPT_HTML_STATE;
		}

		
		textStart = FB_GetPointer(fb);
		startLine = FB_GetLineNum(fb);
	    } else {
		
	    }
	    break;
	case SCRIPT_HTML_STATE:
	    if (curchar == '<') {
		char	*cp;
		



		curOffset = FB_GetPointer(fb) - 1;
		cp = NULL;
		if (FB_GetRange(fb, curOffset, curOffset + 8, &cp) != 9) {
		    if (cp) { 
			PR_Free(cp); 
			cp = NULL; 
		    }
		} else {
		    
		    if ( !PORT_Strncasecmp(cp, "</script>", 9) ) {
			
			curOffset--;
			if (curOffset >= textStart) {
			    if (FB_GetRange(fb, textStart, curOffset, &text) != 
			        curOffset - textStart + 1) {
				PR_fprintf(errorFD, "Unable to read from %s.\n",
				     filename);
				errorCount++;
				goto loser;
			    }
			    curitem = CreateTextItem(text, startLine, linenum);
			    text = NULL;
			    if (tail == NULL) {
				head = tail = curitem;
			    } else {
				tail->next = curitem;
				tail = curitem;
			    }
			}

			
			tagp = ProcessTag(fb, &tagerr);
			if (!tagp) {
			    if (tagerr) {
				PR_fprintf(errorFD, "Error in file %s: %s\n",
				     filename, tagerr);
			    } else {
				PR_fprintf(errorFD, 
				    "Error in file %s, in tag starting at"
				    " line %d\n", filename, linenum);
			    }
			    errorCount++;
			    goto loser;
			}
			curitem = CreateTagItem(tagp, linenum,
						FB_GetLineNum(fb));
			if (tail == NULL) {
			    head = tail = curitem;
			} else {
			    tail->next = curitem;
			    tail = curitem;
			}

			
			state = TEXT_HTML_STATE;

			textStart = FB_GetPointer(fb);
			startLine = FB_GetLineNum(fb);
		    }
		}
	    }
	    break;
	}
    }

    
    if (state == SCRIPT_HTML_STATE) {
	if (tail && tail->type == TAG_ITEM) {
	    PR_fprintf(errorFD, "ERROR: <SCRIPT> tag at %s:%d is not followed "
	        "by a </SCRIPT> tag.\n", filename, tail->startLine);
	} else {
	    PR_fprintf(errorFD, "ERROR: <SCRIPT> tag in file %s is not followed"
	        " by a </SCRIPT tag.\n", filename);
	}
	errorCount++;
	goto loser;
    }
    curOffset = FB_GetPointer(fb) - 1;
    if (curOffset >= textStart) {
	text = NULL;
	if ( FB_GetRange(fb, textStart, curOffset, &text) != 
	    curOffset - textStart + 1) {
	    PR_fprintf(errorFD, "Unable to read from %s.\n", filename);
	    errorCount++;
	    goto loser;
	}
	curitem = CreateTextItem(text, startLine, linenum);
	text = NULL;
	if (tail == NULL) {
	    head = tail = curitem;
	} else {
	    tail->next = curitem;
	    tail = curitem;
	}
    }

    if (dumpParse) {
	PrintHTMLStream(outputFD, head);
    }

    


    for (curitem = head; curitem; curitem = curitem->next) {
	TagItem * tagp = NULL;
	AVPair * pairp = NULL;
	char	*src = NULL, *id = NULL, *codebase = NULL;
	PRBool hasEventHandler = PR_FALSE;
	int	i;

	
	if (archiveDir) {
	    PR_Free(archiveDir); 
	    archiveDir = NULL;
	}

	
	if (curitem->type != TAG_ITEM) {
	    continue;
	}

	tagp = curitem->item.tag;

	
	for (pairp = tagp->attList; pairp; pairp = pairp->next) {

	    
	    if ( !PL_strcasecmp(pairp->attribute, "archive")) {
		if (archiveDir) {
		    
		    PR_fprintf(errorFD,
		        "warning: \"%s\" attribute overwrites previous attribute"
		        " in tag starting at %s:%d.\n",
		        pairp->attribute, filename, curitem->startLine);
		    warningCount++;
		    PR_Free(archiveDir);
		}
		archiveDir = PL_strdup(pairp->value);

		
		if ( (PL_strlen(archiveDir) < 4) || 
		    PL_strcasecmp((archiveDir + strlen(archiveDir) -4), 
			".jar")) {
		    PR_fprintf(errorFD,
		        "warning: ARCHIVE attribute should end in \".jar\" in tag"
		        " starting on %s:%d.\n", filename, curitem->startLine);
		    warningCount++;
		    PR_Free(archiveDir);
		    archiveDir = PR_smprintf("%s.arc", archiveDir);
		} else {
		    PL_strcpy(archiveDir + strlen(archiveDir) -4, ".arc");
		}

		

		if (firstArchiveDir == NULL) {
		    firstArchiveDir = PL_strdup(archiveDir);
		}
	    } 
	    
	    else if ( !PL_strcasecmp(pairp->attribute, "codebase")) {
		if (codebase) {
		    
		    PR_fprintf(errorFD,
		        "warning: \"%s\" attribute overwrites previous attribute"
		        " in tag staring at %s:%d.\n",
		        pairp->attribute, filename, curitem->startLine);
		    warningCount++;
		}
		codebase = pairp->value;
	    } 
	    
	    else if ( !PORT_Strcasecmp(pairp->attribute, "src") ||
	        !PORT_Strcasecmp(pairp->attribute, "href") ) {
		if (src) {
		    
		    PR_fprintf(errorFD,
		        "warning: \"%s\" attribute overwrites previous attribute"
		        " in tag staring at %s:%d.\n",
		        pairp->attribute, filename, curitem->startLine);
		    warningCount++;
		}
		src = pairp->value;
	    } 
	    
	    else if (!PORT_Strcasecmp(pairp->attribute, "code") ) {
		
		if (src) {
		    
		    PR_fprintf(errorFD,
		        "warning: \"%s\" attribute overwrites previous attribute"
		        " ,in tag staring at %s:%d.\n",
		        pairp->attribute, filename, curitem->startLine);
		    warningCount++;
		}
		src = pairp->value;

		
		if ( (PL_strlen(src) < 6) || 
		    PL_strcasecmp( (src + PL_strlen(src) - 6), ".class") ) {
		    src = PR_smprintf("%s.class", src);
		    

		    PR_Free(pairp->value);
		    pairp->value = src;
		}
	    } 
	    
	    else if (!PL_strcasecmp(pairp->attribute, "id") ) {
		if (id) {
		    
		    PR_fprintf(errorFD,
		        "warning: \"%s\" attribute overwrites previous attribute"
		        " in tag staring at %s:%d.\n",
		        pairp->attribute, filename, curitem->startLine);
		    warningCount++;
		}
		id = pairp->value;
	    }

	    
	    







	    else if (!PL_strcasecmp(pairp->attribute, "style") && pairp->value) 
	    {
		HTMLItem * styleItem;
		
		styleItem = CreateTextItem(PL_strdup(pairp->value),
		    curitem->startLine, curitem->endLine);
		if (styleListTail == NULL) {
		    styleList = styleListTail = styleItem;
		} else {
		    styleListTail->next = styleItem;
		    styleListTail = styleItem;
		}
	    } 
	    
	    else {
		for (i = 0; i < num_handlers; i++) {
		    if (!PL_strcasecmp(event_handlers[i], pairp->attribute)) {
			hasEventHandler = PR_TRUE;
			break;
		    }
		}
	    }


	    
	    {
		char	*entityStart, *entityEnd;
		HTMLItem * entityItem;

		






		entityEnd = pairp->value;
		while ( entityEnd && 
		    (entityStart = PL_strstr(entityEnd, "&{"))  != NULL) {
		    entityStart += 2; 
		    entityEnd = PL_strchr(entityStart, '}');
		    if (entityEnd) {
			
			*entityEnd = '\0';
			entityItem = CreateTextItem(PL_strdup(entityStart),
					    pairp->valueLine, pairp->valueLine);
			*entityEnd =  '}';
			if (entityListTail) {
			    entityListTail->next = entityItem;
			    entityListTail = entityItem;
			} else {
			    entityList = entityListTail = entityItem;
			}
		    }
		}
	    }
	}

	
	if (!archiveDir && firstArchiveDir) {
	    archiveDir = PL_strdup(firstArchiveDir);
	}

	
	if (hasEventHandler) {
	    if (!id) {
		PR_fprintf(errorFD,
		    "warning: tag starting at %s:%d has event handler but"
		    " no ID attribute.  The tag will not be signed.\n",
					filename, curitem->startLine);
		warningCount++;
	    } else if (!archiveDir) {
		PR_fprintf(errorFD,
		    "warning: tag starting at %s:%d has event handler but"
		    " no ARCHIVE attribute.  The tag will not be signed.\n",
					    filename, curitem->startLine);
		warningCount++;
	    } else {
		if (SaveInlineScript(tagp->text, id, basedir, archiveDir)) {
		    goto loser;
		}
	    }
	}

	switch (tagp->type) {
	case APPLET_TAG:
	    if (!src) {
		PR_fprintf(errorFD,
		    "error: APPLET tag starting on %s:%d has no CODE "
		    "attribute.\n", filename, curitem->startLine);
		errorCount++;
		goto loser;
	    } else if (!archiveDir) {
		PR_fprintf(errorFD,
		    "error: APPLET tag starting on %s:%d has no ARCHIVE "
		    "attribute.\n", filename, curitem->startLine);
		errorCount++;
		goto loser;
	    } else {
		if (SaveSource(src, codebase, basedir, archiveDir)) {
		    goto loser;
		}
	    }
	    break;
	case SCRIPT_TAG:
	case LINK_TAG:
	case STYLE_TAG:
	    if (!archiveDir) {
		PR_fprintf(errorFD,
		    "error: %s tag starting on %s:%d has no ARCHIVE "
		    "attribute.\n", TagTypeToString(tagp->type),
					    filename, curitem->startLine);
		errorCount++;
		goto loser;
	    } else if (src) {
		if (SaveSource(src, codebase, basedir, archiveDir)) {
		    goto loser;
		}
	    } else if (id) {
		
		if (!curitem->next || (curitem->next->type !=
		    TEXT_ITEM)) {
		    PR_fprintf(errorFD,
		        "warning: %s tag starting on %s:%d is not followed"
		        " by script text.\n", TagTypeToString(tagp->type),
					    filename, curitem->startLine);
		    warningCount++;
		    
		    if (SaveInlineScript("", id, basedir, archiveDir)) {
			goto loser;
		    }
		} else {
		    curitem = curitem->next;
		    if (SaveInlineScript(curitem->item.text,
		         id, basedir,
		        archiveDir)) {
			goto loser;
		    }
		}
	    } else {
		
		PR_fprintf(errorFD,
		    "warning: %s tag starting on %s:%d has no SRC or"
		    " ID attributes.  Will not sign.\n",
		    TagTypeToString(tagp->type), filename, curitem->startLine);
		warningCount++;
	    }
	    break;
	default:
	    
	    break;
	}

    }

    
    if (firstArchiveDir) {
	HTMLItem * style, *entity;

	



	style = styleList; 
	entity = entityList;
	while (style || entity) {
	    if (!entity || (style && (style->endLine < entity->endLine))) {
		
		SaveUnnamableScript(style->item.text, basedir, firstArchiveDir,
				    filename);
		style = style->next;
	    } else {
		
		SaveUnnamableScript(entity->item.text, basedir, firstArchiveDir,
				    filename);
		entity = entity->next;
	    }
	}
    }


    retval = 0;
loser:
    
    while (head) {
	curitem = head;
	head = head->next;
	DestroyHTMLItem(curitem);
    }
    while (styleList) {
	curitem = styleList;
	styleList = styleList->next;
	DestroyHTMLItem(curitem);
    }
    while (entityList) {
	curitem = entityList;
	entityList = entityList->next;
	DestroyHTMLItem(curitem);
    }
    if (text) {
	PR_Free(text); 
	text = NULL;
    }
    if (fb) {
	FB_Destroy(fb); 
	fb = NULL;
    }
    if (fd) {
	PR_Close(fd);
    }
    if (tagerr) {
	PR_smprintf_free(tagerr); 
	tagerr = NULL;
    }
    if (archiveDir) {
	PR_Free(archiveDir); 
	archiveDir = NULL;
    }
    if (firstArchiveDir) {
	PR_Free(firstArchiveDir); 
	firstArchiveDir = NULL;
    }
    return retval;
}










static PRStatus
ensureExists (char *base, char *path)
{
    char	fn [FNSIZE];
    PRDir * dir;
    sprintf (fn, "%s/%s", base, path);

    

    if ( (dir = PR_OpenDir(fn)) ) {
	PR_CloseDir(dir);
	return PR_SUCCESS;
    }
    return PR_MkDir(fn, 0777);
}









static int	
make_dirs(char *path, int file_perms)
{
    char	*Path;
    char	*start;
    char	*sep;
    int	ret = 0;
    PRFileInfo info;

    if (!path) {
	return 0;
    }

    Path = PL_strdup(path);
    start = strpbrk(Path, "/\\");
    if (!start) {
	return 0;
    }
    start++; 

    
    while ( (sep = strpbrk(start, "/\\")) ) {
	*sep = '\0';

	if ( PR_GetFileInfo(Path, &info) != PR_SUCCESS) {
	    
	    if ( PR_MkDir(Path, file_perms) != PR_SUCCESS) {
		PR_fprintf(errorFD, "ERROR: Unable to create directory %s.\n",
		     					Path);
		errorCount++;
		ret = -1;
		goto loser;
	    }
	} else {
	    
	    if ( info.type != PR_FILE_DIRECTORY ) {
		PR_fprintf(errorFD, "ERROR: Unable to create directory %s.\n",
		     					Path);
		errorCount++;
		ret = -1;
		goto loser;
	    }
	}

	start = sep + 1; 
	*sep = '/';
    }

loser:
    PR_Free(Path);
    return ret;
}








static int	
copyinto (char *from, char *to)
{
    PRInt32 num;
    char	buf [BUFSIZ];
    PRFileDesc * infp = NULL, *outfp = NULL;
    int	retval = -1;

    if ((infp = PR_Open(from, PR_RDONLY, 0777)) == NULL) {
	PR_fprintf(errorFD, "ERROR: Unable to open \"%s\" for reading.\n",
	     			from);
	errorCount++;
	goto finish;
    }

    
    if (PR_Access(to, PR_ACCESS_EXISTS) == PR_SUCCESS) {
	PR_fprintf(errorFD, "warning: %s already exists--will overwrite\n", to);
	warningCount++;
	if (rm_dash_r(to)) {
	    PR_fprintf(errorFD,
	        "ERROR: Unable to remove %s.\n", to);
	    errorCount++;
	    goto finish;
	}
    }

    if ((outfp = PR_Open(to, PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, 0777))
         == NULL) {
	char	*errBuf = NULL;

	errBuf = PR_Malloc(PR_GetErrorTextLength() + 1);
	PR_fprintf(errorFD, "ERROR: Unable to open \"%s\" for writing.\n", to);
	if (PR_GetErrorText(errBuf)) {
	    PR_fprintf(errorFD, "Cause: %s\n", errBuf);
	}
	if (errBuf) {
	    PR_Free(errBuf);
	}
	errorCount++;
	goto finish;
    }

    while ( (num = PR_Read(infp, buf, BUFSIZ)) > 0) {
	if (PR_Write(outfp, buf, num) != num) {
	    PR_fprintf(errorFD, "ERROR: Error writing to %s.\n", to);
	    errorCount++;
	    goto finish;
	}
    }

    retval = 0;
finish:
    if (infp) 
	PR_Close(infp);
    if (outfp) 
	PR_Close(outfp);

    return retval;
}


