#pragma once

#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QSurface>

#include <osgViewer/Viewer>
#include <osg/Camera>
#include <osg/ShapeDrawable>
#include <osg/StateSet>
#include <osg/Material>
#include <osgGA/EventQueue>
#include <osgGA/TrackballManipulator>

class QtGraphicsWindow;

class QtOSGWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    QtOSGWidget(QWidget* parent = 0);
    virtual ~QtOSGWidget();

    void render();
signals:
    void moveContext(QThread * th);
    void imageReady();
protected slots:
    void onImageReady();
protected:

    void paintGL() override;
    void resizeGL(int width, int height) override;
    void initializeGL() override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    bool event(QEvent* event) override;

    void paintEvent(QPaintEvent* e) override;
public:

    osgGA::EventQueue* getEventQueue() const;

    osg::ref_ptr<osgViewer::Viewer> _mViewer;
    qreal m_scale;

    bool _needResize = false;
    osg::ref_ptr<osg::Camera> _camera = nullptr;
    osg::ref_ptr<osg::Image> _image = nullptr;
    mutable OpenThreads::Mutex  _mutex;

    int setupCamera();
};