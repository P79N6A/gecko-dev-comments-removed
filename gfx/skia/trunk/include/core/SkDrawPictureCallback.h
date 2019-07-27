






#ifndef SkDrawPictureCallback_DEFINED
#define SkDrawPictureCallback_DEFINED











class SK_API SkDrawPictureCallback {
public:
    SkDrawPictureCallback() {}
    virtual ~SkDrawPictureCallback() {}

    virtual bool abortDrawing() = 0;
};

#endif
