#include <engine.h>
#include <camera.h>
#include <window.h>

#include <cmath>
#include <iostream>
#include <iomanip>

bool compareDouble(double a, double b)
{
  return std::fabs(a - b) < 128*std::numeric_limits<double>::epsilon();
}

bool compareVec(f3d::camera::vector_t vec1, f3d::camera::vector_t vec2)
{
  return compareDouble(vec1[0], vec2[0]) && compareDouble(vec1[1], vec2[1]) && compareDouble(vec1[2], vec2[2]);
}

int TestSDKCamera(int argc, char* argv[])
{
  f3d::engine eng;
  f3d::window& win = eng.getWindow();
  f3d::camera& cam = win.getCamera();
  win.render();

  // Test position
  f3d::camera::vector_t testPos = {0., 0., 10.};
  cam.setPosition(testPos);
  f3d::camera::vector_t pos = cam.getPosition();
  if (pos != testPos)
  {
    std::cerr << "set/get position is not behaving as expected: " << pos[0] << "," << pos[1] << "," << pos[2] << std::endl;
    return EXIT_FAILURE;
  }

  // Test focal point
  f3d::camera::vector_t testFoc = {0., 0., -1.};
  cam.setFocalPoint(testFoc);
  f3d::camera::vector_t foc = cam.getFocalPoint();
  if (foc != testFoc)
  {
    std::cerr << "set/get focal point is not behaving as expected: " << foc[0] << "," << foc[1] << "," << foc[2] << std::endl;
    return EXIT_FAILURE;
  }

  // Test view up
  f3d::camera::vector_t testUp = {1., 0., 0.};
  cam.setViewUp(testUp);
  f3d::camera::vector_t up = cam.getViewUp();
  if (up != testUp)
  {
    std::cerr << "set/get view up is not behaving as expected: " << up[0] << "," << up[1] << "," << up[2] << std::endl;
    return EXIT_FAILURE;
  }

  // Test view angle
  double testAngle = 20;
  cam.setViewAngle(testAngle);
  double angle = cam.getViewAngle();
  if (angle != testAngle)
  {
    std::cerr << "set/get view angle is not behaving as expected: " << angle << std::endl;
    return EXIT_FAILURE;
  }

  // Test azimuth
  cam.azimuth(90);
  f3d::camera::vector_t expectedPos = {0., -11., -1.};
  f3d::camera::vector_t expectedFoc = {1., 0., 0.};
  f3d::camera::vector_t expectedUp = {0., 0., -1.};
  pos = cam.getPosition();
  foc = cam.getViewUp();
  up = cam.getFocalPoint();
  if (!compareVec(pos, expectedPos) || !compareVec(foc, expectedFoc)|| !compareVec(up, expectedUp))
  {
    std::cerr << "Azimuth is not behaving as expected: " << std::endl;
    std::cerr << std::setprecision(12) << "position: " << pos[0] << "," << pos[1] << "," << pos[2] << std::endl;
    std::cerr << std::setprecision(12) << "focal point: " << foc[0] << "," << foc[1] << "," << foc[2] << std::endl;
    std::cerr << std::setprecision(12) << "view up: " << up[0] << "," << up[1] << "," << up[2] << std::endl;
    return EXIT_FAILURE;
  }

  // Test roll
  cam.roll(90);
  expectedFoc = {0., 0., -1.};
  pos = cam.getPosition();
  foc = cam.getViewUp();
  up = cam.getFocalPoint();
  if (!compareVec(pos, expectedPos) || !compareVec(foc, expectedFoc)|| !compareVec(up, expectedUp))
  {
    std::cerr << "Roll is not behaving as expected: " << std::endl;
    std::cerr << std::setprecision(12) << "position: " << pos[0] << "," << pos[1] << "," << pos[2] << std::endl;
    std::cerr << std::setprecision(12) << "focal point: " << foc[0] << "," << foc[1] << "," << foc[2] << std::endl;
    std::cerr << std::setprecision(12) << "view up: " << up[0] << "," << up[1] << "," << up[2] << std::endl;
    return EXIT_FAILURE;
  }

  // Test yaw
  cam.yaw(90);
  expectedUp = {11., -11., -1.};
  pos = cam.getPosition();
  foc = cam.getViewUp();
  up = cam.getFocalPoint();
  if (!compareVec(pos, expectedPos) || !compareVec(foc, expectedFoc)|| !compareVec(up, expectedUp))
  {
    std::cerr << "Yaw is not behaving as expected: " << std::endl;
    std::cerr << std::setprecision(12) << "position: " << pos[0] << "," << pos[1] << "," << pos[2] << std::endl;
    std::cerr << std::setprecision(12) << "focal point: " << foc[0] << "," << foc[1] << "," << foc[2] << std::endl;
    std::cerr << std::setprecision(12) << "view up: " << up[0] << "," << up[1] << "," << up[2] << std::endl;
    return EXIT_FAILURE;
  }

  // Test elevation
  cam.elevation(90);
  expectedPos = {11., -11., -12.};
  expectedFoc = {1., 0., 0.};
  pos = cam.getPosition();
  foc = cam.getViewUp();
  up = cam.getFocalPoint();
  if (!compareVec(pos, expectedPos) || !compareVec(foc, expectedFoc)|| !compareVec(up, expectedUp))
  {
    std::cerr << "Elevation is not behaving as expected: " << std::endl;
    std::cerr << std::setprecision(12) << "position: " << pos[0] << "," << pos[1] << "," << pos[2] << std::endl;
    std::cerr << std::setprecision(12) << "focal point: " << foc[0] << "," << foc[1] << "," << foc[2] << std::endl;
    std::cerr << std::setprecision(12) << "view up: " << up[0] << "," << up[1] << "," << up[2] << std::endl;
    return EXIT_FAILURE;
  }

  // Test pitch
  cam.pitch(90);
  expectedFoc = {0., 0., -1.};
  expectedUp = {22., -11., -12.};
  pos = cam.getPosition();
  foc = cam.getViewUp();
  up = cam.getFocalPoint();
  if (!compareVec(pos, expectedPos) || !compareVec(foc, expectedFoc)|| !compareVec(up, expectedUp))
  {
    std::cerr << "Pitch is not behaving as expected: " << std::endl;
    std::cerr << std::setprecision(12) << "position: " << pos[0] << "," << pos[1] << "," << pos[2] << std::endl;
    std::cerr << std::setprecision(12) << "focal point: " << foc[0] << "," << foc[1] << "," << foc[2] << std::endl;
    std::cerr << std::setprecision(12) << "view up: " << up[0] << "," << up[1] << "," << up[2] << std::endl;
    return EXIT_FAILURE;
  }

  // TODO get/set view matrix once fixed

  return EXIT_SUCCESS;
}
