






































 

#include "xpidl.h"

#ifdef XP_WIN
#define strdup _strdup
#endif

 char OOM[] = "ERROR: out of memory\n";

void *
xpidl_malloc(size_t nbytes)
{
    void *p = malloc(nbytes);
    if (!p) {
        fputs(OOM, stderr);
        exit(1);
    }
    return p;
}

char *
xpidl_strdup(const char *s)
{
    char *ns = strdup(s);
    if (!ns) {
        fputs(OOM, stderr);
        exit(1);
    }
    return ns;
}

void
xpidl_write_comment(TreeState *state, int indent)
{
    fprintf(state->file, "%*s/* ", indent, "");
    IDL_tree_to_IDL(state->tree, state->ns, state->file,
                    IDLF_OUTPUT_NO_NEWLINES |
                    IDLF_OUTPUT_NO_QUALIFY_IDENTS |
                    IDLF_OUTPUT_PROPERTIES);
    fputs(" */\n", state->file);
}





gboolean
xpidl_sprint_iid(nsID *id, char iidbuf[])
{
    int printed;

    printed = sprintf(iidbuf,
                       "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                       (PRUint32) id->m0, (PRUint32) id->m1,(PRUint32) id->m2,
                       (PRUint32) id->m3[0], (PRUint32) id->m3[1],
                       (PRUint32) id->m3[2], (PRUint32) id->m3[3],
                       (PRUint32) id->m3[4], (PRUint32) id->m3[5],
                       (PRUint32) id->m3[6], (PRUint32) id->m3[7]);

#ifdef SPRINTF_RETURNS_STRING
    return (printed && strlen((char *)printed) == 36);
#else
    return (printed == 36);
#endif
}


static const char nsIDFmt2[] =
  "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x";





gboolean
xpidl_parse_iid(nsID *id, const char *str)
{
    PRInt32 count = 0;
    PRUint32 n0, n1, n2;
    PRUint32 n3[8];
    PRUint32 i;

    XPT_ASSERT(str != NULL);
    
    if (strlen(str) != 36) {
        return FALSE;
    }
     
#ifdef DEBUG_shaver_iid
    fprintf(stderr, "parsing iid   %s\n", str);
#endif

    count = sscanf(str, nsIDFmt2,
                   &n0, &n1, &n2,
                   &n3[0],&n3[1],&n3[2],&n3[3],
                   &n3[4],&n3[5],&n3[6],&n3[7]);

    id->m0 = n0;
    id->m1 = (PRUint16) n1;
    id->m2 = (PRUint16) n2;
    for (i = 0; i < 8; i++) {
      id->m3[i] = (PRUint8) n3[i];
    }

#ifdef DEBUG_shaver_iid
    if (count == 11) {
        fprintf(stderr, "IID parsed to ");
        print_IID(id, stderr);
        fputs("\n", stderr);
    }
#endif
    return (gboolean)(count == 11);
}

gboolean
verify_const_declaration(IDL_tree const_tree) {
    struct _IDL_CONST_DCL *dcl = &IDL_CONST_DCL(const_tree);
    const char *name = IDL_IDENT(dcl->ident).str;
    IDL_tree real_type;

    
    if (!IDL_NODE_UP(IDL_NODE_UP(const_tree)) ||
        IDL_NODE_TYPE(IDL_NODE_UP(IDL_NODE_UP(const_tree)))
        != IDLN_INTERFACE) {
        IDL_tree_error(const_tree,
                       "const declaration \'%s\' outside interface",
                       name);
        return FALSE;
    }

    
    real_type = find_underlying_type(dcl->const_type);
    real_type = real_type ? real_type : dcl->const_type;
    if (IDL_NODE_TYPE(real_type) == IDLN_TYPE_INTEGER &&
        (IDL_TYPE_INTEGER(real_type).f_type == IDL_INTEGER_TYPE_SHORT ||
         IDL_TYPE_INTEGER(real_type).f_type == IDL_INTEGER_TYPE_LONG))
    {
        if (!IDL_TYPE_INTEGER(real_type).f_signed &&
            IDL_INTEGER(dcl->const_exp).value < 0)
        {
#ifndef G_HAVE_GINT64
            







            if (IDL_TYPE_INTEGER(real_type).f_type == IDL_INTEGER_TYPE_LONG)
            {
                XPIDL_WARNING((const_tree, IDL_WARNING1,
                              "unsigned const declaration \'%s\' "
                              "initialized with (possibly) negative constant",
                              name));
                return TRUE;
            }
#endif
            IDL_tree_error(const_tree,
                           "unsigned const declaration \'%s\' initialized with "
                           "negative constant",
                           name);
            return FALSE;
        }
    } else {
        IDL_tree_error(const_tree,
                       "const declaration \'%s\' must be of type short or long",
                       name);
        return FALSE;
    }

    return TRUE;
}







