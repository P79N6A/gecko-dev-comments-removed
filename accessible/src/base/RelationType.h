





#ifndef mozilla_a11y_relationtype_h_
#define mozilla_a11y_relationtype_h_

#include "mozilla/TypedEnum.h"

namespace mozilla {
namespace a11y {

MOZ_BEGIN_ENUM_CLASS(RelationType)

  


  LABELLED_BY = 0x00,

  


  LABEL_FOR = 0x01,

  


  DESCRIBED_BY = 0x02,

  


  DESCRIPTION_FOR = 0x3,

  


  NODE_CHILD_OF = 0x4,

  



  NODE_PARENT_OF = 0x5,

  


  CONTROLLED_BY = 0x06,

  


  CONTROLLER_FOR = 0x07,

  



  FLOWS_TO = 0x08,

  



  FLOWS_FROM = 0x09,

  





  MEMBER_OF = 0x0a,

  


  SUBWINDOW_OF = 0x0b,

  




  EMBEDS = 0x0c,

  


  EMBEDDED_BY = 0x0d,

  



  POPUP_FOR = 0x0e,

  


  PARENT_WINDOW_OF = 0x0f,

  



  DEFAULT_BUTTON = 0x10,

  LAST = DEFAULT_BUTTON

MOZ_END_ENUM_CLASS(RelationType)

} 
} 

#endif
