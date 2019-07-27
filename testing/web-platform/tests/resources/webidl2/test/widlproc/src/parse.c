














#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "comment.h"
#include "lex.h"
#include "misc.h"
#include "node.h"
#include "parse.h"




static void
tokerrorexit(struct tok *tok, const char *format, ...)
{
    va_list ap;
    char *m;
    va_start(ap, format);
    m = vmemprintf(format, ap);
    if (tok->type == TOK_EOF)
        locerrorexit(tok->filename, tok->linenum, "at end of input: %s", m);
    else
        locerrorexit(tok->filename, tok->linenum, "at '%.*s': %s", tok->len, tok->start, m);
    va_end(ap);
}













static struct tok *
lexnocomment(void)
{
    struct tok *tok;
    for (;;) {
        tok = lex();
        if (tok->type != TOK_BLOCKCOMMENT && tok->type != TOK_INLINECOMMENT)
            break;
        addcomment(tok);
    }
    return tok;
}









static void
eat(struct tok *tok, int type)
{
    if (tok->type != type) {
        const char *p;
        if (type < TOK_DOMString)
            tokerrorexit(tok, "expected '%c'", type);
        p = keywords;
        while (type != TOK_DOMString) {
            p += strlen(p) + 1;
            type--;
        }
        tokerrorexit(tok, "expected '%s'", p);
    }
    lexnocomment();
}










static void
setid(struct node *node)
{
    node->id = getattr(node, "name");
}








static char *
setidentifier(struct tok *tok)
{
    char *s;
    if (tok->type != TOK_IDENTIFIER)
        tokerrorexit(tok, "expected identifier");
    s = memprintf("%.*s", tok->len, tok->start);
    return s;
}








static char *
setargumentname(struct tok *tok)
{
    char *s;
    if (tok->type != TOK_IDENTIFIER && tok->type < TOK_attribute)
        tokerrorexit(tok, "expected argument name");
    s = memprintf("%.*s", tok->len, tok->start);
    return s;
}



static struct node *parsetype(struct tok *tok);
static struct node *parsedefaultvalue(struct tok *tok, struct node *parent);
static struct node *parseuniontype(struct tok *tok);
static struct node *parseargumentlist(struct tok *tok);
static void parsedefinitions(struct tok *tok, struct node *parent);
static struct node *parsetypesuffixstartingwitharray(struct tok *tok, struct node *node);












static struct node *
parsescopedname(struct tok *tok, const char *name, int ref)
{
    const char *start = tok->start, *end;
    struct node *node;
    unsigned int len = 0;
    char *s = memalloc(3);
    if (tok->type != TOK_IDENTIFIER)
        tokerrorexit(tok, "expected identifier");
    s = memrealloc(s, len + tok->len + 1);
    memcpy(s + len, tok->start, tok->len);
    len += tok->len;
    end = tok->start + tok->len;
    lexnocomment();
    s[len] = 0;
    node = newattr(name, s);
    if (ref) {
        node->start = start;
        node->end = end;
    }
    return node;
}












static struct node *
parsescopednamelist(struct tok *tok, const char *name, const char *name2,
        int comment)
{
    struct node *node = newelement(name);
    for (;;) {
        struct node *attr = parsescopedname(tok, "name", 1);
        struct node *n = newelement(name2);
        if (comment)
            setcommentnode(n);
        addnode(n, attr);
        addnode(node, n);
        if (tok->type != ',')
            break;
        lexnocomment();
    }
    return node;
}









static struct node *
parsereturntype(struct tok *tok)
{
    if (tok->type == TOK_void) {
        struct node *node = newelement("Type");
        addnode(node, newattr("type", "void"));
        lexnocomment();
        return node;
    }
    return parsetype(tok);
}









static const char *
parseunsignedintegertype(struct tok *tok)
{
    static const char *names[] = {
        "short", "long", "long long", 0,
        "unsigned short", "unsigned long", "unsigned long long" };
    enum { TYPE_SHORT, TYPE_LONG, TYPE_LONGLONG,
           TYPE_UNSIGNED = 4 };
    int type = 0;
    if (tok->type == TOK_unsigned) {
        type = TYPE_UNSIGNED;
        lexnocomment();
    }
    if (tok->type == TOK_short) {
        type |= TYPE_SHORT;
        lexnocomment();
    } else if (tok->type != TOK_long)
        tokerrorexit(tok, "expected 'short' or 'long' after 'unsigned'");
    else {
        type |= TYPE_LONG;
        lexnocomment();
        if (tok->type == TOK_long) {
            type += TYPE_LONGLONG - TYPE_LONG;
            lexnocomment();
        }
    }
    return names[type];
}