static gboolean
verify_type_fits_version(IDL_tree in_tree, IDL_tree error_tree)
{
    if (major_version == 1 && minor_version == 1)
    {
        

        
        if (IDL_tree_property_get(in_tree, "utf8string") != NULL ||
            IDL_tree_property_get(in_tree, "cstring")    != NULL ||
            IDL_tree_property_get(in_tree, "astring")    != NULL)
        {
            IDL_tree_error(error_tree,
                           "Cannot use [utf8string], [cstring] and [astring] "
                           "types when generating version 1.1 typelibs\n");
            return FALSE;
        }
    }
    return TRUE;
}

static gboolean
IsNot_AlphaUpper(char letter)
{
    return letter < 'A' || letter > 'Z';
}

static gboolean
IsNot_AlphaLower(char letter)
{
    return letter < 'a' || letter > 'z';
}

static gboolean
matches_IFoo(const char* substring)
{
    if (substring[0] != 'I')
        return FALSE;
    if (IsNot_AlphaUpper(substring[1]))
        return FALSE;
    if (IsNot_AlphaLower(substring[2]))
        return FALSE;
    return TRUE;
}

static gboolean
matches_nsIFoo(const char* attribute_name)
{
    if (IsNot_AlphaLower(attribute_name[0]))
        return FALSE;
    if (IsNot_AlphaLower(attribute_name[1]))
        return FALSE;
    if (matches_IFoo(attribute_name + 2))
        return TRUE;
    if (IsNot_AlphaLower(attribute_name[2]))
        return FALSE;
    return matches_IFoo(attribute_name + 3);
}






gboolean
is_method_scriptable(IDL_tree method_tree, IDL_tree ident)
{
    IDL_tree iface;
    gboolean scriptable_interface;
    
    




    if (IDL_NODE_UP(method_tree) && IDL_NODE_UP(IDL_NODE_UP(method_tree)) &&
        IDL_NODE_TYPE(iface = IDL_NODE_UP(IDL_NODE_UP(method_tree))) 
        == IDLN_INTERFACE)
    {
        scriptable_interface =
            (IDL_tree_property_get(IDL_INTERFACE(iface).ident, "scriptable")
             != NULL);
    } else {
        IDL_tree_error(method_tree,
                       "is_method_scriptable called on a non-interface?");
        return FALSE;
    }

    
    if (!scriptable_interface)
      return FALSE;

    
    if (IDL_tree_property_get(ident, "notxpcom") != NULL)
      return FALSE;

    
    if (IDL_tree_property_get(ident, "noscript") != NULL)
      return FALSE;

    


    return TRUE;
}

