#include "animationManager.h"

#include "interactor_impl.h"
#include "log.h"
#include "options.h"
#include "window.h"

#include <vtkDoubleArray.h>
#include <vtkImporter.h>
#include <vtkProgressBarRepresentation.h>
#include <vtkVersion.h>

#include <cmath>
#include <functional>

namespace f3d::detail
{
//----------------------------------------------------------------------------
bool animationManager::Initialize(
  const options* options, window* window, interactor_impl* interactor, vtkImporter* importer)
{
  assert(importer);
  this->HasAnimation = false;
  this->Playing = false;
  this->CurrentTime = 0;
  this->CurrentTimeSet = false;
  this->Options = options;
  this->Interactor = interactor;
  this->Window = window;
  this->Importer = importer;

  // This can be -1 if animation support is not implemented in the importer
  this->AvailAnimations = this->Importer->GetNumberOfAnimations();

  if (this->AvailAnimations > 0 && interactor)
  {
    this->ProgressWidget = vtkSmartPointer<vtkProgressBarWidget>::New();
    interactor->SetInteractorOn(this->ProgressWidget);

    vtkProgressBarRepresentation* progressRep =
      vtkProgressBarRepresentation::SafeDownCast(this->ProgressWidget->GetRepresentation());
    progressRep->SetProgressRate(0.0);
    progressRep->ProportionalResizeOff();
    progressRep->SetPosition(0.0, 0.0);
    progressRep->SetPosition2(1.0, 0.0);
    progressRep->SetMinimumSize(0, 5);
    progressRep->SetProgressBarColor(1, 0, 0);
    progressRep->DrawBackgroundOff();
    progressRep->DragableOff();
    progressRep->SetShowBorderToOff();
// Complete vtkProgressBarRepresentation needs
// https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7359
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 0, 20201027)
    progressRep->DrawFrameOff();
    progressRep->SetPadding(0.0, 0.0);
#endif
    progressRep->SetVisibility(options->getAsBool("ui.animation-progress"));

    this->ProgressWidget->On();
  }
  else
  {
    this->ProgressWidget = nullptr;
  }

  int animationIndex = options->getAsInt("scene.animation.index");
  double animationTime = options->getAsDouble("scene.animation.time");

  if (this->AvailAnimations <= 0)
  {
    log::debug("No animation available in this file");
    if (animationIndex > 0)
    {
      log::warn("An animation index has been specified but there are no animation available.");
    }
    if (animationTime != 0)
    {
      log::warn("No animation available, cannot load a specific animation time");
    }

    return false;
  }
  else
  {
    log::debug("Animation(s) available in this file are:");
  }
  for (int i = 0; i < this->AvailAnimations; i++)
  {
    log::debug(i, ": ", this->Importer->GetAnimationName(i));
  }
  log::debug("");

  this->AnimationIndex = options->getAsInt("scene.animation.index");
  if (this->AnimationIndex > 0 && this->AnimationIndex >= this->AvailAnimations)
  {
    log::warn(
      "Specified animation index is greater than the highest possible animation index, enabling "
      "the first animation.");
    this->AnimationIndex = 0;
  }
  this->EnableOnlyCurrentAnimation();

  // Recover time ranges for all enabled animations
  this->TimeRange[0] = std::numeric_limits<double>::infinity();
  this->TimeRange[1] = -std::numeric_limits<double>::infinity();
  for (vtkIdType animIndex = 0; animIndex < this->AvailAnimations; animIndex++)
  {
    if (this->Importer->IsAnimationEnabled(animIndex))
    {
      double timeRange[2];
      int nbTimeSteps;
      vtkNew<vtkDoubleArray> timeSteps;

// Complete GetTemporalInformation needs https://gitlab.kitware.com/vtk/vtk/-/merge_requests/7246
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 0, 20201016)
      // Discard timesteps, F3D only cares about real elapsed time using time range
      // Specifying the frame rate in the next call is not needed after VTK 9.2.20230603 :
      // VTK_VERSION_CHECK(9, 2, 20230603)
      double frameRate = this->Options->getAsDouble("scene.animation.frame-rate");
      this->Importer->GetTemporalInformation(
        animIndex, frameRate, nbTimeSteps, timeRange, timeSteps);
#else
      this->Importer->GetTemporalInformation(animIndex, nbTimeSteps, timeRange, timeSteps);
#endif
      // Accumulate time ranges
      this->TimeRange[0] = std::min(timeRange[0], this->TimeRange[0]);
      this->TimeRange[1] = std::max(timeRange[1], this->TimeRange[1]);
      this->HasAnimation = true;
    }
  }
  if (this->TimeRange[0] >= this->TimeRange[1])
  {
    log::warn("Animation(s) time range delta is invalid: [", this->TimeRange[0], ", ",
      this->TimeRange[1], "]. Disabling animation.");
    this->HasAnimation = false;
    return false;
  }
  else
  {
    log::debug("Animation(s) time range is: [", this->TimeRange[0], ", ", this->TimeRange[1], "].");
  }

  bool autoplay = options->getAsBool("scene.animation.autoplay");
  if (autoplay)
  {
    this->StartAnimation();
  }
  return true;
}

//----------------------------------------------------------------------------
void animationManager::StartAnimation()
{
  if (!this->IsPlaying())
  {
    this->ToggleAnimation();
  }
}

//----------------------------------------------------------------------------
void animationManager::StopAnimation()
{
  if (this->IsPlaying())
  {
    this->ToggleAnimation();
  }
}