static struct node *
parsetypesuffix(struct tok *tok, struct node *node)
{
    if (tok->type == TOK_DOUBLEBRACKET) {
        struct node *typenode = node;
	node = newelement("Type");
        addnode(node, newattr("type", "array"));
        addnode(node, typenode);
        lexnocomment();
	node = parsetypesuffix(tok, node);
    } else if (tok->type == '?') {
        addnode(node, newattr("nullable", "nullable"));
        lexnocomment();
	node = parsetypesuffixstartingwitharray(tok, node);
    }
    return node;
}









static struct node *
parsetypesuffixstartingwitharray(struct tok *tok,  struct node *node)
{
    if (tok->type == TOK_DOUBLEBRACKET) {
        struct node *typenode = node;
	node = newelement("Type");
        addnode(node, newattr("type", "array"));
        addnode(node, typenode);
        lexnocomment();
	node = parsetypesuffix(tok, node);
    }
    return node;
}









static struct node *
parseprimitiveorstringtype(struct tok *tok)
{
  struct node *node;
    switch (tok->type) {
    case TOK_unsigned:
    case TOK_short:
    case TOK_long:
        node = newelement("Type");
        addnode(node, newattr("type", parseunsignedintegertype(tok)));
        break;
    default:
        node = newelement("Type");
        switch (tok->type) {
        default:
            tokerrorexit(tok, "expected type");
            break;
	case TOK_unrestricted:
	  lexnocomment();
	  if (tok->type == TOK_float) {
            addnode(node, newattr("type", "unrestricted float"));
	  } else if (tok->type == TOK_double) {
            addnode(node, newattr("type", "unrestricted double"));
	  } else {
            tokerrorexit(tok, "expected float or double after unrestricted");
	  }
	  break;
        case TOK_boolean:
            addnode(node, newattr("type", "boolean"));
            break;
        case TOK_byte:
            addnode(node, newattr("type", "byte"));
            break;
        case TOK_octet:
            addnode(node, newattr("type", "octet"));
            break;
        case TOK_float:
            addnode(node, newattr("type", "float"));
            break;
        case TOK_double:
            addnode(node, newattr("type", "double"));
            break;
        case TOK_DOMString:
            addnode(node, newattr("type", "DOMString"));
            break;
        case TOK_ByteString:
            addnode(node, newattr("type", "ByteString"));
            break;
        case TOK_Date:
            addnode(node, newattr("type", "Date"));
            break;
        case TOK_RegExp:
            addnode(node, newattr("type", "RegExp"));
            break;

        }
        lexnocomment();
    }
    return node;
}









static struct node *
parsenonanytype(struct tok *tok)
{
    struct node *node;
    switch (tok->type) {
    case TOK_IDENTIFIER:
        node = newelement("Type");
        addnode(node, parsescopedname(tok, "name", 1));
	node = parsetypesuffix(tok, node);
        break;
    case TOK_sequence:
        node = newelement("Type");
        addnode(node, newattr("type", "sequence"));
        lexnocomment();
        eat(tok, '<');
        addnode(node, parsetype(tok));
        eat(tok, '>');
	if (tok->type == '?') {
	  addnode(node, newattr("nullable", "nullable"));
	  lexnocomment();
	}
        break;
    case TOK_object:
        node = newelement("Type");
        addnode(node, newattr("type", "object"));
        lexnocomment();
	node = parsetypesuffix(tok, node);
        break;
    default:
        node = parseprimitiveorstringtype(tok);
        node = parsetypesuffix(tok, node);
        break;
    }       
    return node;
}









