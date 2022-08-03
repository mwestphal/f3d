/**
 * @class   camera
 * @brief   Abstract class to control a camera in a window
 *
 * A class to control a camera in a window.
 * There is in essence three API.
 * The Position/FocalPoint/ViewUp API, the ViewMatrix API and the camera movements API.
 * They can be mixed by using ViewMatrix API with the other APIs but it can result in unexpected
 * results.
 * When using the Position/FocalPoint/ViewUp API or the camera movements API,
 * the view up is automatically orthogonalized.
 */

#ifndef f3d_camera_h
#define f3d_camera_h

#include "export.h"

#include <array>
#include <string>

namespace f3d
{
class F3D_EXPORT camera
{
public:
  typedef std::array<double, 3> vector3_t;
  typedef std::array<double, 16> matrix4_t;

  //@{
  /**
   * Set/Get the camera parameters.
   */
  virtual camera& setPosition(const vector3_t& pos) noexcept = 0;
  virtual vector3_t getPosition() noexcept = 0;
  virtual camera& getPosition(vector3_t& pos) noexcept = 0;
  virtual camera& setFocalPoint(const vector3_t& foc) noexcept = 0;
  virtual vector3_t getFocalPoint() noexcept = 0;
  virtual camera& getFocalPoint(vector3_t& foc) noexcept = 0;
  virtual camera& setViewUp(const vector3_t& up) noexcept = 0;
  virtual vector3_t getViewUp() noexcept = 0;
  virtual camera& getViewUp(vector3_t& up) noexcept = 0;
  virtual camera& setViewAngle(const double& angle) noexcept = 0;
  virtual double getViewAngle() noexcept = 0;
  virtual camera& getViewAngle(double& angle) noexcept = 0;
  //@}

  //@{
  /**
   * Set/Get the camera view matrix. Please note that when using the view matrix API, using the
   * other camera manipulation API may results in unexepected results due to normalization.
   */
  virtual camera& setViewMatrix(const matrix4_t& matrix) noexcept = 0;
  virtual matrix4_t getViewMatrix() noexcept = 0;
  virtual camera& getViewMatrix(matrix4_t& matrix) noexcept = 0;
  //@}

  //@{
  /**
   * Standard camera manipulation methods.
   */
  virtual camera& dolly(double val) noexcept = 0;
  virtual camera& roll(double angle) noexcept = 0;
  virtual camera& azimuth(double angle) noexcept = 0;
  virtual camera& yaw(double angle) noexcept = 0;
  virtual camera& elevation(double angle) noexcept = 0;
  virtual camera& pitch(double angle) noexcept = 0;
  //@}

  /**
   * Store the current camera configuration as default.
   */
  virtual camera& setCurrentAsDefault() noexcept = 0;

  /**
   * Reset the camera to the stored default camera configuration.
   */
  virtual camera& resetToDefault() noexcept = 0;

  /**
   * Reset the camera using the bounds of actors in the scene.
   */
  virtual camera& resetToBounds() noexcept = 0;

protected:
  camera() noexcept = default;
  virtual ~camera() noexcept = default;
  camera(const camera&) noexcept = delete;
  camera& operator=(const camera&) noexcept = delete;
};
}

#endif
