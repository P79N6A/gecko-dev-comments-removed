







































#ifndef MOZILLA_A11Y_APPLICATION_ACCESSIBLE_WRAP_H__
#define MOZILLA_A11Y_APPLICATION_ACCESSIBLE_WRAP_H__

#include "ApplicationAccessible.h"

class ApplicationAccessibleWrap: public ApplicationAccessible
{
public:
  static void PreCreate() {}
  static void Unload() {}
};

#endif