static struct node *
parseunionmembertype(struct tok *tok)
{
  struct node *node;
  if (tok->type == TOK_any) {
    struct node *typenode = newelement("Type");
    addnode(typenode, newattr("type", "any"));
    lexnocomment();
    eat(tok, TOK_DOUBLEBRACKET);
    node = newelement("Type");
    addnode(node, newattr("type", "array"));
    addnode(node, typenode);
    lexnocomment();
    node = parsetypesuffix(tok, node);
  } else if (tok->type == '(') { 
    node = parseuniontype(tok);
  } else {
    node = parsenonanytype(tok);
  }
  return node;
}










static struct node *
parseuniontype(struct tok *tok)
{
  struct node *node;
  eat(tok, '(');
  node = newelement("Type");
  addnode(node, newattr("type", "union"));
  if (tok->type != ')') {
    for (;;) {
      addnode(node, parseunionmembertype(tok));
      if (tok->type != TOK_or)
	break;
      lexnocomment();
    }
  }
  eat(tok, ')');
  node = parsetypesuffix(tok, node);      
  return node;
}









static struct node *
parsetype(struct tok *tok)
{
    struct node *node;
    if (tok->type == '(') {
      node = parseuniontype(tok);
    } else if (tok->type == TOK_any) {
      node = newelement("Type");
      addnode(node, newattr("type", "any"));
      lexnocomment();
      node = parsetypesuffixstartingwitharray(tok, node);
    } else {
      node = parsenonanytype(tok);	
    }
    return node;
}















static struct node *
parseextendedattribute(struct tok *tok)
{
	const char *start ;
    struct node *node = newelement("ExtendedAttribute");
    char *attrname = setidentifier(tok);
    addnode(node, newattr("name", attrname));
    start = tok->prestart;
    node->wsstart = start;
    node->end = tok->start + tok->len;
    if(!strcmp(attrname, "Constructor") || !strcmp(attrname, "NamedConstructor")) {
	    setcommentnode(node);
	}
    lexnocomment();
    if (tok->type == '=') {
        lexnocomment();
        addnode(node, parsescopedname(tok, "value", 0));
    }
    if (tok->type == '(') {
        lexnocomment();
        addnode(node, parseargumentlist(tok));
	    node->end = tok->start + tok->len;
        eat(tok, ')');
    }
    return node;
}









static struct node *
parseextendedattributelist(struct tok *tok)
{
    struct node *node;
    if (tok->type != '[')
        return 0;
    node = newelement("ExtendedAttributeList");
    for (;;) {
        lexnocomment();
        addnode(node, parseextendedattribute(tok));
        if (tok->type != ',')
            break;
    }
    if (tok->type != ']')
        tokerrorexit(tok, "expected ',' or ']'");
    lexnocomment();
    return node;
}










static struct node *
parseexceptionfield(struct tok *tok, struct node *eal)
{
    struct node *node = newelement("ExceptionField");
    if (eal) addnode(node, eal);
    setcommentnode(node);
    addnode(node, parsetype(tok));
    addnode(node, newattr("name", setidentifier(tok)));
    tok = lexnocomment();
    return node;
}










static struct node *
parseargument(struct tok *tok)
{
    struct node *node = newelement("Argument");
    struct node *eal = parseextendedattributelist(tok);
    setcommentnode(node);
    if (eal) addnode(node, eal);
    if (tok->type == TOK_optional) {
        addnode(node, newattr("optional", "optional"));
        lexnocomment();
    }
    addnode(node, parsetype(tok));
    if (tok->type == TOK_ELLIPSIS) {
        addnode(node, newattr("ellipsis", "ellipsis"));
        lexnocomment();
    }
    addnode(node, newattr("name", setargumentname(tok)));
    lexnocomment();
    
    if (tok->type == '=') {
      tok = lexnocomment();
      node = parsedefaultvalue(tok, node);
    }
    return node;
}









static struct node *
parseargumentlist(struct tok *tok)
{
    struct node *node = newelement("ArgumentList");
    
    if (tok->type != ')') {
        for (;;) {
            addnode(node, parseargument(tok));
            if (tok->type != ',')
                break;
            lexnocomment();
        }
    }
    return node;
}











static struct node *
parseoperationrest(struct tok *tok, struct node *node)
{
    if (tok->type == TOK_IDENTIFIER) {
        addnode(node, newattr("name", setidentifier(tok)));
        lexnocomment();
    }
    eat(tok, '(');
    addnode(node, parseargumentlist(tok));
    eat(tok, ')');
    return node;
}









