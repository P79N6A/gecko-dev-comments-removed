





class JSObject;
struct JSContext;
struct JSStructuredCloneReader;
struct JSStructuredCloneWriter;

namespace mozilla {
namespace dom {

class ImageData;

JSObject*
ReadStructuredCloneImageData(JSContext* aCx, JSStructuredCloneReader* aReader);

bool
WriteStructuredCloneImageData(JSContext* aCx, JSStructuredCloneWriter* aWriter,
                              ImageData* aImageData);

} 
} 
