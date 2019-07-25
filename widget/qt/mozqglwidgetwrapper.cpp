





#include "mozqglwidgetwrapper.h"
#include <QGraphicsView>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL/QGLContext>

MozQGLWidgetWrapper::MozQGLWidgetWrapper()
  : mWidget(new QGLWidget())
{
}

MozQGLWidgetWrapper::~MozQGLWidgetWrapper()
{
    delete mWidget;
}

void MozQGLWidgetWrapper::makeCurrent()
{
    mWidget->makeCurrent();
}

void MozQGLWidgetWrapper::setViewport(QGraphicsView* aView)
{
    aView->setViewport(mWidget);
}

bool MozQGLWidgetWrapper::hasGLContext(QGraphicsView* aView)
{
    return aView && qobject_cast<QGLWidget*>(aView->viewport());
}

bool MozQGLWidgetWrapper::isRGBAContext()
{
    QGLContext* context = const_cast<QGLContext*>(QGLContext::currentContext());
    return context && context->format().alpha();
}