static struct node *
parsereturntypeandoperationrest(struct tok *tok, struct node *eal, struct node *attrs) 
{
  struct node *node =  newelement("Operation");
  struct node *nodeType = parsereturntype(tok);
  if (eal) addnode(node, eal);
  setcommentnode(node);
  addnode(node, attrs);
  addnode(node, nodeType);
  return parseoperationrest(tok, node);
}










static struct node *
parseoptionaliteratorinterface(struct tok *tok, struct node *node)
{
  if (tok->type == '=') {
    lexnocomment();
    addnode(node, newattr("interface", setidentifier(tok)));
    lexnocomment();
  }
  return node;
}











static struct node *
parseoperationoriteratorrest(struct tok *tok, struct node *eal, struct node *attrs)
{
  struct node *node;
  struct node *nodeType = parsereturntype(tok);
  unsigned int isIterator = 0;
  if (tok->type == TOK_iterator) {
    lexnocomment();
    if (tok->type == TOK_object) {
      lexnocomment();
      node = newelement("IteratorObject");
      addnode(node, nodeType);
      return node;
    } else {
      node = newelement("Iterator");
      isIterator = 1;    
    }
  } else {
    node = newelement("Operation");
  }
  if (eal) addnode(node, eal);
  setcommentnode(node);
  addnode(node, attrs);
  addnode(node, nodeType);
  if (isIterator)
    return parseoptionaliteratorinterface(tok, node);
  else
    return parseoperationrest(tok, node);
}












static struct node *
parseattribute(struct tok *tok, struct node *eal, struct node *attrs)
{
    struct node *node = newelement("Attribute");
    if (eal) addnode(node, eal);
    setcommentnode(node);
    addnode(node, attrs);
    if (tok->type == TOK_inherit) {
        lexnocomment();
	addnode(node, newattr("inherit", "inherit"));
    }
    if (tok->type == TOK_readonly) {
        lexnocomment();
        addnode(node, newattr("readonly", "readonly"));
    }
    eat(tok, TOK_attribute);
    addnode(node, parsetype(tok));
    addnode(node, newattr("name", setidentifier(tok)));
    lexnocomment();
    return node;
}










static struct node *
parseserializer (struct tok *tok, struct node *eal) {
	struct node *nodeAttribute;
	struct node *node = newelement("Serializer");
  if (tok->type == '=') {
    if (eal) addnode(node, eal);
    lexnocomment();
    if (tok->type == TOK_IDENTIFIER) {
      addnode(node, newattr("attribute", setidentifier(tok)));
      lexnocomment();
    } else if (tok->type == '{') {
      unsigned int done = 0;
      struct node *nodeMap = newelement("Map");
      lexnocomment();
      if (tok->type == TOK_getter) {
	addnode(nodeMap, newattr("pattern", "getter"));
	done = 1;
      } else if (tok->type == TOK_attribute) {
	addnode(nodeMap, newattr("pattern", "all"));
	done = 1;
      } else if (tok->type == TOK_inherit) {
	addnode(nodeMap, newattr("inherit", "inherit"));
	lexnocomment();
	eat(tok, ',');
	if (tok->type == TOK_attribute) {
	  addnode(nodeMap, newattr("pattern", "all"));
	  done = 1;
	}
      } else if (tok->type != TOK_IDENTIFIER) {
	tokerrorexit(tok, "expected 'attribute', 'getter', 'inherit' or attribute identifiers in serializer map");
      }
      if (done) {
	lexnocomment();
	eat(tok, '}');
      } else {
	addnode(nodeMap, newattr("pattern", "selection"));
	do {
	  if (tok->type != TOK_IDENTIFIER)
	    tokerrorexit(tok, "expected attribute identifiers in serializer map %s", tok->prestart);
	  nodeAttribute = newelement("PatternAttribute");
	  addnode(nodeAttribute, newattr("name", setidentifier(tok)));
	  addnode(nodeMap, nodeAttribute);
	  lexnocomment();
	  if (tok->type == ',')
	    lexnocomment();
	} while (tok->type != '}');
	eat(tok, '}');
      }
      addnode(node, nodeMap);
    } else if (tok->type == '[') {
      struct node *nodeList = newelement("List");
      lexnocomment();
      if (tok->type == TOK_getter) {
	addnode(nodeList, newattr("pattern", "getter"));
	lexnocomment();
	eat(tok, ']');
      } else {
	addnode(nodeList, newattr("pattern", "selection"));
	do {
	  if (tok->type != TOK_IDENTIFIER)
	    tokerrorexit(tok, "expected attribute identifiers in serializer list");
	  nodeAttribute = newelement("PatternAttribute");
	  addnode(nodeAttribute, newattr("name", setidentifier(tok)));
	  addnode(nodeList, nodeAttribute);
	  lexnocomment();
	  if (tok->type == ',')
	    lexnocomment();
	} while (tok->type != ']');	    
	eat(tok, ']');
      }
      addnode(node, nodeList);
    } else {
      tokerrorexit(tok, "Expected '{', '[' or an attribute identifier in the serializer declaration");
    }
    return node;
  } else {
    if (eal) addnode(node, eal);
    return node;
  }
}