gboolean
verify_attribute_declaration(IDL_tree attr_tree)
{
    IDL_tree iface;
    IDL_tree ident;
    IDL_tree attr_type;

    




    if (strcmp(
        IDL_IDENT(
            IDL_LIST(IDL_ATTR_DCL(attr_tree).simple_declarations).data).str, 
        "IID") == 0) {
        IDL_tree_error(attr_tree,
                       "Attributes named IID not supported, causes vtable "
                       "ordering problems");
        return FALSE;
    }

    



    ident = IDL_LIST(IDL_ATTR_DCL(attr_tree).simple_declarations).data;

    




    if (!is_method_scriptable(attr_tree, ident))
        return TRUE;

    



    attr_type = IDL_ATTR_DCL(attr_tree).param_type_spec;

    if (attr_type != NULL)
    {
        if (UP_IS_NATIVE(attr_type) &&
            IDL_tree_property_get(attr_type, "nsid") == NULL &&
            IDL_tree_property_get(attr_type, "domstring") == NULL &&
            IDL_tree_property_get(attr_type, "utf8string") == NULL &&
            IDL_tree_property_get(attr_type, "cstring") == NULL &&
            IDL_tree_property_get(attr_type, "astring") == NULL)
        {
            IDL_tree_error(attr_tree,
                           "attributes in [scriptable] interfaces that are "
                           "non-scriptable because they refer to native "
                           "types must be marked [noscript]\n");
            return FALSE;
        }
        




         
        if ((IDL_tree_property_get(ident, "notxpcom") == NULL || !(IDL_ATTR_DCL(attr_tree).f_readonly)) &&
            IDL_tree_property_get(attr_type,"nsid") != NULL &&
            IDL_tree_property_get(attr_type,"ptr") == NULL &&
            IDL_tree_property_get(attr_type,"ref") == NULL)
        {
            IDL_tree_error(attr_tree,
                           "Feature not currently supported: "
                           "attributes with a type of nsid must be marked "
                           "either [ptr] or [ref], or "
                           "else must be marked [notxpcom] "
                           "and must be read-only\n");
            return FALSE;
        }

        




        if (matches_nsIFoo(IDL_IDENT(IDL_LIST(IDL_ATTR_DCL(attr_tree).
                simple_declarations).data).str)) {
            XPIDL_WARNING((attr_tree, IDL_WARNING1,
                 "Naming an attribute nsIFoo causes "
                 "problems for interface flattening"));
        }

        




        if (!verify_type_fits_version(attr_type, attr_tree))
            return FALSE;
    }

    if (IDL_LIST(IDL_ATTR_DCL(attr_tree).simple_declarations).next != NULL)
    {
        IDL_tree_error(attr_tree,
            "multiple attributes in a single declaration is not supported\n");
        return FALSE;
    }
    return TRUE;
}







IDL_tree 
find_underlying_type(IDL_tree typedef_ident)
{
    IDL_tree up;

    if (typedef_ident == NULL || IDL_NODE_TYPE(typedef_ident) != IDLN_IDENT)
        return NULL;

    up = IDL_NODE_UP(typedef_ident);
    if (up == NULL || IDL_NODE_TYPE(up) != IDLN_LIST)
        return NULL;
    up = IDL_NODE_UP(up);
    if (up == NULL || IDL_NODE_TYPE(up) != IDLN_TYPE_DCL)
        return NULL;

    return IDL_TYPE_DCL(up).type_spec;
}

static IDL_tree 
find_named_parameter(IDL_tree method_tree, const char *param_name)
{
    IDL_tree iter;
    for (iter = IDL_OP_DCL(method_tree).parameter_dcls; iter;
         iter = IDL_LIST(iter).next)
    {
        IDL_tree param = IDL_LIST(iter).data;
        IDL_tree simple_decl = IDL_PARAM_DCL(param).simple_declarator;
        const char *current_name = IDL_IDENT(simple_decl).str;
        if (strcmp(current_name, param_name) == 0)
            return param;
    }
    return NULL;
}

typedef enum ParamAttrType {
    IID_IS,
    LENGTH_IS,
    SIZE_IS
} ParamAttrType;





