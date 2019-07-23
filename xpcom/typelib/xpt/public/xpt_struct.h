









































#ifndef __xpt_struct_h__
#define __xpt_struct_h__

#include "xpt_arena.h"

PR_BEGIN_EXTERN_C












typedef struct XPTHeader XPTHeader;
typedef struct XPTInterfaceDirectoryEntry XPTInterfaceDirectoryEntry;
typedef struct XPTInterfaceDescriptor XPTInterfaceDescriptor;
typedef struct XPTConstDescriptor XPTConstDescriptor;
typedef struct XPTMethodDescriptor XPTMethodDescriptor;
typedef struct XPTParamDescriptor XPTParamDescriptor;
typedef struct XPTTypeDescriptor XPTTypeDescriptor;
typedef struct XPTTypeDescriptorPrefix XPTTypeDescriptorPrefix;
typedef struct XPTString XPTString;
typedef struct XPTAnnotation XPTAnnotation;
#ifndef nsID_h__





struct nsID {
    PRUint32 m0;
    PRUint16 m1;
    PRUint16 m2;
    PRUint8  m3[8];
};

typedef struct nsID nsID;
#endif

#define XPT_COPY_IID(to, from)                                                \
  (to).m0 = (from).m0;                                                        \
  (to).m1 = (from).m1;                                                        \
  (to).m2 = (from).m2;                                                        \
  (to).m3[0] = (from).m3[0];                                                  \
  (to).m3[1] = (from).m3[1];                                                  \
  (to).m3[2] = (from).m3[2];                                                  \
  (to).m3[3] = (from).m3[3];                                                  \
  (to).m3[4] = (from).m3[4];                                                  \
  (to).m3[5] = (from).m3[5];                                                  \
  (to).m3[6] = (from).m3[6];                                                  \
  (to).m3[7] = (from).m3[7];





struct XPTHeader {
    PRUint8                     magic[16];
    PRUint8                     major_version;
    PRUint8                     minor_version;
    PRUint16                    num_interfaces;
    PRUint32                    file_length;
    XPTInterfaceDirectoryEntry  *interface_directory;
    PRUint32                    data_pool;
    XPTAnnotation               *annotations;
};

#define XPT_MAGIC "XPCOM\nTypeLib\r\n\032"

#define XPT_MAGIC_STRING "XPCOM\\nTypeLib\\r\\n\\032"
#define XPT_MAJOR_VERSION 0x01
#define XPT_MINOR_VERSION 0x02











#define XPT_MAJOR_INCOMPATIBLE_VERSION 0x02



































  
#define XPT_VERSION_UNKNOWN     0
#define XPT_VERSION_UNSUPPORTED 1
#define XPT_VERSION_OLD         2
#define XPT_VERSION_CURRENT     3

typedef struct {
    const char* str;
    PRUint8     major;
    PRUint8     minor;
    PRUint16    code;
} XPT_TYPELIB_VERSIONS_STRUCT; 


#define XPT_TYPELIB_VERSIONS {                                                \
    {"1.0", 1, 0, XPT_VERSION_UNSUPPORTED},                                   \
    {"1.1", 1, 1, XPT_VERSION_OLD},                                           \
    {"1.2", 1, 2, XPT_VERSION_CURRENT}                                        \
}

extern XPT_PUBLIC_API(PRUint16)
XPT_ParseVersionString(const char* str, PRUint8* major, PRUint8* minor);

extern XPT_PUBLIC_API(XPTHeader *)
XPT_NewHeader(XPTArena *arena, PRUint16 num_interfaces, 
              PRUint8 major_version, PRUint8 minor_version);

extern XPT_PUBLIC_API(void)
XPT_FreeHeader(XPTArena *arena, XPTHeader* aHeader);


extern XPT_PUBLIC_API(PRUint32)
XPT_SizeOfHeader(XPTHeader *header);


extern XPT_PUBLIC_API(PRUint32)
XPT_SizeOfHeaderBlock(XPTHeader *header);







struct XPTInterfaceDirectoryEntry {
    nsID                   iid;
    char                   *name;
    char                   *name_space;
    XPTInterfaceDescriptor *interface_descriptor;

#if 0 
    
    PRUint32                 offset; 
#endif
};

extern XPT_PUBLIC_API(PRBool)
XPT_FillInterfaceDirectoryEntry(XPTArena *arena, 
                                XPTInterfaceDirectoryEntry *ide,
                                nsID *iid, char *name, char *name_space,
                                XPTInterfaceDescriptor *descriptor);

