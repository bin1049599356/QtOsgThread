qt osg 支持多线程模式


原理：

在qt主线程创建opengl上下文，movetothread 到 渲染线程，然后使用离屏渲染，把帧缓存图片画到主线程opengl上下文。