static gboolean
check_param_attribute(IDL_tree method_tree, IDL_tree param,
                      ParamAttrType whattocheck)
{
    const char *method_name = IDL_IDENT(IDL_OP_DCL(method_tree).ident).str;
    const char *referred_name = NULL;
    IDL_tree param_type = IDL_PARAM_DCL(param).param_type_spec;
    IDL_tree simple_decl = IDL_PARAM_DCL(param).simple_declarator;
    const char *param_name = IDL_IDENT(simple_decl).str;
    const char *attr_name;
    const char *needed_type;

    if (whattocheck == IID_IS) {
        attr_name = "iid_is";
        needed_type = "IID";
    } else if (whattocheck == LENGTH_IS) {
        attr_name = "length_is";
        needed_type = "unsigned long (or PRUint32)";
    } else if (whattocheck == SIZE_IS) {
        attr_name = "size_is";
        needed_type = "unsigned long (or PRUint32)";
    } else {
        XPT_ASSERT("asked to check an unknown attribute type!");
        return TRUE;
    }
    
    referred_name = IDL_tree_property_get(simple_decl, attr_name);
    if (referred_name != NULL) {
        IDL_tree referred_param = find_named_parameter(method_tree,
                                                       referred_name);
        IDL_tree referred_param_type;
        if (referred_param == NULL) {
            IDL_tree_error(method_tree,
                           "attribute [%s(%s)] refers to missing "
                           "parameter \"%s\"",
                           attr_name, referred_name, referred_name);
            return FALSE;
        }
        if (referred_param == param) {
            IDL_tree_error(method_tree,
                           "attribute [%s(%s)] refers to its own parameter",
                           attr_name, referred_name);
            return FALSE;
        }
        
        referred_param_type = IDL_PARAM_DCL(referred_param).param_type_spec;
        if (whattocheck == IID_IS) {
            
            if (IDL_tree_property_get(referred_param_type, "nsid") == NULL) {
                IDL_tree_error(method_tree,
                               "target \"%s\" of [%s(%s)] attribute "
                               "must be of %s type",
                               referred_name, attr_name, referred_name,
                               needed_type);
                return FALSE;
            }
        } else if (whattocheck == LENGTH_IS || whattocheck == SIZE_IS) {
            
            IDL_tree real_type;

            
            real_type = find_underlying_type(referred_param_type);
            real_type = real_type ? real_type : referred_param_type;

            if (IDL_NODE_TYPE(real_type) != IDLN_TYPE_INTEGER ||
                IDL_TYPE_INTEGER(real_type).f_signed != FALSE ||
                IDL_TYPE_INTEGER(real_type).f_type != IDL_INTEGER_TYPE_LONG)
            {
                IDL_tree_error(method_tree,
                               "target \"%s\" of [%s(%s)] attribute "
                               "must be of %s type",
                               referred_name, attr_name, referred_name,
                               needed_type);

                return FALSE;
            }
        }
    }

    return TRUE;
}