static struct node *
parseattributeoroperationoriterator(struct tok *tok, struct node *eal)
{
	int alreadyseen ;
    struct node *attrs = newattrlist();
    if (tok->type == TOK_serializer) {
      lexnocomment();
      if (tok->type == '=' || tok->type ==';') {
	return parseserializer(tok, eal);
      } else {
	addnode(attrs, newattr("serializer", "serializer"));
	return parsereturntypeandoperationrest(tok, eal, attrs);
      }
    }
    if (tok->type == TOK_stringifier) {
        addnode(attrs, newattr("stringifier", "stringifier"));
        lexnocomment();
        if (tok->type == ';') {
            struct node *node = newelement("Stringifier");
            if (eal) addnode(node, eal);
            return node;
        }
    }
    if (tok->type == TOK_static) {
        lexnocomment();
        addnode(attrs, newattr("static", "static"));
    }
    if (tok->type == TOK_inherit || tok->type == TOK_readonly || tok->type == TOK_attribute)
        return parseattribute(tok, eal, attrs);
    if (!nodeisempty(attrs))
 	return parsereturntypeandoperationrest(tok, eal, attrs);
    alreadyseen = 0;
    for (;;) {
      static const int t[] = { TOK_getter,
			       TOK_setter, TOK_creator, TOK_deleter, TOK_legacycaller,
			       0 };
      const int *tt = t;
      char *s;
      while (*tt && *tt != tok->type)
	tt++;
      if (!*tt)
	break;
      s = memprintf("%.*s", tok->len, tok->start);
      if (alreadyseen & (1 << (tt - t)))
	tokerrorexit(tok, "'%s' qualifier cannot be repeated", s);
      alreadyseen |= 1 << (tt - t);
      addnode(attrs, newattr(s, s));
      lexnocomment();
    }
    if (!nodeisempty(attrs))    
      return parsereturntypeandoperationrest(tok, eal, attrs);
    else
      return parseoperationoriteratorrest(tok, eal, attrs);
}











static struct node *
parseconstexpr (struct tok *tok, struct node *node) {
  char *s;
  switch(tok->type) {
  case TOK_true:
  case TOK_false:
  case TOK_minusinfinity:
  case TOK_INTEGER:
  case TOK_FLOAT:
  case TOK_null:
  case TOK_infinity:
  case TOK_NaN:
    break;
  default:
    tokerrorexit(tok, "expected constant value");
    break;
  }
  s = memalloc(tok->len + 1);
  memcpy(s, tok->start, tok->len);
  s[tok->len] = 0;
  if (tok->type != TOK_STRING) {
    addnode(node, newattr("value", s));
  } else {
    addnode(node, newattr("stringvalue", s));
  }
  lexnocomment();
  return node;
}










static struct node *
parsedefaultvalue (struct tok *tok, struct node *node) {
  char *s;
  if (tok->type == TOK_STRING) {
    s = memalloc(tok->len + 1);
    memcpy(s, tok->start, tok->len);
    s[tok->len] = 0;
    addnode(node, newattr("stringvalue", s));
    lexnocomment();
    return node;
  } else {
    return parseconstexpr(tok, node);
  }
}












