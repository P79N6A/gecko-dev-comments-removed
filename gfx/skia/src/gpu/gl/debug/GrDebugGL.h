







#ifndef GrDebugGL_DEFINED
#define GrDebugGL_DEFINED

#include "SkTArray.h"
#include "gl/GrGLInterface.h"

class GrFakeRefObj;
class GrTextureUnitObj;
class GrBufferObj;
class GrTextureObj;
class GrFrameBufferObj;
class GrRenderBufferObj;
class GrProgramObj;




class GrDebugGL {
public:
    enum GrObjTypes {
        kTexture_ObjTypes = 0,
        kBuffer_ObjTypes,
        kRenderBuffer_ObjTypes,
        kFrameBuffer_ObjTypes,
        kShader_ObjTypes,
        kProgram_ObjTypes,
        kTextureUnit_ObjTypes,
        kObjTypeCount
    };

    GrFakeRefObj *createObj(GrObjTypes type) {
        GrFakeRefObj *temp = (*gFactoryFunc[type])();

        fObjects.push_back(temp);

        return temp;
    }

    GrFakeRefObj *findObject(GrGLuint ID, GrObjTypes type);

    GrGLuint getMaxTextureUnits() const { return kDefaultMaxTextureUnits; }

    void setCurTextureUnit(GrGLuint curTextureUnit) { fCurTextureUnit = curTextureUnit; }
    GrGLuint getCurTextureUnit() const { return fCurTextureUnit; }

    GrTextureUnitObj *getTextureUnit(int iUnit) {
        GrAlwaysAssert(0 <= iUnit && kDefaultMaxTextureUnits > iUnit);

        return fTextureUnits[iUnit];
    }

    void setArrayBuffer(GrBufferObj *arrayBuffer);
    GrBufferObj *getArrayBuffer()                   { return fArrayBuffer; }

    void setElementArrayBuffer(GrBufferObj *elementArrayBuffer);
    GrBufferObj *getElementArrayBuffer()                            { return fElementArrayBuffer; }

    void setTexture(GrTextureObj *texture);

    void setFrameBuffer(GrFrameBufferObj *frameBuffer);
    GrFrameBufferObj *getFrameBuffer()                  { return fFrameBuffer; }

    void setRenderBuffer(GrRenderBufferObj *renderBuffer);
    GrRenderBufferObj *getRenderBuffer()                  { return fRenderBuffer; }

    void useProgram(GrProgramObj *program);

    void setPackRowLength(GrGLint packRowLength) {
        fPackRowLength = packRowLength;
    }
    GrGLint getPackRowLength() const { return fPackRowLength; }

    void setUnPackRowLength(GrGLint unPackRowLength) {
        fUnPackRowLength = unPackRowLength;
    }
    GrGLint getUnPackRowLength() const { return fUnPackRowLength; }

    static GrDebugGL *getInstance() {
        
        GrAssert(0 < gStaticRefCount);

        if (NULL == gObj) {
            gObj = SkNEW(GrDebugGL);
        }

        return gObj;
    }

    void report() const;

    static void staticRef() {
        gStaticRefCount++;
    }

    static void staticUnRef() {
        GrAssert(gStaticRefCount > 0);
        gStaticRefCount--;
        if (0 == gStaticRefCount) {
            SkDELETE(gObj);
            gObj = NULL;
        }
    }

protected:

private:
    
    static const GrGLint kDefaultMaxTextureUnits = 8;

    GrGLint         fPackRowLength;
    GrGLint         fUnPackRowLength;
    GrGLuint        fMaxTextureUnits;
    GrGLuint        fCurTextureUnit;
    GrBufferObj *   fArrayBuffer;
    GrBufferObj *   fElementArrayBuffer;
    GrFrameBufferObj *fFrameBuffer;
    GrRenderBufferObj *fRenderBuffer;
    GrProgramObj *  fProgram;
    GrTextureObj *  fTexture;
    GrTextureUnitObj *fTextureUnits[kDefaultMaxTextureUnits];

    typedef GrFakeRefObj *(*Create)();

    static Create gFactoryFunc[kObjTypeCount];

    static GrDebugGL* gObj;
    static int gStaticRefCount;

    
    SkTArray<GrFakeRefObj *> fObjects;

    GrDebugGL();
    ~GrDebugGL();
};




#define GR_CREATE(className, classEnum)                     \
    reinterpret_cast<className *>(GrDebugGL::getInstance()->createObj(classEnum))



#define GR_FIND(id, className, classEnum)                   \
    reinterpret_cast<className *>(GrDebugGL::getInstance()->findObject(id, classEnum))

#endif 
