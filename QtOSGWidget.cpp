#include "QtOSGWidget.h"

#include <QThread>
#include <QApplication>
#include <QDesktopWidget>
#include <QWindow>
#include <QMouseEvent>
#include <QPainter>

#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osg/MatrixTransform>
#include <osgGA/MultiTouchTrackballManipulator>
#include <osgUtil/RayIntersector>

#include <QVBoxLayout>

#include <QOpenGLWidget>
#include <osgViewer/Viewer>
#include <osg/Camera>
#include <osg/ShapeDrawable>
#include <osg/StateSet>
#include <osg/Material>
#include <osgGA/EventQueue>
#include <osgGA/TrackballManipulator>

#include <osg/Texture2D>

#include <QOpenGLFunctions>
#include <QOffscreenSurface>

//
//USE_OSGPLUGIN(osg)
//USE_OSGPLUGIN(osg2)
//USE_OSGPLUGIN(rgb)
//USE_OSGPLUGIN(jpeg)
//
//USE_GRAPHICSWINDOW()


class RenderCompleteCallback : public osg::Camera::DrawCallback
{
public:
    RenderCompleteCallback(QtOSGWidget * w) : _widget(w){}
    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        if (_widget->_needResize)
        {
            if (renderInfo.getCurrentCamera())
            {
                renderInfo.getCurrentCamera()->resize(_widget->width() * _widget->m_scale,
                    _widget->height() * _widget->m_scale,
                    osg::Camera::RESIZE_PROJECTIONMATRIX | osg::Camera::RESIZE_DEFAULT);
            }
            _widget->_needResize = false;
        }
    }

    void setWidget(QtOSGWidget* widget) { _widget = widget; }
private:
    QtOSGWidget* _widget;
};
    
QtOSGWidget::QtOSGWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , _mViewer(new osgViewer::Viewer)
    , m_scale(QApplication::desktop()->devicePixelRatio())
{
    connect(this, &QtOSGWidget::moveContext, this, &QtOSGWidget::onMoveContext, Qt::BlockingQueuedConnection);
}


QtOSGWidget::~QtOSGWidget() {

}

void QtOSGWidget::onMoveContext(QThread* th)
{
    _mGraphicsWindow->moveContext(th);
}

void QtOSGWidget::onImageReady()
{

}

void QtOSGWidget::paintEvent(QPaintEvent* e)
{
    QOpenGLWidget::paintEvent(e);
}

void QtOSGWidget::resizeGL(int width, int height)
{
    this->getEventQueue()->windowResize(this->x() * m_scale, this->y() * m_scale, width * m_scale, height * m_scale, 0);
    _mGraphicsWindow->resized(this->x() * m_scale, this->y() * m_scale, width * m_scale, height * m_scale);
    osg::Camera* camera = _mViewer->getCamera();
    camera->setViewport(0,0,this->width() * m_scale, this->height() * m_scale);

    _needResize = true;
}

    
void QtOSGWidget::initializeGL() {

    _mViewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    _mViewer->setThreadingModel(osgViewer::Viewer::DrawThreadPerContext);


    osg::Group* rootNode = new osg::Group();

    osg::Cylinder* cylinder = new osg::Cylinder();
    osg::ShapeDrawable* sd = new osg::ShapeDrawable(cylinder);
    sd->setColor(osg::Vec4(0.8f, 0.5f, 0.2f, 1.f));
    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(sd);

    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    osg::Material* material = new osg::Material;
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    stateSet->setAttributeAndModes(material, osg::StateAttribute::ON);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);


    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
    traits->x = this->x();
    traits->y = this->y();
    traits->width = this->width();
    traits->height = this->height();
    traits->doubleBuffer = true;
    traits->sampleBuffers = true;
    traits->samples = 8;

    //_mGraphicsWindow = (new osgViewer::GraphicsWindowEmbedded(traits));
    _mGraphicsWindow = (new QtGraphicsWindow(traits));
    _mGraphicsWindow->_qglWidget = this;

    osg::Camera* cameraOrg = new osg::Camera;
    cameraOrg->setGraphicsContext(_mGraphicsWindow);
    cameraOrg->setViewport(0, 0, width(), height());

    const osg::BoundingSphere& bs = geode->getBound();
    if (!bs.valid())
        return ;
    float aspectRatio = static_cast<float>(this->width()) / static_cast<float>(this->height());

    if (_mViewer->getThreadingModel() != osgViewer::ViewerBase::ThreadingModel::SingleThreaded)
    {
        _camera = new osg::Camera;
        _camera->setViewport(0, 0, this->width(), this->height());
        _camera->setClearColor(osg::Vec4(0.0f, 0.0f, 1.f, 1.f));
        _camera->setProjectionMatrixAsPerspective(30.f, aspectRatio, 1.f, 1000.f);
        _camera->setViewMatrixAsLookAt(bs.center() - osg::Vec3(0.0f, 2.0f, 0.0f) * bs.radius(), bs.center(), osg::Vec3(0.0f, 0.0f, 1.0f));

        //_camera->setGraphicsContext(_mGraphicsWindow);


        _camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        _camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
        _camera->setRenderOrder(osg::Camera::PRE_RENDER);//最先渲染

        osg::ref_ptr<osg::Texture2D> colorTexture = new osg::Texture2D;
        //colorTexture->setTextureSize(width(), height());
        //colorTexture->setInternalFormat(GL_RGBA);
        //if (false)
        //{
        _image = new osg::Image();
        _image->allocateImage(width(), height(), 1, GL_RGB, GL_UNSIGNED_BYTE);
        _camera->attach(osg::Camera::COLOR_BUFFER, _image.get(), 8);

        _camera->addFinalDrawCallback(new RenderCompleteCallback(this));

        //    colorTexture->setImage(0, _image);
        //}
        //else
        //{
        //    camera->attach(osg::Camera::COLOR_BUFFER, colorTexture, 0, 0, false, 0, 0);
        //}


        _camera->addChild(geode);
        rootNode->addChild(_camera);
    }
    else
    {
        rootNode->addChild(geode);

        cameraOrg->setClearColor(osg::Vec4(0.0f, 0.0f, 1.f, 1.f));
        cameraOrg->setProjectionMatrixAsPerspective(30.f, aspectRatio, 1.f, 1000.f);
        cameraOrg->setViewMatrixAsLookAt(bs.center() - osg::Vec3(0.0f, 2.0f, 0.0f) * bs.radius(), bs.center(), osg::Vec3(0.0f, 0.0f, 1.0f));
    }

    _mViewer->setCamera(cameraOrg);
    _mViewer->setSceneData(rootNode);
    osgGA::TrackballManipulator* manipulator = new osgGA::TrackballManipulator;
    manipulator->setAllowThrow(false);
    this->setMouseTracking(true);
    _mViewer->setCameraManipulator(manipulator);
    _mViewer->realize();
}