static struct node *
parsedictionarymember(struct tok *tok, struct node *eal)
{
    struct node *node = newelement("DictionaryMember");
    if (eal) addnode(node, eal);
    setcommentnode(node);
    addnode(node, parsetype(tok));
    addnode(node, newattr("name", setidentifier(tok)));
    tok = lexnocomment();
    
    if (tok->type == '=') {
      tok = lexnocomment();
      node = parsedefaultvalue(tok, node);
    }
    return node;
}










static struct node *
parseconst(struct tok *tok, struct node *eal)
{
    struct node *node = newelement("Const");
    setcommentnode(node);
    if (eal) addnode(node, eal);
    tok = lexnocomment();
    switch(tok->type) {
    case TOK_boolean:
    case TOK_byte:
    case TOK_octet:
    case TOK_float:
    case TOK_double:
    case TOK_unsigned:
    case TOK_unrestricted:
    case TOK_short:
    case TOK_long:
        addnode(node, parsetype(tok));
	break;
    default:
        tokerrorexit(tok, "expected acceptable constant type");
        break;
    }
    addnode(node, newattr("name", setidentifier(tok)));
    tok = lexnocomment();
    eat(tok, '=');
    node = parseconstexpr(tok, node);
    return node;
}










static struct node *
parseimplementsstatement(struct tok *tok, struct node *eal)
{
    struct node *node = newelement("Implements");
    setcommentnode(node);
    if (eal) addnode(node, eal);
    addnode(node, parsescopedname(tok, "name1", 1));
    eat(tok, TOK_implements);
    addnode(node, parsescopedname(tok, "name2", 1));
    return node;
}










static struct node *
parsetypedef(struct tok *tok, struct node *eal)
{
struct node *ealtype;
struct node *typenode;
    struct node *node = newelement("Typedef");
    setcommentnode(node);
    if (eal) addnode(node, eal);
    tok = lexnocomment();
    ealtype = parseextendedattributelist(tok);
    typenode = parsetype(tok);
    if (ealtype) addnode(typenode, ealtype);
    addnode(node, typenode);
    addnode(node, newattr("name", setidentifier(tok)));
    tok = lexnocomment();
    return node;
}










static struct node *
parseexception(struct tok *tok, struct node *eal)
{
    struct node *node = newelement("Exception");
    setcommentnode(node);
    if (eal) addnode(node, eal);
    tok = lexnocomment();
    addnode(node, newattr("name", setidentifier(tok)));
    lexnocomment();
    if (tok->type == ':') {
        lexnocomment();
        addnode(node, parsescopednamelist(tok, "ExceptionInheritance", "Name", 1));
    }
    eat(tok, '{');
    while (tok->type != '}') {
        const char *start = tok->prestart;
        struct node *node2;
        struct node *eal = parseextendedattributelist(tok);
        if (tok->type == TOK_const)
            node2 = parseconst(tok, eal);
        else
            node2 = parseexceptionfield(tok, eal);
        addnode(node, node2);
        node2->wsstart = start;
        node2->end = tok->start + tok->len;
        setid(node2);
        eat(tok, ';');
    }
    lexnocomment();
    return node;
}










static struct node *
parseinterface(struct tok *tok, struct node *eal)
{
    struct node *node = newelement("Interface");
    if (eal) addnode(node, eal);
    setcommentnode(node);
    tok = lexnocomment();
    addnode(node, newattr("name", setidentifier(tok)));
    tok = lexnocomment();
    if (tok->type == ':') {
        lexnocomment();
        addnode(node, parsescopednamelist(tok, "InterfaceInheritance", "Name", 1));
    }
    eat(tok, '{');
    while (tok->type != '}') {
        const char *start = tok->prestart;
        struct node *eal = parseextendedattributelist(tok);
        struct node *node2;
        if (tok->type == TOK_const)
            addnode(node, node2 = parseconst(tok, eal));
        else
            addnode(node, node2 = parseattributeoroperationoriterator(tok, eal));
        node2->wsstart = start;
        node2->end = tok->start + tok->len;
        setid(node2);
        eat(tok, ';');
    }
    lexnocomment();
    return node;
}










