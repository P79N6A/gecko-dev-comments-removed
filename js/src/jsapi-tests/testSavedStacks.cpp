





#include "jscompartment.h"
#include "jsfriendapi.h"
#include "jsstr.h"

#include "jsapi-tests/tests.h"

#include "vm/ArrayObject.h"
#include "vm/SavedStacks.h"

BEGIN_TEST(testSavedStacks_withNoStack)
{
    JSCompartment *compartment = js::GetContextCompartment(cx);
    compartment->setObjectMetadataCallback(js::SavedStacksMetadataCallback);
    JS::RootedObject obj(cx, js::NewDenseEmptyArray(cx));
    compartment->setObjectMetadataCallback(nullptr);
    return true;
}
END_TEST(testSavedStacks_withNoStack)
