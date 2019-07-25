





#ifndef MOZQGLWIDGETWRAPPER_H
#define MOZQGLWIDGETWRAPPER_H






class QGLWidget;
class QGraphicsView;
class MozQGLWidgetWrapper
{
public:
    MozQGLWidgetWrapper();
    ~MozQGLWidgetWrapper();
    void makeCurrent();
    void setViewport(QGraphicsView*);
    static bool hasGLContext(QGraphicsView*);
    static bool isRGBAContext();
private:
    QGLWidget* mWidget;
};

#endif
