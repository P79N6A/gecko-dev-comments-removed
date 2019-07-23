















#include "BasicPlugin.h"


static NPNetscapeFuncs* browser;


static CFStringRef browserUAString = NULL;


typedef struct PluginInstance {
  NPP npp;
  NPWindow window;
} PluginInstance;

void drawPlugin(NPP instance, NPCocoaEvent* event);


NPError NP_Initialize(NPNetscapeFuncs* browserFuncs)
{  
  
  browser = browserFuncs;

  return NPERR_NO_ERROR;
}


NPError NP_GetEntryPoints(NPPluginFuncs* pluginFuncs)
{
  pluginFuncs->version = 11;
  pluginFuncs->size = sizeof(pluginFuncs);
  pluginFuncs->newp = NPP_New;
  pluginFuncs->destroy = NPP_Destroy;
  pluginFuncs->setwindow = NPP_SetWindow;
  pluginFuncs->newstream = NPP_NewStream;
  pluginFuncs->destroystream = NPP_DestroyStream;
  pluginFuncs->asfile = NPP_StreamAsFile;
  pluginFuncs->writeready = NPP_WriteReady;
  pluginFuncs->write = (NPP_WriteProcPtr)NPP_Write;
  pluginFuncs->print = NPP_Print;
  pluginFuncs->event = NPP_HandleEvent;
  pluginFuncs->urlnotify = NPP_URLNotify;
  pluginFuncs->getvalue = NPP_GetValue;
  pluginFuncs->setvalue = NPP_SetValue;

  return NPERR_NO_ERROR;
}


void NP_Shutdown(void)
{
  CFRelease(browserUAString);
  browserUAString = NULL;
}


NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode, int16_t argc, char* argn[], char* argv[], NPSavedData* saved)
{
  PluginInstance *newInstance = (PluginInstance*)malloc(sizeof(PluginInstance));
  bzero(newInstance, sizeof(PluginInstance));

  newInstance->npp = instance;
  instance->pdata = newInstance;

  
  NPBool supportsCoreGraphics = false;
  if (browser->getvalue(instance, NPNVsupportsCoreGraphicsBool, &supportsCoreGraphics) == NPERR_NO_ERROR && supportsCoreGraphics) {
    browser->setvalue(instance, NPPVpluginDrawingModel, (void*)NPDrawingModelCoreGraphics);
  } else {
    printf("CoreGraphics drawing model not supported, can't create a plugin instance.\n");
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  }

  
  NPBool supportsCocoaEvents = false;
  if (browser->getvalue(instance, NPNVsupportsCocoaBool, &supportsCocoaEvents) == NPERR_NO_ERROR && supportsCocoaEvents) {
    browser->setvalue(instance, NPPVpluginEventModel, (void*)NPEventModelCocoa);
  } else {
    printf("Cocoa event model not supported, can't create a plugin instance.\n");
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  }

  if (!browserUAString) {
    const char* ua = browser->uagent(instance);
    if (ua)
      browserUAString = CFStringCreateWithCString(kCFAllocatorDefault, ua, kCFStringEncodingASCII);
  }

  return NPERR_NO_ERROR;
}


NPError NPP_Destroy(NPP instance, NPSavedData** save)
{
  free(instance->pdata);

  return NPERR_NO_ERROR;
}


NPError NPP_SetWindow(NPP instance, NPWindow* window)
{
  PluginInstance* currentInstance = (PluginInstance*)(instance->pdata);

  currentInstance->window = *window;
  
  return NPERR_NO_ERROR;
}


NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream, NPBool seekable, uint16_t* stype)
{
  *stype = NP_ASFILEONLY;
  return NPERR_NO_ERROR;
}

NPError NPP_DestroyStream(NPP instance, NPStream* stream, NPReason reason)
{
  return NPERR_NO_ERROR;
}

int32_t NPP_WriteReady(NPP instance, NPStream* stream)
{
  return 0;
}

int32_t NPP_Write(NPP instance, NPStream* stream, int32_t offset, int32_t len, void* buffer)
{
  return 0;
}

void NPP_StreamAsFile(NPP instance, NPStream* stream, const char* fname)
{
}

void NPP_Print(NPP instance, NPPrint* platformPrint)
{
  
}

int16_t NPP_HandleEvent(NPP instance, void* event)
{
  NPCocoaEvent* cocoaEvent = (NPCocoaEvent*)event;
  if (cocoaEvent && (cocoaEvent->type == NPCocoaEventDrawRect)) {
    drawPlugin(instance, (NPCocoaEvent*)event);
    return 1;
  }

  return 0;
}

void NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{

}

NPError NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
  return NPERR_GENERIC_ERROR;
}

NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
  return NPERR_GENERIC_ERROR;
}

void drawPlugin(NPP instance, NPCocoaEvent* event)
{
  if (!browserUAString)
    return;

  PluginInstance* currentInstance = (PluginInstance*)(instance->pdata);
  CGContextRef cgContext = event->data.draw.context;
  if (!cgContext)
    return;

  float windowWidth = currentInstance->window.width;
  float windowHeight = currentInstance->window.height;
  
  
  CGContextSaveGState(cgContext);
  
  
  CGContextTranslateCTM(cgContext, 0.0, windowHeight);
  CGContextScaleCTM(cgContext, 1.0, -1.0);
  
  
  CGContextAddRect(cgContext, CGRectMake(0, 0, windowWidth, windowHeight));
  CGContextSetGrayFillColor(cgContext, 0.5, 1.0);
  CGContextDrawPath(cgContext, kCGPathFill);
  
  
  CGContextAddRect(cgContext, CGRectMake(0, 0, windowWidth, windowHeight));
  CGContextSetGrayStrokeColor(cgContext, 0.0, 1.0);
  CGContextSetLineWidth(cgContext, 6.0);
  CGContextStrokePath(cgContext);
  
  
  CGContextSetGrayFillColor(cgContext, 0.0, 1.0);
  ATSUStyle atsuStyle;
  ATSUCreateStyle(&atsuStyle);
  CFIndex stringLength = CFStringGetLength(browserUAString);
  UniChar* unicharBuffer = (UniChar*)malloc((stringLength + 1) * sizeof(UniChar));
  CFStringGetCharacters(browserUAString, CFRangeMake(0, stringLength), unicharBuffer);
  UniCharCount runLengths = kATSUToTextEnd;
  ATSUTextLayout atsuLayout;
  ATSUCreateTextLayoutWithTextPtr(unicharBuffer,
                                  kATSUFromTextBeginning,
                                  kATSUToTextEnd,
                                  stringLength,
                                  1,
                                  &runLengths,
                                  &atsuStyle,
                                  &atsuLayout);
  ATSUAttributeTag contextTag = kATSUCGContextTag;
  ByteCount byteSize = sizeof(CGContextRef);
  ATSUAttributeValuePtr contextATSUPtr = &cgContext;
  ATSUSetLayoutControls(atsuLayout, 1, &contextTag, &byteSize, &contextATSUPtr);
  ATSUTextMeasurement lineAscent, lineDescent;
  ATSUGetLineControl(atsuLayout,
                    kATSUFromTextBeginning,
                    kATSULineAscentTag,
                    sizeof(ATSUTextMeasurement),
                    &lineAscent,
                    &byteSize);
  ATSUGetLineControl(atsuLayout,
                    kATSUFromTextBeginning,
                    kATSULineDescentTag,
                    sizeof(ATSUTextMeasurement),
                    &lineDescent,
                    &byteSize);
  float lineHeight = FixedToFloat(lineAscent) + FixedToFloat(lineDescent);  
  ItemCount softBreakCount;
  ATSUBatchBreakLines(atsuLayout,
                      kATSUFromTextBeginning,
                      stringLength,
                      FloatToFixed(windowWidth - 10.0),
                      &softBreakCount);
  ATSUGetSoftLineBreaks(atsuLayout,
                        kATSUFromTextBeginning,
                        kATSUToTextEnd,
                        0, NULL, &softBreakCount);
  UniCharArrayOffset* softBreaks = (UniCharArrayOffset*)malloc(softBreakCount * sizeof(UniCharArrayOffset));
  ATSUGetSoftLineBreaks(atsuLayout,
                        kATSUFromTextBeginning,
                        kATSUToTextEnd,
                        softBreakCount, softBreaks, &softBreakCount);
  UniCharArrayOffset currentDrawOffset = kATSUFromTextBeginning;
  int i = 0;
  while (i < softBreakCount) {
    ATSUDrawText(atsuLayout, currentDrawOffset, softBreaks[i], FloatToFixed(5.0), FloatToFixed(windowHeight - 5.0 - (lineHeight * (i + 1.0))));
    currentDrawOffset = softBreaks[i];
    i++;
  }
  ATSUDrawText(atsuLayout, currentDrawOffset, kATSUToTextEnd, FloatToFixed(5.0), FloatToFixed(windowHeight - 5.0 - (lineHeight * (i + 1.0))));
  free(unicharBuffer);
  free(softBreaks);
  
  
  CGContextRestoreGState(cgContext);
}
