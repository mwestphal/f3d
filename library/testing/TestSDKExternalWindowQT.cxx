#include <engine.h>
#include <loader.h>
#include <window.h>

#include <QGuiApplication>
#include <QKeyEvent>
#include <QOpenGLWindow>

#include "TestSDKHelpers.h"

#include <utility>

class F3DWindow : public QOpenGLWindow
{
public:
  F3DWindow(std::string filePath, std::string baselinePath, std::string outputPath)
    : QOpenGLWindow()
    , mFilePath(std::move(filePath))
    , mBaselinePath(std::move(baselinePath))
    , mOutputPath(std::move(outputPath))
  {
  }

protected:
  void keyPressEvent(QKeyEvent* event) override
  {
    if (event->key() == Qt::Key_Escape)
    {
      this->close();
    }
  }

  void timerEvent(QTimerEvent* event) override
  {
    // before closing, compare the content of the framebuffer with the baseline
    QImage frameBuffer = grabFramebuffer().mirrored().convertToFormat(QImage::Format_RGB888);

    f3d::image img(frameBuffer.width(), frameBuffer.height(), 3);
    img.setContent(frameBuffer.bits());

    if (!TestSDKHelpers::RenderTest(
          img, this->mBaselinePath, this->mOutputPath, "TestSDKExternalWindowQT"))
    {
      QCoreApplication::exit(EXIT_FAILURE);
    }

    this->close();
  }

  void initializeGL() override
  {
    this->QOpenGLWindow::initializeGL();

    auto loadFunc = [](void* userData, const char* name) -> f3d::window::F3DOpenGLAPIProc {
      if (auto* context = reinterpret_cast<QOpenGLContext*>(userData))
      {
        if (auto* symbol = context->getProcAddress(name))
        {
          return symbol;
        }
      }
      return nullptr;
    };

    mEngine = f3d::engine::createExternal(loadFunc, this->context());
    f3d::loader& load = mEngine->getLoader();
    load.loadGeometry(this->mFilePath);

//    mEngine.getWindow().initializeExternal(loadFunc, this->context());
  }

  void paintGL() override
  {
    mEngine->getWindow().render();
  }

private:
  std::shared_ptr<f3d::engine> mEngine;
  std::string mFilePath;
  std::string mBaselinePath;
  std::string mOutputPath;
};

int TestSDKExternalWindowQT(int argc, char* argv[])
{
  QGuiApplication a(argc, argv);

  F3DWindow w(
    std::string(argv[1]) + "/data/cow.vtp", std::string(argv[1]) + "/baselines/", argv[2]);
  w.setTitle("F3D QT External Window");
  w.resize(300, 300);
  w.startTimer(1000);

  w.show();

  return a.exec();
}
