




































#include <stdio.h>
#include "nsColor.h"
#include "nsColorNames.h"
#include "nsString.h"

int main(int argc, char** argv)
{
  for (int i = 0; i < eColorName_COUNT; i++) {
    nscolor rgba = nsColorNames::kColors[i];
    printf("%s: NS_RGB(%d,%d,%d,%d)\n",
       nsColorNames::GetStringValue(nsColorName(i)).get(),
	   NS_GET_R(rgba), NS_GET_G(rgba), NS_GET_B(rgba), NS_GET_A(rgba));
  }
  return 0;
}
