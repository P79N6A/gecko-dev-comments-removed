




#ifndef mozilla_dom_SVGElementFactory_h
#define mozilla_dom_SVGElementFactory_h

class nsIAtom;

namespace mozilla {
namespace dom {

class SVGElementFactory {
public:
  static void Init();
  static void Shutdown();

  static bool Exists(nsIAtom *aTag);
};

} 
} 

#endif 
