





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

BEGIN_TEST(testSavedStacks_ApiDefaultValues)
{
    js::RootedSavedFrame savedFrame(cx, nullptr);

    
    JS::RootedString str(cx);
    JS::SavedFrameResult result = JS::GetSavedFrameSource(cx, savedFrame, &str);
    CHECK(result == JS::SavedFrameResult::AccessDenied);
    CHECK(str.get() == cx->runtime()->emptyString);

    
    uint32_t line = 123;
    result = JS::GetSavedFrameLine(cx, savedFrame, &line);
    CHECK(result == JS::SavedFrameResult::AccessDenied);
    CHECK(line == 0);

    
    uint32_t column = 123;
    result = JS::GetSavedFrameColumn(cx, savedFrame, &column);
    CHECK(result == JS::SavedFrameResult::AccessDenied);
    CHECK(column == 0);

    
    result = JS::GetSavedFrameFunctionDisplayName(cx, savedFrame, &str);
    CHECK(result == JS::SavedFrameResult::AccessDenied);
    CHECK(str.get() == nullptr);

    
    JS::RootedObject parent(cx);
    result = JS::GetSavedFrameParent(cx, savedFrame, &parent);
    CHECK(result == JS::SavedFrameResult::AccessDenied);
    CHECK(parent.get() == nullptr);

    
    result = JS::GetSavedFrameSource(cx, savedFrame, &str);
    CHECK(result == JS::SavedFrameResult::AccessDenied);
    CHECK(str.get() == cx->runtime()->emptyString);

    return true;
}
END_TEST(testSavedStacks_ApiDefaultValues)