extern XPT_PUBLIC_API(void)
XPT_DestroyInterfaceDirectoryEntry(XPTArena *arena, 
                                   XPTInterfaceDirectoryEntry* ide);





struct XPTInterfaceDescriptor {
    PRUint16                parent_interface;
    PRUint16                num_methods;
    XPTMethodDescriptor     *method_descriptors;
    PRUint16                num_constants;
    XPTConstDescriptor      *const_descriptors;
    PRUint8                 flags;

    















    XPTTypeDescriptor       *additional_types;
    PRUint16                num_additional_types;
};

#define XPT_ID_SCRIPTABLE           0x80
#define XPT_ID_FUNCTION             0x40
#define XPT_ID_FLAGMASK             0xc0
#define XPT_ID_TAGMASK              (~XPT_ID_FLAGMASK)
#define XPT_ID_TAG(id)              ((id).flags & XPT_ID_TAGMASK)

#define XPT_ID_IS_SCRIPTABLE(flags) (!!(flags & XPT_ID_SCRIPTABLE))
#define XPT_ID_IS_FUNCTION(flags) (!!(flags & XPT_ID_FUNCTION))

extern XPT_PUBLIC_API(PRBool)
XPT_GetInterfaceIndexByName(XPTInterfaceDirectoryEntry *ide_block,
                            PRUint16 num_interfaces, char *name, 
                            PRUint16 *indexp);

extern XPT_PUBLIC_API(XPTInterfaceDescriptor *)
XPT_NewInterfaceDescriptor(XPTArena *arena, 
                           PRUint16 parent_interface, PRUint16 num_methods,
                           PRUint16 num_constants, PRUint8 flags);

extern XPT_PUBLIC_API(void)
XPT_FreeInterfaceDescriptor(XPTArena *arena, XPTInterfaceDescriptor* id);

extern XPT_PUBLIC_API(PRBool)
XPT_InterfaceDescriptorAddTypes(XPTArena *arena, XPTInterfaceDescriptor *id, 
                                PRUint16 num);

extern XPT_PUBLIC_API(PRBool)
XPT_InterfaceDescriptorAddMethods(XPTArena *arena, XPTInterfaceDescriptor *id, 
                                  PRUint16 num);

extern XPT_PUBLIC_API(PRBool)
XPT_InterfaceDescriptorAddConsts(XPTArena *arena, XPTInterfaceDescriptor *id, 
                                 PRUint16 num);





struct XPTString {
    PRUint16 length;
    char   *bytes;
};

extern XPT_PUBLIC_API(XPTString *)
XPT_NewString(XPTArena *arena, PRUint16 length, char *bytes);

extern XPT_PUBLIC_API(XPTString *)
XPT_NewStringZ(XPTArena *arena, char *bytes);



















struct XPTTypeDescriptorPrefix {
    PRUint8 flags;
};


#define XPT_TDP_POINTER          0x80
#define XPT_TDP_UNIQUE_POINTER   0x40
#define XPT_TDP_REFERENCE        0x20
#define XPT_TDP_FLAGMASK         0xe0
#define XPT_TDP_TAGMASK          (~XPT_TDP_FLAGMASK)
#define XPT_TDP_TAG(tdp)         ((tdp).flags & XPT_TDP_TAGMASK)

#define XPT_TDP_IS_POINTER(flags)        (flags & XPT_TDP_POINTER)
#define XPT_TDP_IS_UNIQUE_POINTER(flags) (flags & XPT_TDP_UNIQUE_POINTER)
#define XPT_TDP_IS_REFERENCE(flags)      (flags & XPT_TDP_REFERENCE)





enum XPTTypeDescriptorTags {
    TD_INT8              = 0,
    TD_INT16             = 1,
    TD_INT32             = 2,
    TD_INT64             = 3,
    TD_UINT8             = 4,
    TD_UINT16            = 5,
    TD_UINT32            = 6,
    TD_UINT64            = 7,
    TD_FLOAT             = 8, 
    TD_DOUBLE            = 9,
    TD_BOOL              = 10,  
    TD_CHAR              = 11,  
    TD_WCHAR             = 12, 
    TD_VOID              = 13,  
    TD_PNSIID            = 14,
    TD_DOMSTRING         = 15,
    TD_PSTRING           = 16,
    TD_PWSTRING          = 17,
    TD_INTERFACE_TYPE    = 18,
    TD_INTERFACE_IS_TYPE = 19,
    TD_ARRAY             = 20,
    TD_PSTRING_SIZE_IS   = 21,
    TD_PWSTRING_SIZE_IS  = 22,
    TD_UTF8STRING        = 23,
    TD_CSTRING           = 24,
    TD_ASTRING           = 25
};

