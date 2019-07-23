




































#include "xpidl.h"





static gboolean
doc_prolog(TreeState *state)
{
    fprintf(state->file, "<html>\n");
    fprintf(state->file, "<head>\n");

    fprintf(state->file,
            "<!-- this file is generated from %s.idl -->\n",
            state->basename);
    fprintf(state->file, "<title>documentation for %s.idl interfaces</title>\n",
            state->basename);
    fprintf(state->file, "</head>\n\n");
    fprintf(state->file, "<body>\n");

    return TRUE;
}

static gboolean
doc_epilog(TreeState *state)
{
    fprintf(state->file, "</body>\n");
    fprintf(state->file, "</html>\n");

    return TRUE;
}


static gboolean
doc_list(TreeState *state)
{
    IDL_tree iter;
    for (iter = state->tree; iter; iter = IDL_LIST(iter).next) {
        state->tree = IDL_LIST(iter).data;
        if (!xpidl_process_node(state))
            return FALSE;
    }
    return TRUE;
}

static gboolean
print_list(FILE *outfile, IDL_tree list)
{
    if (list == NULL)
        return TRUE;

    fprintf(outfile, "<ul>\n");
    while (list != NULL) {
        fprintf(outfile, "    <li>%s\n",
                IDL_IDENT(IDL_LIST(list).data).str);
        list = IDL_LIST(list).next;
    }
    fprintf(outfile, "</ul>\n");
    return TRUE;
}

static gboolean
doc_interface(TreeState *state)
{
    IDL_tree iface = state->tree;
    IDL_tree iter;
    IDL_tree orig;
    char *classname = IDL_IDENT(IDL_INTERFACE(iface).ident).str;
    GSList *doc_comments = IDL_IDENT(IDL_INTERFACE(iface).ident).comments;

    fprintf(state->file, "interface %s<br>\n", classname);

    
    





    if (doc_comments != NULL) {
        fprintf(state->file, "doc comments:<br>\n");
        fprintf(state->file, "<pre>\n");
        printlist(state->file, doc_comments);
        fprintf(state->file, "</pre>\n");
        fprintf(state->file, "<br>\n");
    }
    
    
    



    if ((iter = IDL_INTERFACE(iface).inheritance_spec)) {
        fprintf(state->file, "%s inherits from:<br>\n", classname);
        print_list(state->file, iter);
        fprintf(state->file, "<br>\n");
    }

    






    orig = state->tree;
    state->tree = IDL_INTERFACE(iface).body;
    if (state->tree && !xpidl_process_node(state))
        return FALSE;
    state->tree = orig;

    return TRUE;
}






static gboolean
write_type(IDL_tree type_tree, FILE *outfile)
{
    if (!type_tree) {
        fputs("void", outfile);
        return TRUE;
    }

    switch (IDL_NODE_TYPE(type_tree)) {
      case IDLN_TYPE_INTEGER: {
        gboolean sign = IDL_TYPE_INTEGER(type_tree).f_signed;
        switch (IDL_TYPE_INTEGER(type_tree).f_type) {
          case IDL_INTEGER_TYPE_SHORT:
            fputs(sign ? "PRInt16" : "PRUint16", outfile);
            break;
          case IDL_INTEGER_TYPE_LONG:
            fputs(sign ? "PRInt32" : "PRUint32", outfile);
            break;
          case IDL_INTEGER_TYPE_LONGLONG:
            fputs(sign ? "PRInt64" : "PRUint64", outfile);
            break;
          default:
            g_error("Unknown integer type %d\n",
                    IDL_TYPE_INTEGER(type_tree).f_type);
            return FALSE;
        }
        break;
      }
      case IDLN_TYPE_CHAR:
        fputs("char", outfile);
        break;
      case IDLN_TYPE_WIDE_CHAR:
        fputs("PRUnichar", outfile); 
        break;
      case IDLN_TYPE_WIDE_STRING:
        fputs("PRUnichar *", outfile);
        break;
      case IDLN_TYPE_STRING:
        fputs("char *", outfile);
        break;
      case IDLN_TYPE_BOOLEAN:
        fputs("PRBool", outfile);
        break;
      case IDLN_TYPE_OCTET:
        fputs("PRUint8", outfile);
        break;
      case IDLN_TYPE_FLOAT:
        switch (IDL_TYPE_FLOAT(type_tree).f_type) {
          case IDL_FLOAT_TYPE_FLOAT:
            fputs("float", outfile);
            break;
          case IDL_FLOAT_TYPE_DOUBLE:
            fputs("double", outfile);
            break;
          
          default:
            fprintf(outfile, "unknown_type_%d", IDL_NODE_TYPE(type_tree));
            break;
        }
        break;
      case IDLN_IDENT:
        if (UP_IS_NATIVE(type_tree)) {
            fputs(IDL_NATIVE(IDL_NODE_UP(type_tree)).user_type, outfile);
            if (IDL_tree_property_get(type_tree, "ptr")) {
                fputs(" *", outfile);
            } else if (IDL_tree_property_get(type_tree, "ref")) {
                fputs(" &", outfile);
            }
        } else {
            fputs(IDL_IDENT(type_tree).str, outfile);
        }
        if (UP_IS_AGGREGATE(type_tree))
            fputs(" *", outfile);
        break;
      default:
        fprintf(outfile, "unknown_type_%d", IDL_NODE_TYPE(type_tree));
        break;
    }
    return TRUE;
}


static gboolean
doc_attribute_declaration(TreeState *state)
{
    IDL_tree attr = state->tree;

    if (!verify_attribute_declaration(attr))
        return FALSE;
    




    if (IDL_ATTR_DCL(attr).f_readonly)
        fprintf(state->file, "readonly ");

    fprintf(state->file, "attribute ");

    if (!write_type(IDL_ATTR_DCL(attr).param_type_spec, state->file))
        return FALSE;

    fprintf(state->file, "\n");
    print_list(state->file, IDL_ATTR_DCL(attr).simple_declarations);
    fprintf(state->file, "<br>\n");

    return TRUE;
}


static gboolean
doc_method_declaration(TreeState *state)
{
    



    





    fprintf(state->file,
            "method %s<br>\n",
            IDL_IDENT(IDL_OP_DCL(state->tree).ident).str);

    return TRUE;
}

backend *
xpidl_doc_dispatch(void)
{
    static backend result;
    static nodeHandler table[IDLN_LAST];
    static gboolean initialized = FALSE;

    result.emit_prolog = doc_prolog;
    result.emit_epilog = doc_epilog;

    if (!initialized) {
        

        

        table[IDLN_LIST] = doc_list;
        table[IDLN_INTERFACE] = doc_interface;
        table[IDLN_ATTR_DCL] = doc_attribute_declaration;
        table[IDLN_OP_DCL] = doc_method_declaration;

        initialized = TRUE;
    }
  
    result.dispatch_table = table;
    return &result;
}