gboolean
verify_method_declaration(IDL_tree method_tree)
{
    struct _IDL_OP_DCL *op = &IDL_OP_DCL(method_tree);
    IDL_tree iface;
    IDL_tree iter;
    gboolean notxpcom;
    gboolean scriptable_method;
    gboolean seen_retval = FALSE;
    gboolean hasoptional = PR_FALSE;
    const char *method_name = IDL_IDENT(IDL_OP_DCL(method_tree).ident).str;

    




    if (strcmp(method_name, "GetIID") == 0) {
        IDL_tree_error(method_tree,
                       "Methods named GetIID not supported, causes vtable "
                       "ordering problems");
        return FALSE;
    }
    if (op->f_varargs) {
        
        IDL_tree_error(method_tree, "varargs are not currently supported");
        return FALSE;
    }

    



    scriptable_method = is_method_scriptable(method_tree, op->ident);
    notxpcom = IDL_tree_property_get(op->ident, "notxpcom") != NULL;

    
    for (iter = op->parameter_dcls; iter; iter = IDL_LIST(iter).next) {
        IDL_tree param = IDL_LIST(iter).data;
        IDL_tree param_type =
            IDL_PARAM_DCL(param).param_type_spec;
        IDL_tree simple_decl =
            IDL_PARAM_DCL(param).simple_declarator;
        const char *param_name = IDL_IDENT(simple_decl).str;
        
        




        if (scriptable_method &&
            UP_IS_NATIVE(param_type) &&
            IDL_tree_property_get(param_type, "nsid") == NULL &&
            IDL_tree_property_get(simple_decl, "iid_is") == NULL &&
            IDL_tree_property_get(param_type, "domstring") == NULL &&
            IDL_tree_property_get(param_type, "utf8string") == NULL &&
            IDL_tree_property_get(param_type, "cstring") == NULL &&
            IDL_tree_property_get(param_type, "astring") == NULL)
        {
            IDL_tree_error(method_tree,
                           "methods in [scriptable] interfaces that are "
                           "non-scriptable because they refer to native "
                           "types (parameter \"%s\") must be marked "
                           "[noscript]", param_name);
            return FALSE;
        }

        




        if (!(notxpcom && IDL_PARAM_DCL(param).attr != IDL_PARAM_IN) &&
            IDL_tree_property_get(param_type, "nsid") != NULL &&
            IDL_tree_property_get(param_type, "ptr") == NULL &&
            IDL_tree_property_get(param_type, "ref") == NULL) 
        {
            IDL_tree_error(method_tree,
                           "Feature currently not supported: "
                           "parameter \"%s\" is of type nsid and "
                           "must be marked either [ptr] or [ref] "
                           "or method \"%s\" must be marked [notxpcom] "
                           "and must not be an input parameter",
                           param_name,
                           method_name);
            return FALSE;
        }
        


        if (IDL_tree_property_get(simple_decl, "retval") != NULL) {
            if (IDL_LIST(iter).next != NULL) {
                IDL_tree_error(method_tree,
                               "only the last parameter can be marked [retval]");
                return FALSE;
            }
            if (op->op_type_spec) {
                IDL_tree_error(method_tree,
                               "can't have [retval] with non-void return type");
                return FALSE;
            }
            
            if (seen_retval) {
                IDL_tree_error(method_tree,
                               "can't have more than one [retval] parameter");
                return FALSE;
            }
            seen_retval = TRUE;
        }

        




        if (IDL_tree_property_get(simple_decl, "shared") != NULL) {
            IDL_tree real_type;
            real_type = find_underlying_type(param_type);
            real_type = real_type ? real_type : param_type;

            if (IDL_tree_property_get(simple_decl, "array") != NULL) {
                IDL_tree_error(method_tree,
                               "[shared] parameter \"%s\" cannot "
                               "be of array type", param_name);
                return FALSE;
            }                

            if (!(IDL_NODE_TYPE(real_type) == IDLN_TYPE_STRING ||
                  IDL_NODE_TYPE(real_type) == IDLN_TYPE_WIDE_STRING ||
                  (UP_IS_NATIVE(real_type) &&
                   !IDL_tree_property_get(real_type, "nsid") &&
                   !IDL_tree_property_get(real_type, "domstring")  &&
                   !IDL_tree_property_get(real_type, "utf8string") &&
                   !IDL_tree_property_get(real_type, "cstring")    &&
                   !IDL_tree_property_get(real_type, "astring"))))
            {
                IDL_tree_error(method_tree,
                               "[shared] parameter \"%s\" must be of type "
                               "string, wstring or native", param_name);
                return FALSE;
            }
        }

        



        if (IDL_tree_property_get(simple_decl, "optional") != NULL) {
            hasoptional = PR_TRUE;
        }
        else if (hasoptional && IDL_tree_property_get(simple_decl, "retval") == NULL) {
            IDL_tree_error(method_tree,
                           "non-optional non-retval parameter used after one marked [optional]");
                return FALSE;
        }

        



        if (IDL_PARAM_DCL(param).attr == IDL_PARAM_INOUT &&
            UP_IS_NATIVE(param_type) &&
            (IDL_tree_property_get(param_type, "domstring")  != NULL ||
             IDL_tree_property_get(param_type, "utf8string") != NULL ||
             IDL_tree_property_get(param_type, "cstring")    != NULL ||
             IDL_tree_property_get(param_type, "astring")    != NULL )) {
            IDL_tree_error(method_tree,
                           "[domstring], [utf8string], [cstring], [astring] "
                           "types cannot be used as inout parameters");
            return FALSE;
        }


        


        if (IDL_tree_property_get(simple_decl, "array") != NULL &&
            UP_IS_NATIVE(param_type) &&
            (IDL_tree_property_get(param_type, "domstring")  != NULL ||
             IDL_tree_property_get(param_type, "utf8string") != NULL ||
             IDL_tree_property_get(param_type, "cstring")    != NULL ||
             IDL_tree_property_get(param_type, "astring")    != NULL)) {
            IDL_tree_error(method_tree,
                           "[domstring], [utf8string], [cstring], [astring] "
                           "types cannot be used in array parameters");
            return FALSE;
        }                

        if (!check_param_attribute(method_tree, param, IID_IS) ||
            !check_param_attribute(method_tree, param, LENGTH_IS) ||
            !check_param_attribute(method_tree, param, SIZE_IS))
            return FALSE;

        




        if (!verify_type_fits_version(param_type, method_tree))
            return FALSE;
        
    }
    
    
    
    if (scriptable_method &&
        op->op_type_spec != NULL && UP_IS_NATIVE(op->op_type_spec) &&
        IDL_tree_property_get(op->op_type_spec, "nsid") == NULL &&
        IDL_tree_property_get(op->op_type_spec, "domstring") == NULL &&
        IDL_tree_property_get(op->op_type_spec, "utf8string") == NULL &&
        IDL_tree_property_get(op->op_type_spec, "cstring") == NULL &&
        IDL_tree_property_get(op->op_type_spec, "astring") == NULL)
    {
        IDL_tree_error(method_tree,
                       "methods in [scriptable] interfaces that are "
                       "non-scriptable because they return native "
                       "types must be marked [noscript]");
        return FALSE;
    }


    



    if (!notxpcom &&
        op->op_type_spec != NULL &&
        IDL_tree_property_get(op->op_type_spec, "nsid") != NULL &&
        IDL_tree_property_get(op->op_type_spec, "ptr") == NULL &&
        IDL_tree_property_get(op->op_type_spec, "ref") == NULL) 
    {
        IDL_tree_error(method_tree,
                       "Feature currently not supported: "
                       "return value is of type nsid and "
                       "must be marked either [ptr] or [ref], "
                       "or else method \"%s\" must be marked [notxpcom] ",
                       method_name);
        return FALSE;
    }

    




    if (op->op_type_spec != NULL &&
        !verify_type_fits_version(op->op_type_spec, method_tree))
    {
        return FALSE;
    }

    return TRUE;
}