struct XPTTypeDescriptor {
    XPTTypeDescriptorPrefix prefix;
    PRUint8 argnum;                 
    PRUint8 argnum2;                
    union {                         
        PRUint16 iface;             
        PRUint16 additional_type;   
    } type;
};

#define XPT_COPY_TYPE(to, from)                                               \
  (to).prefix.flags = (from).prefix.flags;                                    \
  (to).argnum = (from).argnum;                                                \
  (to).argnum2 = (from).argnum2;                                              \
  (to).type.additional_type = (from).type.additional_type;


















union XPTConstValue {
    PRInt8    i8;
    PRUint8   ui8; 
    PRInt16   i16; 
    PRUint16  ui16;
    PRInt32   i32; 
    PRUint32  ui32;
    PRInt64   i64; 
    PRUint64  ui64; 
    float     flt;
    double    dbl;
    PRBool    bul;
    char      ch; 
    PRUint16  wch;
    nsID      *iid;
    XPTString *string;
    char      *str;
    PRUint16  *wstr;
}; 

struct XPTConstDescriptor {
    char                *name;
    XPTTypeDescriptor   type;
    union XPTConstValue value;
};





struct XPTParamDescriptor {
    PRUint8           flags;
    XPTTypeDescriptor type;
};


#define XPT_PD_IN       0x80
#define XPT_PD_OUT      0x40
#define XPT_PD_RETVAL   0x20
#define XPT_PD_SHARED   0x10
#define XPT_PD_DIPPER   0x08
#define XPT_PD_OPTIONAL 0x04
#define XPT_PD_FLAGMASK 0xfc

#define XPT_PD_IS_IN(flags)     (flags & XPT_PD_IN)
#define XPT_PD_IS_OUT(flags)    (flags & XPT_PD_OUT)
#define XPT_PD_IS_RETVAL(flags) (flags & XPT_PD_RETVAL)
#define XPT_PD_IS_SHARED(flags) (flags & XPT_PD_SHARED)
#define XPT_PD_IS_DIPPER(flags) (flags & XPT_PD_DIPPER)
#define XPT_PD_IS_OPTIONAL(flags) (flags & XPT_PD_OPTIONAL)

extern XPT_PUBLIC_API(PRBool)
XPT_FillParamDescriptor(XPTArena *arena, 
                        XPTParamDescriptor *pd, PRUint8 flags,
                        XPTTypeDescriptor *type);





struct XPTMethodDescriptor {
    char                *name;
    XPTParamDescriptor  *params;
    XPTParamDescriptor  *result;
    PRUint8             flags;
    PRUint8             num_args;
};


#define XPT_MD_GETTER   0x80
#define XPT_MD_SETTER   0x40
#define XPT_MD_NOTXPCOM 0x20
#define XPT_MD_CTOR     0x10
#define XPT_MD_HIDDEN   0x08
#define XPT_MD_OPT_ARGC 0x04
#define XPT_MD_FLAGMASK 0xfc

#define XPT_MD_IS_GETTER(flags)      (flags & XPT_MD_GETTER)
#define XPT_MD_IS_SETTER(flags)      (flags & XPT_MD_SETTER)
#define XPT_MD_IS_NOTXPCOM(flags)    (flags & XPT_MD_NOTXPCOM)
#define XPT_MD_IS_CTOR(flags)        (flags & XPT_MD_CTOR)
#define XPT_MD_IS_HIDDEN(flags)      (flags & XPT_MD_HIDDEN)
#define XPT_MD_WANTS_OPT_ARGC(flags) (flags & XPT_MD_OPT_ARGC)

extern XPT_PUBLIC_API(PRBool)
XPT_FillMethodDescriptor(XPTArena *arena, 
                         XPTMethodDescriptor *meth, PRUint8 flags, char *name,
                         PRUint8 num_args);




















struct XPTAnnotation {
    XPTAnnotation *next;
    PRUint8       flags;
    
    XPTString     *creator;
    XPTString     *private_data;
};

#define XPT_ANN_LAST	                0x80
#define XPT_ANN_IS_LAST(flags)          (flags & XPT_ANN_LAST)
#define XPT_ANN_PRIVATE                 0x40
#define XPT_ANN_IS_PRIVATE(flags)       (flags & XPT_ANN_PRIVATE)

extern XPT_PUBLIC_API(XPTAnnotation *)
XPT_NewAnnotation(XPTArena *arena, PRUint8 flags, XPTString *creator, 
                  XPTString *private_data);

PR_END_EXTERN_C

#endif 
