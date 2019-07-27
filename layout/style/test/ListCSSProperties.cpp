






#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mozilla/ArrayUtils.h"

struct PropertyInfo {
    const char *propName;
    const char *domName;
    const char *pref;
};

const PropertyInfo gLonghandProperties[] = {

#define CSS_PROP_PUBLIC_OR_PRIVATE(publicname_, privatename_) publicname_
#define CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, \
                 stylestruct_, stylestructoffset_, animtype_)                 \
    { #name_, #method_, pref_ },
#define CSS_PROP_LIST_INCLUDE_LOGICAL

#include "nsCSSPropList.h"

#undef CSS_PROP_LIST_EXCLUDE_LOGICAL
#undef CSS_PROP
#undef CSS_PROP_PUBLIC_OR_PRIVATE

};






const char* gLonghandPropertiesWithDOMProp[] = {

#define CSS_PROP_LIST_EXCLUDE_INTERNAL
#define CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, \
                 stylestruct_, stylestructoffset_, animtype_)                 \
    #name_,
#define CSS_PROP_LIST_INCLUDE_LOGICAL

#include "nsCSSPropList.h"

#undef CSS_PROP_LIST_INCLUDE_LOGICAL
#undef CSS_PROP
#undef CSS_PROP_LIST_EXCLUDE_INTERNAL

};

const PropertyInfo gShorthandProperties[] = {

#define CSS_PROP_PUBLIC_OR_PRIVATE(publicname_, privatename_) publicname_


#define LISTCSSPROPERTIES_INNER_MACRO(method_) #method_
#define CSS_PROP_SHORTHAND(name_, id_, method_, flags_, pref_)	\
    { #name_, LISTCSSPROPERTIES_INNER_MACRO(method_), pref_ },

#include "nsCSSPropList.h"

#undef CSS_PROP_SHORTHAND
#undef LISTCSSPROPERTIES_INNER_MACRO
#undef CSS_PROP_PUBLIC_OR_PRIVATE

#define CSS_PROP_ALIAS(name_, id_, method_, pref_) \
    { #name_, #method_, pref_ },

#include "nsCSSPropAliasList.h"

#undef CSS_PROP_ALIAS

};


const char* gShorthandPropertiesWithDOMProp[] = {

#define CSS_PROP_LIST_EXCLUDE_INTERNAL
#define CSS_PROP_SHORTHAND(name_, id_, method_, flags_, pref_)	\
    #name_,

#include "nsCSSPropList.h"

#undef CSS_PROP_SHORTHAND
#undef CSS_PROP_LIST_EXCLUDE_INTERNAL

#define CSS_PROP_ALIAS(name_, id_, method_, pref_) \
    #name_,

#include "nsCSSPropAliasList.h"

#undef CSS_PROP_ALIAS

};

const char *gInaccessibleProperties[] = {
    
    
    "-x-cols",
    "-x-lang",
    "-x-span",
    "-x-system-font",
    "-x-text-zoom",
    "-moz-control-character-visibility",
    "-moz-script-level", 
    "-moz-script-size-multiplier",
    "-moz-script-min-size",
    "-moz-math-variant",
    "-moz-math-display" 
};

inline int
is_inaccessible(const char* aPropName)
{
    for (unsigned j = 0; j < MOZ_ARRAY_LENGTH(gInaccessibleProperties); ++j) {
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
        if (p->pref[0]) {
            printf(", pref: \"%s\"", p->pref);
        }
        printf(" }");
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
                MOZ_ARRAY_LENGTH(gLonghandProperties),
                gLonghandPropertiesWithDOMProp,
                MOZ_ARRAY_LENGTH(gLonghandPropertiesWithDOMProp));
    print_array("gShorthandProperties",
                gShorthandProperties,
                MOZ_ARRAY_LENGTH(gShorthandProperties),
                gShorthandPropertiesWithDOMProp,
                MOZ_ARRAY_LENGTH(gShorthandPropertiesWithDOMProp));
    return 0;
}