gboolean
check_native(TreeState *state)
{
    char *native_name;
    
    if (IDL_NATIVE(state->tree).user_type) 
        return TRUE;
    native_name = IDL_IDENT(IDL_NATIVE(state->tree).ident).str;
    IDL_tree_error(state->tree,
                   "``native %s;'' needs C++ type: ``native %s(<C++ type>);''",
                   native_name, native_name);
    return FALSE;
}




void
printlist(FILE *outfile, GSList *slist)
{
    guint i;
    guint len = g_slist_length(slist);

    for(i = 0; i < len; i++) {
        fprintf(outfile, 
                "%s\n", (char *)g_slist_nth_data(slist, i));
    }
}

void
xpidl_list_foreach(IDL_tree p, IDL_tree_func foreach, gpointer user_data)
{
    IDL_tree_func_data tfd;

    while (p) {
        struct _IDL_LIST *list = &IDL_LIST(p);
        tfd.tree = list->data;
        if (!foreach(&tfd, user_data))
            return;
        p = list->next;
    }
}




gboolean
verify_interface_declaration(IDL_tree interface_tree)
{
    IDL_tree iter;
    




    if (IDL_tree_property_get(IDL_INTERFACE(interface_tree).ident, 
        "scriptable")) {
        for (iter = IDL_INTERFACE(interface_tree).inheritance_spec; iter; 
            iter = IDL_LIST(iter).next) {
            if (IDL_tree_property_get(
                IDL_INTERFACE(iter).ident, "scriptable") == 0) {
                XPIDL_WARNING((interface_tree,IDL_WARNING1,
                    "%s is scriptable but inherits from the non-scriptable interface %s\n",
                    IDL_IDENT(IDL_INTERFACE(interface_tree).ident).str,
                    IDL_IDENT(IDL_INTERFACE(iter).ident).str));
            }
        }
    }
    return TRUE;
}




const char *
xpidl_basename(const char * path)
{
    const char * result = g_basename(path);
    



#if defined(XP_WIN32)
    const char * slash = strrchr(path, '/');
    
    if (slash != NULL && (slash > result))
        result = slash + 1;
#endif
    return result;
}
