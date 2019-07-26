








#ifndef mozilla_dom_DOMSlots_h
#define mozilla_dom_DOMSlots_h



#define DOM_OBJECT_SLOT 0




#define DOM_XRAY_EXPANDO_SLOT 1





#define DOM_OBJECT_SLOT_SOW 2




#define DOM_PROTO_INSTANCE_CLASS_SLOT 0



#define DOM_INTERFACE_SLOTS_BASE (DOM_XRAY_EXPANDO_SLOT + 1)




#define DOM_INTERFACE_PROTO_SLOTS_BASE (DOM_XRAY_EXPANDO_SLOT + 1)

static_assert(DOM_PROTO_INSTANCE_CLASS_SLOT != DOM_XRAY_EXPANDO_SLOT,
              "Interface prototype object use both of these, so they must "
              "not be the same slot.");

#endif 
