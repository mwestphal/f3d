/**
 * @class   camera
 * @brief   Abstract class to control a camera in a camera
 *
 * A class to control a camera in a window, use only after the first render of the window.
 */

#ifndef f3d_camera_h
#define f3d_camera_h

#include "export.h"

#include <array>
#include <stdexcept>
#include <string>

namespace f3d
{
class F3D_EXPORT camera
{
public:
  class exception : public std::runtime_error
  {
  public:
    exception(const std::string& what = "")
      : std::runtime_error(what)
    {
    }
  };

  typedef std::array<double, 3> vector3_t;
  typedef std::array<double, 16> matrix4_t;

  //@{
  /**
   * Set/Get the canera parameters.
   * Call render on the window before calling any of these methods, or they will throw an exception.
   */
  virtual void setPosition(const vector3_t& pos) = 0;
  virtual vector3_t getPosition() = 0;
  virtual void setFocalPoint(const vector3_t& foc) = 0;
  virtual vector3_t getFocalPoint() = 0;
  virtual void setViewUp(const vector3_t& up) = 0;
  virtual vector3_t getViewUp() = 0;
  virtual void setViewAngle(const double& angle) = 0;
  virtual double getViewAngle() = 0;
  //@}

  //@{
  /**
   * Set/Get the camera view matrix. Please note that when using the view matrix API, using the
   * other camera manipulation API may results in unexepected results due to normalization. Call
   * render on the window before calling any of these methods, or they will throw an exception.
   */
  virtual void setViewMatrix(const matrix4_t& matrix) = 0;
  virtual matrix4_t getViewMatrix() = 0;
  //@}

  //@{
  /**
   * Standard camera manipulation methods.
   * Call render on the window before calling any of these methods, or they will throw an exception.
   */
  virtual void dolly(double val) = 0;
  virtual void roll(double angle) = 0;
  virtual void azimuth(double angle) = 0;
  virtual void yaw(double angle) = 0;
  virtual void elevation(double angle) = 0;
  virtual void pitch(double angle) = 0;
  //@}

  /**
   * Store the current camera configuration as default.
   * Call render on the window before calling this or it will throw an exception.
   */
  virtual void setCurrentAsDefault() = 0;

  /**
   * Reset the camera to the stored default camera configuration.
   * Call render on the window before calling this or it will throw an exception.
   */
  virtual void resetToDefault() = 0;

  /**
   * Reset the camera using the bounds of actors in the scene.
   * Call render on the window before calling this or it will throw an exception.
   */
  virtual void resetToBounds() = 0;

protected:
  camera() = default;
  virtual ~camera() = default;
  camera(const camera&) = delete;
  camera& operator=(const camera&) = delete;
};
}

#endif
