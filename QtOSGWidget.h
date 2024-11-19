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


signals:
    void moveContext(QThread * th);
    void imageReady();
protected slots:
    void onMoveContext(QThread* th);
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

    //osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _mGraphicsWindow;
    osg::ref_ptr<QtGraphicsWindow> _mGraphicsWindow;
    osg::ref_ptr<osgViewer::Viewer> _mViewer;
    qreal m_scale;

    unsigned m_vao = 0;
    unsigned m_vbo = 0;
    std::unique_ptr<QOpenGLShaderProgram> m_program;


    bool _needResize = false;
    osg::Camera* _camera = nullptr;
    osg::ref_ptr<osg::Image> _image = nullptr;
};


class QtGraphicsWindow : public osgViewer::GraphicsWindow
{
public:

    QtGraphicsWindow(osg::GraphicsContext::Traits* traits = 0)
    {
        _traits = traits;
        init();
    }

    QtGraphicsWindow(int x, int y, int width, int height)
    {
        _traits = new GraphicsContext::Traits;
        _traits->x = x;
        _traits->y = y;
        _traits->width = width;
        _traits->height = height;
        init();
    }

    virtual bool isSameKindAs(const Object* object) const { return dynamic_cast<const QtGraphicsWindow*>(object) != 0; }
    virtual const char* libraryName() const { return "osgViewer"; }
    virtual const char* className() const { return "QtGraphicsWindow"; }

    void init()
    {
        if (valid())
        {
            setState(new osg::State);
            getState()->setGraphicsContext(this);

            if (_traits.valid() && _traits->sharedContext.valid())
            {
                getState()->setContextID(_traits->sharedContext->getState()->getContextID());
                incrementContextIDUsageCount(getState()->getContextID());
            }
            else
            {
                getState()->setContextID(osg::GraphicsContext::createNewContextID());
            }
        }
    }

    virtual bool valid() const;
    virtual bool realizeImplementation();
    virtual bool isRealizedImplementation() const;
    virtual void closeImplementation();
    virtual bool makeCurrentImplementation();
    virtual bool releaseContextImplementation();
    virtual void swapBuffersImplementation();
    virtual void grabFocus();
    virtual void grabFocusIfPointerInWindow();
    virtual void raiseWindow();


    QtOSGWidget* _qglWidget;


    void moveContext(QThread* th);
private:
    bool _realized = false;


    QOpenGLContext* m_mainContext;
    QOpenGLContext* m_renderContext = nullptr;
    QSurface* m_surface;
};