static struct node *
parsecallback(struct tok *tok, struct node *eal)
{
  struct node *node;
  if (tok->type == TOK_interface) {
    node = parseinterface(tok, eal);
    addnode(node, newattr("callback", "callback"));    
  } else {
    node = newelement("Callback");
    if (eal) addnode(node, eal);
    setcommentnode(node);
    addnode(node, newattr("name", setidentifier(tok)));
    tok = lexnocomment();
    eat(tok, '=');
    addnode(node, parsereturntype(tok));
    eat(tok, '(');
    addnode(node, parseargumentlist(tok));
    eat(tok, ')');
  }
  return node;
}










static struct node *
parsedictionary(struct tok *tok, struct node *eal)
{
    struct node *node = newelement("Dictionary");
    if (eal) addnode(node, eal);
    setcommentnode(node);
    tok = lexnocomment();
    addnode(node, newattr("name", setidentifier(tok)));
    tok = lexnocomment();
    if (tok->type == ':') {
        lexnocomment();
        addnode(node, parsescopednamelist(tok, "DictionaryInheritance", "Name", 1));
    }
    eat(tok, '{');
    while (tok->type != '}') {
        const char *start = tok->prestart;
        struct node *eal = parseextendedattributelist(tok);
        struct node *node2;
        if (tok->type == TOK_const)
            addnode(node, node2 = parseconst(tok, eal));
        else
            addnode(node, node2 = parsedictionarymember(tok, eal));
        node2->wsstart = start;
        node2->end = tok->start + tok->len;
        setid(node2);
        eat(tok, ';');
    }
    lexnocomment();
    return node;
}










static struct node *
parseenum(struct tok *tok, struct node *eal)
{
	char *s;
    struct node *node = newelement("Enum");
    if (eal) addnode(node, eal);
    setcommentnode(node);
    tok = lexnocomment();
    addnode(node, newattr("name", setidentifier(tok)));
    tok = lexnocomment();
    eat(tok, '{');
    while (tok->type != '}') {
      if (tok->type == TOK_STRING) {
	const char *start = tok->prestart;
	struct node *node2 = newelement("EnumValue");
	setcommentnode(node2);
	
	s = memalloc(tok->len + 1);
	memcpy(s, tok->start, tok->len);
	s[tok->len] = 0;
	addnode(node2, newattr("stringvalue", s));
        node2->wsstart = start;
        node2->end = tok->start + tok->len;
        setid(node2);
	addnode(node, node2);
      } else {
	tokerrorexit(tok, "expected string in enum");
      }
      lexnocomment();
      if (tok->type == ',') {
	lexnocomment();
      }
    }
    eat(tok, '}');
    return node;
}









static void
parsedefinitions(struct tok *tok, struct node *parent)
{
    parent->wsstart = tok->prestart;
    for (;;) {
        const char *wsstart = tok->prestart;
        struct node *eal = parseextendedattributelist(tok);
        struct node *node;
        switch (tok->type) {
        case TOK_partial:
	    eat(tok, TOK_partial);
	    if (tok->type == TOK_dictionary) {
	      node = parsedictionary(tok, eal);
	    } else {
	      node = parseinterface(tok, eal);
	    }
	    addnode(node, newattr("partial", "partial"));
            break;
        case TOK_interface:
  	    node = parseinterface(tok, eal);
            break;
	case TOK_callback:
	    eat(tok, TOK_callback);
	    node = parsecallback(tok, eal);
            break;
	case TOK_dictionary:
            node = parsedictionary(tok, eal);
            break;	  
	case TOK_enum:
            node = parseenum(tok, eal);
            break;	  
        case TOK_exception:
            node = parseexception(tok, eal);
            break;
        case TOK_typedef:
            node = parsetypedef(tok, eal);
            break;
        case TOK_IDENTIFIER:
            node = parseimplementsstatement(tok, eal);
            break;
        default:
            if (eal)
                tokerrorexit(tok, "expected definition after extended attribute list");
            node = 0;
            break;
        }
        if (!node)
            break;
        node->wsstart = wsstart;
        node->end = tok->start + tok->len;
        eat(tok, ';');
        addnode(parent, node);
        setid(node);
        parent->end = node->end;
    }
}






struct node *
parse(void)
{
	struct tok *tok; 
    struct node *root = newelement("Definitions");
    setcommentnode(root);
    tok = lexnocomment();
    parsedefinitions(tok, root);
    if (tok->type != TOK_EOF)
        tokerrorexit(tok, "expected end of input");
    reversechildren(root);
    return root;
}

