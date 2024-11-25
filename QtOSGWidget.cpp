#include "QtOSGWidget.h"

#include <QThread>
#include <QApplication>
#include <QDesktopWidget>
#include <QWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QThread>

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
#include <osgViewer/ViewerEventHandlers>

#include <osgDB/WriteFile>
#include <QElapsedTimer>
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
    RenderCompleteCallback(QtOSGWidget * w, GLenum readBuffer) : _widget(w),
        _readBuffer(readBuffer){}
    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE)
        glReadBuffer(_readBuffer);
#else
        osg::notify(osg::NOTICE) << "Error: GLES unable to do glReadBuffer" << std::endl;
#endif

        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_widget->_mutex);
        osg::GraphicsContext* gc = renderInfo.getState()->getGraphicsContext();
        GLenum pixelFormat;
        if (gc->getTraits())
        {
            if (gc->getTraits()->alpha)
                pixelFormat = GL_RGBA;
            else
                pixelFormat = GL_RGB;

#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE)
            if (pixelFormat == GL_RGB)
            {
                GLint value = 0;
#ifndef GL_IMPLEMENTATION_COLOR_READ_FORMAT
#define GL_IMPLEMENTATION_COLOR_READ_FORMAT 0x8B9B
#endif
                glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &value);
                if (value != GL_RGB ||
                    value != GL_UNSIGNED_BYTE)
                {
                    pixelFormat = GL_RGBA;//always supported
                }
            }
#endif
            int width = gc->getTraits()->width;
            int height = gc->getTraits()->height;

            //std::cout << "Capture: size=" << width << "x" << height << ", format=" << (pixelFormat == GL_RGBA ? "GL_RGBA" : "GL_RGB") << std::endl;

            _widget->_image->readPixels(0, 0, width, height, pixelFormat, GL_UNSIGNED_BYTE);
        }



        //if (_widget->_needResize)
        //{
        //    if (renderInfo.getCurrentCamera())
        //    {
        //        renderInfo.getCurrentCamera()->resize(_widget->width() * _widget->m_scale,
        //            _widget->height() * _widget->m_scale,
        //            osg::Camera::RESIZE_PROJECTIONMATRIX | osg::Camera::RESIZE_DEFAULT);
        //    }
            //_widget->_needResize = false;

        //    emit _widget->imageReady();
            //osgDB::writeImageFile(*_widget->_image, "E:\\test.jpg");
        //}
    }

    void setWidget(QtOSGWidget* widget) { _widget = widget; }
private:
    GLenum                      _readBuffer;
    QtOSGWidget* _widget;
};
    
QtOSGWidget::QtOSGWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , _mViewer(new osgViewer::Viewer)
    , m_scale(QApplication::desktop()->devicePixelRatio())
{


    auto pStatsEventHandler = new osgViewer::StatsHandler; // 构造一视景器统计事件处理器
    _mViewer->addEventHandler(pStatsEventHandler);

    //_mViewer->setUpViewInWindow(0, 0, width(), height());
    _mViewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    _mViewer->setThreadingModel(osgViewer::Viewer::ThreadPerCamera);



    osg::Group* rootNode = new osg::Group();
    osg::Group* showNode = new osg::Group();

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

    osg::Matrix mat;
    for (int i = 0; i < 10000; i++)
    {
        mat = osg::Matrix::translate(sin(i * 10) * 1, cos(i * 10) * 1, sin(i * 10) * 1) * osg::Matrix::rotate(0.1, osg::Vec3d(sin(i * 10) * 1, cos(i * 10) * 1, sin(i * 10) * 1));
        osg::ref_ptr<osg::MatrixTransform> node = new osg::MatrixTransform;
        node->addChild(geode);
        node->setMatrix(mat);

        showNode->addChild(node);
    }

    _image = new osg::Image();
    //_image->allocateImage(width(), height(), 1, GL_RGB, GL_UNSIGNED_BYTE);

    const osg::BoundingSphere& bs = showNode->getBound();
    if (!bs.valid())
        return;
    float aspectRatio = static_cast<float>(this->width()) / static_cast<float>(this->height());


    setupCamera();

    _camera->setClearColor(osg::Vec4(0.0f, 0.0f, 1.f, 1.f));
    _camera->setProjectionMatrixAsPerspective(30.f, aspectRatio, 1.f, 1000.f);
    _camera->setViewMatrixAsLookAt(bs.center() - osg::Vec3(0.0f, 100.0f, 0.0f) * bs.radius(), bs.center(), osg::Vec3(0.0f, 0.0f, 1.0f));

    rootNode->addChild(showNode);

    _mViewer->setSceneData(rootNode);
    osgGA::TrackballManipulator* manipulator = new osgGA::TrackballManipulator;
    manipulator->setDistance(100);
    manipulator->setCenter(bs.center());
    manipulator->setAllowThrow(false);
    this->setMouseTracking(true);
    _mViewer->setCameraManipulator(manipulator, false);
    _mViewer->realize();

    class WorkerThread : public QThread
    {
    public:
        explicit WorkerThread(QObject* parent = nullptr) : QThread(parent), m_isStop(false) {}
        void run() {
            _w->render();
            //_w->_mViewer->run();
        }
        void stop() { m_isStop = true; }

        QtOSGWidget* _w;
    private:
        bool m_isStop;//停止标志位  
    };

    auto thread = new WorkerThread(this);
    thread->_w = this;
    thread->start(QThread::HighPriority);
}