#include <osgDB/WriteFile>
void QtOSGWidget::paintGL() {
    if(_mViewer->getThreadingModel() == osgViewer::Viewer::SingleThreaded)
        _mViewer->frame();
    else if (_mGraphicsWindow->isRealized())
    {
        _mViewer->getCameraManipulator()->updateCamera(*_camera);
        _mViewer->frame();

        QPainter p(this);
        QImage img((const uchar*)_image->data(), _image->s(), _image->t(), _image->getRowSizeInBytes(), QImage::Format_RGB888);
        img = img.mirrored(false, true);
        p.drawImage(QRect(0, 0, width(), height()), img);
        //osgDB::writeImageFile(*_image, "E:\\xxx.jpg");
    }
}
    
void QtOSGWidget::mouseMoveEvent(QMouseEvent* event)
{
    int x = event->x() * m_scale;
    int y = event->y() * m_scale;
    this->getEventQueue()->mouseMotion(x, y);
}

    
void QtOSGWidget::mousePressEvent(QMouseEvent* event)
{
    unsigned int button = 0;
    switch (event->button()) {
    case Qt::LeftButton:
        button = 1;
        break;
    case Qt::MiddleButton:
        button = 2;
        break;
    case Qt::RightButton:
        button = 3;
        break;
    default:
        break;
    }
    this->getEventQueue()->mouseButtonPress(event->x() * m_scale, event->y() * m_scale, button);
}

    
void QtOSGWidget::mouseReleaseEvent(QMouseEvent* event)
{
    unsigned int button = 0;
    switch (event->button()) {
    case Qt::LeftButton:
        button = 1;
        break;
    case Qt::MiddleButton:
        button = 2;
        break;
    case Qt::RightButton:
        button = 3;
        break;
    default:
        break;
    }
    this->getEventQueue()->mouseButtonRelease(event->x() * m_scale, event->y() * m_scale, button);
}

    
void QtOSGWidget::wheelEvent(QWheelEvent* event)
{
    int delta = event->delta();
    osgGA::GUIEventAdapter::ScrollingMotion motion = delta > 0 ?
        osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN;
    this->getEventQueue()->mouseScroll(motion, 0);
}

    
bool QtOSGWidget::event(QEvent* event)
{
    bool handled = QOpenGLWidget::event(event);
    this->update();
    return handled;
}

osgGA::EventQueue* QtOSGWidget::getEventQueue() const {
    osgGA::EventQueue* eventQueue = _mGraphicsWindow->getEventQueue();
    return eventQueue;
}


bool QtGraphicsWindow::valid() const 
{
    return true; 
}
bool QtGraphicsWindow::realizeImplementation()
{ 
    return true; 
}
bool QtGraphicsWindow::isRealizedImplementation() const
{ 
    return _realized;
}
void QtGraphicsWindow::closeImplementation()
{

}
bool QtGraphicsWindow::makeCurrentImplementation() 
{ 
    _qglWidget->context();

    QOpenGLContext* qglcx = _qglWidget->context();
    if (qglcx->thread() != QThread::currentThread()) {
        if (!qglcx->thread()) return true;//窗口关闭时
        //这是在另一个线程中做得，需要让主线程来movetothread，需要用信号槽机制告诉主线程

        emit _qglWidget->moveContext(QThread::currentThread());

        _realized = true;
        m_renderContext->makeCurrent(_qglWidget->context()->surface());
    }
    else {
        _realized = true;
        qglcx->makeCurrent(_qglWidget->context()->surface());
    }

    return true; 
}
bool QtGraphicsWindow::releaseContextImplementation() 
{
    return true; 
}
void QtGraphicsWindow::swapBuffersImplementation() 
{

}
void QtGraphicsWindow::grabFocus() 
{

}
void QtGraphicsWindow::grabFocusIfPointerInWindow() 
{

}
void QtGraphicsWindow::raiseWindow() 
{

}

void QtGraphicsWindow::moveContext(QThread* th)
{
    m_mainContext = _qglWidget->context();

    m_renderContext = new QOpenGLContext;
    m_renderContext->setFormat(m_mainContext->format());
    m_renderContext->setShareContext(m_mainContext);
    m_renderContext->create();
    m_renderContext->moveToThread(th);

}