//----------------------------------------------------------------------------
void animationManager::ToggleAnimation()
{
  if (this->HasAnimation && this->Interactor)
  {
    this->Playing = !this->Playing;

    if (this->CallBackId != 0)
    {
      this->Interactor->removeTimerCallBack(this->CallBackId);
    }
    if (this->Playing)
    {
      // Initialize time if not already
      if (!this->CurrentTimeSet)
      {
        this->CurrentTime = this->TimeRange[0];
        this->CurrentTimeSet = true;
      }

      // Always reset previous tick when starting the animation
      this->PreviousTick = std::chrono::steady_clock::now();

      double frameRate = this->Options->getAsDouble("scene.animation.frame-rate");
      this->CallBackId =
        this->Interactor->createTimerCallBack(1000.0 / frameRate, [this]() { this->Tick(); });
    }

    if (this->Playing && this->Options->getAsInt("scene.camera.index") >= 0)
    {
      this->Interactor->disableCameraMovement();
    }
    else
    {
      this->Interactor->enableCameraMovement();
    }
  }
}

//----------------------------------------------------------------------------
void animationManager::Tick()
{
  assert(this->Interactor);

  // Compute time since previous tick
  std::chrono::steady_clock::time_point tick = std::chrono::steady_clock::now();
  auto timeInMS =
    std::chrono::duration_cast<std::chrono::milliseconds>(tick - this->PreviousTick).count();
  this->PreviousTick = tick;

  // Convert to a usable time in seconds
  double elapsedTime = static_cast<double>(timeInMS) / 1000.0;
  double animationSpeedFactor = this->Options->getAsDouble("scene.animation.speed-factor");

  // elapsedTime can be negative
  elapsedTime *= animationSpeedFactor;
  this->CurrentTime += elapsedTime;

  // Modulo computation, compute CurrentTime in the time range.
  if (this->CurrentTime < this->TimeRange[0] || this->CurrentTime > this->TimeRange[1])
  {
    auto modulo = [](double val, double mod)
    {
      const double remainder = fmod(val, mod);
      return remainder < 0 ? remainder + mod : remainder;
    };
    this->CurrentTime = this->TimeRange[0] +
      modulo(this->CurrentTime - this->TimeRange[0], this->TimeRange[1] - this->TimeRange[0]);
  }

  if (this->LoadAtTime(this->CurrentTime))
  {
    this->Window->render();
  }
}

//----------------------------------------------------------------------------
bool animationManager::LoadAtTime(double timeValue)
{
  assert(this->Importer);
  if (!this->HasAnimation)
  {
    return false;
  }

  /* clamp target time to available range */
  if (timeValue < this->TimeRange[0])
  {
    log::warn("Animation time ", timeValue, " is outside of range [", this->TimeRange[0], ", ",
      this->TimeRange[1], "], using ", this->TimeRange[0], ".");
    timeValue = this->TimeRange[0];
  }
  else if (timeValue > this->TimeRange[1])
  {
    log::warn("Animation time ", timeValue, " is outside of range [", this->TimeRange[0], ", ",
      this->TimeRange[1], "], using ", this->TimeRange[1], ".");
    timeValue = this->TimeRange[1];
  }

  this->CurrentTime = timeValue;
  this->CurrentTimeSet = true;
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 3, 20240706)
  if (!this->Importer->UpdateAtTimeValue(this->CurrentTime))
  {
    log::error("Could not load time value: ", this->CurrentTime);
    return false;
  }
#else
  this->Importer->UpdateTimeStep(this->CurrentTime);
#endif

  if (this->Interactor && this->ProgressWidget)
  {
    // Set progress bar
    vtkProgressBarRepresentation* progressRep =
      vtkProgressBarRepresentation::SafeDownCast(this->ProgressWidget->GetRepresentation());
    progressRep->SetProgressRate(
      (this->CurrentTime - this->TimeRange[0]) / (this->TimeRange[1] - this->TimeRange[0]));

    this->Interactor->UpdateRendererAfterInteraction();
  }
  return true;
}

// ---------------------------------------------------------------------------------
void animationManager::CycleAnimation()
{
  assert(this->Importer);
  if (this->AvailAnimations <= 0)
  {
    return;
  }
  this->AnimationIndex += 1;

  if (this->AnimationIndex == this->AvailAnimations)
  {
    this->AnimationIndex = -1;
  }

  this->EnableOnlyCurrentAnimation();
  this->LoadAtTime(this->TimeRange[0]);
}

// ---------------------------------------------------------------------------------
int animationManager::GetAnimationIndex()
{
  return this->AnimationIndex;
}

// ---------------------------------------------------------------------------------
std::string animationManager::GetAnimationName()
{
  if (!this->Importer || this->AvailAnimations <= 0)
  {
    return "No animation";
  }

  if (this->AnimationIndex == -1)
  {
    return "All Animations";
  }
  return this->Importer->GetAnimationName(this->AnimationIndex);
}

//----------------------------------------------------------------------------
void animationManager::EnableOnlyCurrentAnimation()
{
  assert(this->Importer);
  for (int i = 0; i < this->AvailAnimations; i++)
  {
    this->Importer->DisableAnimation(i);
  }
  for (int i = 0; i < this->AvailAnimations; i++)
  {
    if (this->AnimationIndex == -1 || i == this->AnimationIndex)
    {
      this->Importer->EnableAnimation(i);
    }
  }
}

//----------------------------------------------------------------------------
void animationManager::GetTimeRange(double timeRange[2])
{
  timeRange[0] = this->TimeRange[0];
  timeRange[1] = this->TimeRange[1];
}
}