QtOSGWidget::~QtOSGWidget() {

}

void QtOSGWidget::onImageReady()
{
    update();
}

void QtOSGWidget::paintEvent(QPaintEvent* e)
{
    QOpenGLWidget::paintEvent(e);
}

void QtOSGWidget::resizeGL(int width, int height)
{
    osg::Camera* camera = _mViewer->getCamera();
    this->getEventQueue()->windowResize(this->x() * m_scale, this->y() * m_scale, width * m_scale, height * m_scale, 0);
    camera->getGraphicsContext()->resized(this->x() * m_scale, this->y() * m_scale, width * m_scale, height * m_scale);
    camera->setViewport(0,0,this->width() * m_scale, this->height() * m_scale);


    _needResize = true;
}


int QtOSGWidget::setupCamera()
{
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->readDISPLAY();
    traits->setUndefinedScreenDetailsToDefaultScreen();
    traits->width = width();
    traits->height = height();
    traits->pbuffer = true;
    traits->sampleBuffers = true;
    traits->samples = 8;

    osg::ref_ptr<osg::GraphicsContext> pbuffer = osg::GraphicsContext::createGraphicsContext(traits.get());
    if (pbuffer.valid())
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera(*_mViewer->getCamera());
        camera->setGraphicsContext(pbuffer.get());
        camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
        GLenum buffer = pbuffer->getTraits()->doubleBuffer ? GL_BACK : GL_FRONT;
        camera->setDrawBuffer(buffer);
        camera->setReadBuffer(buffer);
        if(camera->getFinalDrawCallback() == nullptr)
            camera->setFinalDrawCallback(new RenderCompleteCallback(this, buffer));
        _camera = camera;

        _mViewer->setCamera(_camera.get());
    }

    return 0;
}
    
void QtOSGWidget::initializeGL() {

    //std::thread th([&]() {
    //    render();
    //    }
    //);

    //th.detach();
}

void QtOSGWidget::render()
{
    //m_renderContext->makeCurrent(context()->surface());

    while (!_mViewer->done())
    {
        QElapsedTimer  timer;
        timer.start();

        //if (_camera)
        //    _mViewer->getCameraManipulator()->updateCamera(*_camera);

        if (_needResize)
        {
            _mViewer->stopThreading();

            setupCamera();
            _mViewer->startThreading();

            _needResize = false;
        }
        _mViewer->frame();

        qint64 elapsedTime = timer.elapsed();
        qDebug() << "time one frame:" << elapsedTime << "ms";
    }
}

void QtOSGWidget::paintGL() {

    QElapsedTimer  timer;
    timer.start();

    //if(_mViewer->getThreadingModel() == osgViewer::Viewer::SingleThreaded)
    //    _mViewer->frame();
    //else if (_mGraphicsWindow->isRealized())
    //{
    //    _mViewer->getCameraManipulator()->updateCamera(*_camera);
    //    _mViewer->frame();

    //    QPainter p(this);
    //    QImage img((const uchar*)_image->data(), _image->s(), _image->t(), _image->getRowSizeInBytes(), QImage::Format_RGB888);
    //    img = img.mirrored(false, true);
    //    p.drawImage(QRect(0, 0, width(), height()), img);
    //    //osgDB::writeImageFile(*_image, "E:\\xxx.jpg");
    //}

    //return;

        QPainter p(this);
        QImage img((const uchar*)_image->data(), _image->s(), _image->t(), _image->getRowSizeInBytes(), QImage::Format_RGB888);
        img = img.mirrored(false, true);
        p.drawImage(QRect(0, 0, width(), height()), img);

        qint64 elapsedTime = timer.elapsed();
        qDebug() << "time paintGL frame:" << elapsedTime << "ms";
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
    osgGA::EventQueue* eventQueue = _mViewer->getEventQueue();
    return eventQueue;
}
