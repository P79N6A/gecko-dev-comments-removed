






































#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct PropertyInfo {
    const char *propName;
    const char *domName;
};

const PropertyInfo gLonghandProperties[] = {

#define CSS_PROP_NOTIMPLEMENTED(name_, id_, method_) \
    { #name_, #method_ },
#define CSS_PROP(name_, id_, method_, datastruct_, member_, type_, kwtable_) \
    { #name_, #method_ },

#include "nsCSSPropList.h"

#undef CSS_PROP_NOTIMPLEMENTED
#undef CSS_PROP

};






const char* gLonghandPropertiesWithDOMProp[] = {

#define CSS_PROP_LIST_EXCLUDE_INTERNAL
#define CSS_PROP_NOTIMPLEMENTED(name_, id_, method_) \
    #name_,
#define CSS_PROP(name_, id_, method_, datastruct_, member_, type_, kwtable_) \
    #name_,

#include "nsCSSPropList.h"

#undef CSS_PROP_NOTIMPLEMENTED
#undef CSS_PROP
#undef CSS_PROP_LIST_EXCLUDE_INTERNAL

};

const PropertyInfo gShorthandProperties[] = {

#define CSS_PROP_SHORTHAND(name_, id_, method_) \
    { #name_, #method_ },

#include "nsCSSPropList.h"

#undef CSS_PROP_SHORTHAND

};


const char* gShorthandPropertiesWithDOMProp[] = {

#define CSS_PROP_LIST_EXCLUDE_INTERNAL
#define CSS_PROP_SHORTHAND(name_, id_, method_) \
    #name_,

#include "nsCSSPropList.h"

#undef CSS_PROP_SHORTHAND
#undef CSS_PROP_LIST_EXCLUDE_INTERNAL

};


#define ARRAY_LENGTH(a_) (sizeof(a_)/sizeof((a_)[0]))

const char *gInaccessibleProperties[] = {
    
    
    "margin-end-value",
    "margin-left-value",
    "margin-right-value",
    "margin-start-value",
    "margin-left-ltr-source",
    "margin-left-rtl-source",
    "margin-right-ltr-source",
    "margin-right-rtl-source",
    "padding-end-value",
    "padding-left-value",
    "padding-right-value",
    "padding-start-value",
    "padding-left-ltr-source",
    "padding-left-rtl-source",
    "padding-right-ltr-source",
    "padding-right-rtl-source"
};

inline int
is_inaccessible(const char* aPropName)
{
    for (unsigned j = 0; j < ARRAY_LENGTH(gInaccessibleProperties); ++j) {
        if (strcmp(aPropName, gInaccessibleProperties[j]) == 0)
            return 1;
    }
    return 0;
}

void
print_array(const char *aName,
            const PropertyInfo *aProps, unsigned aPropsLength,
            const char * const * aDOMProps, unsigned aDOMPropsLength)
{
    printf("var %s = [\n", aName);

    int first = 1;
    unsigned j = 0; 
    for (unsigned i = 0; i < aPropsLength; ++i) {
        const PropertyInfo *p = aProps + i;

        if (is_inaccessible(p->propName))
            
            
            
            continue;

        if (first)
            first = 0;
        else
            printf(",\n");

        printf("\t{ name: \"%s\", prop: ", p->propName);
        if (j >= aDOMPropsLength || strcmp(p->propName, aDOMProps[j]) != 0)
            printf("null");
        else {
            ++j;
            if (strncmp(p->domName, "Moz", 3) == 0)
                printf("\"%s\"", p->domName);
            else
                
                printf("\"%c%s\"", p->domName[0] + 32, p->domName + 1);
        }
        printf("}");
    }

    if (j != aDOMPropsLength) {
        fprintf(stderr, "Assertion failure %s:%d\n", __FILE__, __LINE__);
        fprintf(stderr, "j==%d, aDOMPropsLength == %d\n", j, aDOMPropsLength);
        exit(1);
    }

    printf("\n];\n\n");
}

int
main()
{
    print_array("gLonghandProperties",
                gLonghandProperties,
                ARRAY_LENGTH(gLonghandProperties),
                gLonghandPropertiesWithDOMProp,
                ARRAY_LENGTH(gLonghandPropertiesWithDOMProp));
    print_array("gShorthandProperties",
                gShorthandProperties,
                ARRAY_LENGTH(gShorthandProperties),
                gShorthandPropertiesWithDOMProp,
                ARRAY_LENGTH(gShorthandPropertiesWithDOMProp));
    return 0;
}
