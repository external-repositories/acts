// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/EventData/NeutralTrackParameters.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/detail/coordinate_transformations.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

namespace tt = boost::test_tools;

namespace Acts {
namespace Test {

using Covariance = BoundSymMatrix;
using Jacobian = BoundMatrix;

/// @brief Simplified propagator state
struct PropState {
  /// @brief Constructor
  PropState(StraightLineStepper::State sState) : stepping(sState) {}
  /// State of the straight line stepper
  StraightLineStepper::State stepping;
  /// Propagator options which only carry the particle's mass
  struct {
    double mass = 42.;
  } options;
};

/// These tests are aiming to test whether the state setup is working properly
BOOST_AUTO_TEST_CASE(straight_line_stepper_state_test) {
  // Set up some variables
  GeometryContext tgContext = GeometryContext();
  MagneticFieldContext mfContext = MagneticFieldContext();
  NavigationDirection ndir = backward;
  double stepSize = 123.;
  double tolerance = 234.;

  Vector3D pos(1., 2., 3.);
  Vector3D mom(4., 5., 6.);
  double time = 7.;
  double charge = -1.;

  // Test charged parameters without covariance matrix
  CurvilinearParameters cp(std::nullopt, pos, mom, charge, time);
  StraightLineStepper::State slsState(tgContext, mfContext, cp, ndir, stepSize,
                                      tolerance);

  // Test the result & compare with the input/test for reasonable members
  BOOST_TEST(slsState.jacToGlobal == BoundToFreeMatrix::Zero());
  BOOST_TEST(slsState.jacTransport == FreeMatrix::Identity());
  BOOST_TEST(slsState.derivative == FreeVector::Zero());
  BOOST_TEST(!slsState.covTransport);
  BOOST_TEST(slsState.cov == Covariance::Zero());
  BOOST_TEST(slsState.pos == pos);
  BOOST_TEST(slsState.dir == mom.normalized());
  BOOST_TEST(slsState.p == mom.norm());
  BOOST_TEST(slsState.q == charge);
  BOOST_TEST(slsState.t == time);
  BOOST_TEST(slsState.navDir == ndir);
  BOOST_TEST(slsState.pathAccumulated == 0.);
  BOOST_TEST(slsState.stepSize == ndir * stepSize);
  BOOST_TEST(slsState.previousStepSize == 0.);
  BOOST_TEST(slsState.tolerance == tolerance);

  // Test without charge and covariance matrix
  NeutralCurvilinearTrackParameters ncp(std::nullopt, pos, mom, time);
  slsState = StraightLineStepper::State(tgContext, mfContext, ncp, ndir,
                                        stepSize, tolerance);
  BOOST_TEST(slsState.q == 0.);

  // Test with covariance matrix
  Covariance cov = 8. * Covariance::Identity();
  ncp = NeutralCurvilinearTrackParameters(cov, pos, mom, time);
  slsState = StraightLineStepper::State(tgContext, mfContext, ncp, ndir,
                                        stepSize, tolerance);
  BOOST_TEST(slsState.jacToGlobal != BoundToFreeMatrix::Zero());
  BOOST_TEST(slsState.covTransport);
  BOOST_TEST(slsState.cov == cov);
}

/// These tests are aiming to test the functions of the StraightLineStepper
/// The numerical correctness of the stepper is tested in the integration tests
BOOST_AUTO_TEST_CASE(straight_line_stepper_test) {
  // Set up some variables for the state
  GeometryContext tgContext = GeometryContext();
  MagneticFieldContext mfContext = MagneticFieldContext();
  NavigationDirection ndir = backward;
  double stepSize = 123.;
  double tolerance = 234.;

  // Construct the parameters
  Vector3D pos(1., 2., 3.);
  Vector3D mom(4., 5., 6.);
  double time = 7.;
  double charge = -1.;
  Covariance cov = 8. * Covariance::Identity();
  CurvilinearParameters cp(cov, pos, mom, charge, time);

  // Build the state and the stepper
  StraightLineStepper::State slsState(tgContext, mfContext, cp, ndir, stepSize,
                                      tolerance);
  StraightLineStepper sls;

  // Test the getters
  BOOST_TEST(sls.position(slsState) == slsState.pos);
  BOOST_TEST(sls.direction(slsState) == slsState.dir);
  BOOST_TEST(sls.momentum(slsState) == slsState.p);
  BOOST_TEST(sls.charge(slsState) == slsState.q);
  BOOST_TEST(sls.time(slsState) == slsState.t);

  //~ BOOST_TEST(sls.overstepLimit(slsState) == tolerance);

  // Step size modifies
  const std::string originalStepSize = slsState.stepSize.toString();

  sls.setStepSize(slsState, 1337.);
  BOOST_TEST(slsState.previousStepSize == ndir * stepSize);
  BOOST_TEST(slsState.stepSize == 1337.);

  sls.releaseStepSize(slsState);
  BOOST_TEST(slsState.stepSize == -123.);
  BOOST_TEST(sls.outputStepSize(slsState) == originalStepSize);

  // Test the curvilinear state construction
  auto curvState = sls.curvilinearState(slsState);
  auto curvPars = std::get<0>(curvState);
  CHECK_CLOSE_ABS(curvPars.position(), cp.position(), 1e-6);
  CHECK_CLOSE_ABS(curvPars.momentum(), cp.momentum(), 1e-6);
  CHECK_CLOSE_ABS(curvPars.charge(), cp.charge(), 1e-6);
  CHECK_CLOSE_ABS(curvPars.time(), cp.time(), 1e-6);
  BOOST_TEST(curvPars.covariance().has_value());
  BOOST_TEST(*curvPars.covariance() != cov);
  CHECK_CLOSE_COVARIANCE(std::get<1>(curvState),
                         BoundMatrix(BoundMatrix::Identity()), 1e-6);
  CHECK_CLOSE_ABS(std::get<2>(curvState), 0., 1e-6);

  // Test the update method
  Vector3D newPos(2., 4., 8.);
  Vector3D newMom(3., 9., 27.);
  double newTime(321.);
  sls.update(slsState, newPos, newMom.normalized(), newMom.norm(), newTime);
  BOOST_TEST(slsState.pos == newPos);
  BOOST_TEST(slsState.dir == newMom.normalized());
  BOOST_TEST(slsState.p == newMom.norm());
  BOOST_TEST(slsState.q == charge);
  BOOST_TEST(slsState.t == newTime);

  // The covariance transport
  slsState.cov = cov;
  sls.covarianceTransport(slsState);
  BOOST_TEST(slsState.cov != cov);
  BOOST_TEST(slsState.jacToGlobal != BoundToFreeMatrix::Zero());
  BOOST_TEST(slsState.jacTransport == FreeMatrix::Identity());
  BOOST_TEST(slsState.derivative == FreeVector::Zero());

  // Perform a step without and with covariance transport
  slsState.cov = cov;
  PropState ps(slsState);

  ps.stepping.covTransport = false;
  double h = sls.step(ps).value();
  BOOST_TEST(ps.stepping.stepSize == ndir * stepSize);
  BOOST_TEST(ps.stepping.stepSize == h);
  CHECK_CLOSE_COVARIANCE(ps.stepping.cov, cov, 1e-6);
  BOOST_TEST(ps.stepping.pos.norm() > newPos.norm());
  BOOST_TEST(ps.stepping.dir == newMom.normalized());
  BOOST_TEST(ps.stepping.p == newMom.norm());
  BOOST_TEST(ps.stepping.q == charge);
  BOOST_TEST(ps.stepping.t < newTime);
  BOOST_TEST(ps.stepping.derivative == FreeVector::Zero());
  BOOST_TEST(ps.stepping.jacTransport == FreeMatrix::Identity());

  ps.stepping.covTransport = true;
  double h2 = sls.step(ps).value();
  BOOST_TEST(ps.stepping.stepSize == ndir * stepSize);
  BOOST_TEST(h2 == h);
  CHECK_CLOSE_COVARIANCE(ps.stepping.cov, cov, 1e-6);
  BOOST_TEST(ps.stepping.pos.norm() > newPos.norm());
  BOOST_TEST(ps.stepping.dir == newMom.normalized());
  BOOST_TEST(ps.stepping.p == newMom.norm());
  BOOST_TEST(ps.stepping.q == charge);
  BOOST_TEST(ps.stepping.t < newTime);
  BOOST_TEST(ps.stepping.derivative != FreeVector::Zero());
  BOOST_TEST(ps.stepping.jacTransport != FreeMatrix::Identity());

  /// Test the state reset
  // Construct the parameters
  Vector3D pos2(1.5, -2.5, 3.5);
  Vector3D mom2(4.5, -5.5, 6.5);
  double time2 = 7.5;
  double charge2 = 1.;
  BoundSymMatrix cov2 = 8.5 * Covariance::Identity();
  CurvilinearParameters cp2(cov2, pos2, mom2, charge2, time2);
  FreeVector freeParams =
      detail::coordinate_transformation::boundParameters2freeParameters(
          tgContext, cp2.parameters(), cp2.referenceSurface());
  ndir = forward;
  double stepSize2 = -2. * stepSize;

  // Reset all possible parameters
  StraightLineStepper::State slsStateCopy(ps.stepping);
  sls.resetState(slsStateCopy, cp2.parameters(), *cp2.covariance(),
                 cp2.referenceSurface(), ndir, stepSize2);
  // Test all components
  BOOST_TEST(slsStateCopy.jacToGlobal != BoundToFreeMatrix::Zero());
  BOOST_TEST(slsStateCopy.jacToGlobal != ps.stepping.jacToGlobal);
  BOOST_TEST(slsStateCopy.jacTransport == FreeMatrix::Identity());
  BOOST_TEST(slsStateCopy.derivative == FreeVector::Zero());
  BOOST_TEST(slsStateCopy.covTransport);
  BOOST_TEST(slsStateCopy.cov == cov2);
  BOOST_TEST(slsStateCopy.pos == freeParams.template segment<3>(eFreePos0));
  BOOST_TEST(slsStateCopy.dir ==
             freeParams.template segment<3>(eFreeDir0).normalized());
  BOOST_TEST(slsStateCopy.p == std::abs(1. / freeParams[eFreeQOverP]));
  BOOST_TEST(slsStateCopy.q == ps.stepping.q);
  BOOST_TEST(slsStateCopy.t == freeParams[eFreeTime]);
  BOOST_TEST(slsStateCopy.navDir == ndir);
  BOOST_TEST(slsStateCopy.pathAccumulated == 0.);
  BOOST_TEST(slsStateCopy.stepSize == ndir * stepSize2);
  BOOST_TEST(slsStateCopy.previousStepSize == ps.stepping.previousStepSize);
  BOOST_TEST(slsStateCopy.tolerance == ps.stepping.tolerance);

  // Reset all possible parameters except the step size
  slsStateCopy = ps.stepping;
  sls.resetState(slsStateCopy, cp2.parameters(), *cp2.covariance(),
                 cp2.referenceSurface(), ndir);
  // Test all components
  BOOST_TEST(slsStateCopy.jacToGlobal != BoundToFreeMatrix::Zero());
  BOOST_TEST(slsStateCopy.jacToGlobal != ps.stepping.jacToGlobal);
  BOOST_TEST(slsStateCopy.jacTransport == FreeMatrix::Identity());
  BOOST_TEST(slsStateCopy.derivative == FreeVector::Zero());
  BOOST_TEST(slsStateCopy.covTransport);
  BOOST_TEST(slsStateCopy.cov == cov2);
  BOOST_TEST(slsStateCopy.pos == freeParams.template segment<3>(eFreePos0));
  BOOST_TEST(slsStateCopy.dir ==
             freeParams.template segment<3>(eFreeDir0).normalized());
  BOOST_TEST(slsStateCopy.p == std::abs(1. / freeParams[eFreeQOverP]));
  BOOST_TEST(slsStateCopy.q == ps.stepping.q);
  BOOST_TEST(slsStateCopy.t == freeParams[eFreeTime]);
  BOOST_TEST(slsStateCopy.navDir == ndir);
  BOOST_TEST(slsStateCopy.pathAccumulated == 0.);
  BOOST_TEST(slsStateCopy.stepSize ==
             ndir * std::numeric_limits<double>::max());
  BOOST_TEST(slsStateCopy.previousStepSize == ps.stepping.previousStepSize);
  BOOST_TEST(slsStateCopy.tolerance == ps.stepping.tolerance);

  // Reset the least amount of parameters
  slsStateCopy = ps.stepping;
  sls.resetState(slsStateCopy, cp2.parameters(), *cp2.covariance(),
                 cp2.referenceSurface());
  // Test all components
  BOOST_TEST(slsStateCopy.jacToGlobal != BoundToFreeMatrix::Zero());
  BOOST_TEST(slsStateCopy.jacToGlobal != ps.stepping.jacToGlobal);
  BOOST_TEST(slsStateCopy.jacTransport == FreeMatrix::Identity());
  BOOST_TEST(slsStateCopy.derivative == FreeVector::Zero());
  BOOST_TEST(slsStateCopy.covTransport);
  BOOST_TEST(slsStateCopy.cov == cov2);
  BOOST_TEST(slsStateCopy.pos == freeParams.template segment<3>(eFreePos0));
  BOOST_TEST(slsStateCopy.dir ==
             freeParams.template segment<3>(eFreeDir0).normalized());
  BOOST_TEST(slsStateCopy.p == std::abs(1. / freeParams[eFreeQOverP]));
  BOOST_TEST(slsStateCopy.q == ps.stepping.q);
  BOOST_TEST(slsStateCopy.t == freeParams[eFreeTime]);
  BOOST_TEST(slsStateCopy.navDir == forward);
  BOOST_TEST(slsStateCopy.pathAccumulated == 0.);
  BOOST_TEST(slsStateCopy.stepSize == std::numeric_limits<double>::max());
  BOOST_TEST(slsStateCopy.previousStepSize == ps.stepping.previousStepSize);
  BOOST_TEST(slsStateCopy.tolerance == ps.stepping.tolerance);

  /// Repeat with surface related methods
  auto plane = Surface::makeShared<PlaneSurface>(pos, mom.normalized());
  BoundParameters bp(tgContext, cov, pos, mom, charge, time, plane);
  slsState = StraightLineStepper::State(tgContext, mfContext, cp, ndir,
                                        stepSize, tolerance);

  // Test the intersection in the context of a surface
  auto targetSurface = Surface::makeShared<PlaneSurface>(
      pos + ndir * 2. * mom.normalized(), mom.normalized());
  sls.updateSurfaceStatus(slsState, *targetSurface, BoundaryCheck(false));
  BOOST_TEST(slsState.stepSize.value(ConstrainedStep::actor), ndir * 2.);

  // Test the step size modification in the context of a surface
  sls.updateStepSize(
      slsState,
      targetSurface->intersect(slsState.geoContext, slsState.pos,
                               slsState.navDir * slsState.dir, false),
      false);
  BOOST_TEST(slsState.stepSize == 2.);
  slsState.stepSize = ndir * stepSize;
  sls.updateStepSize(
      slsState,
      targetSurface->intersect(slsState.geoContext, slsState.pos,
                               slsState.navDir * slsState.dir, false),
      true);
  BOOST_TEST(slsState.stepSize == 2.);

  // Test the bound state construction
  auto boundState = sls.boundState(slsState, *plane);
  auto boundPars = std::get<0>(boundState);
  CHECK_CLOSE_ABS(boundPars.position(), bp.position(), 1e-6);
  CHECK_CLOSE_ABS(boundPars.momentum(), bp.momentum(), 1e-6);
  CHECK_CLOSE_ABS(boundPars.charge(), bp.charge(), 1e-6);
  CHECK_CLOSE_ABS(boundPars.time(), bp.time(), 1e-6);
  BOOST_TEST(boundPars.covariance().has_value());
  BOOST_TEST(*boundPars.covariance() != cov);
  CHECK_CLOSE_COVARIANCE(std::get<1>(boundState),
                         BoundMatrix(BoundMatrix::Identity()), 1e-6);
  CHECK_CLOSE_ABS(std::get<2>(boundState), 0., 1e-6);

  // Update in context of a surface
  BoundParameters bpTarget(tgContext, 2. * cov, 2. * pos, 2. * mom,
                           -1. * charge, 2. * time, targetSurface);
  Vector3D dir = bpTarget.momentum().normalized();
  freeParams[eFreePos0] = bpTarget.position()[eX];
  freeParams[eFreePos1] = bpTarget.position()[eY];
  freeParams[eFreePos2] = bpTarget.position()[eZ];
  freeParams[eFreeTime] = bpTarget.time();
  freeParams[eFreeDir0] = dir[eMom0];
  freeParams[eFreeDir1] = dir[eMom1];
  freeParams[eFreeDir2] = dir[eMom2];
  freeParams[eFreeQOverP] = bpTarget.charge() / bpTarget.momentum().norm();

  sls.update(slsState, freeParams, *bpTarget.covariance());
  BOOST_TEST(slsState.pos == 2. * pos);
  CHECK_CLOSE_ABS(slsState.dir, mom.normalized(), 1e-6);
  BOOST_TEST(slsState.p == 2. * mom.norm());
  BOOST_TEST(slsState.q == 1. * charge);
  BOOST_TEST(slsState.t == 2. * time);
  CHECK_CLOSE_COVARIANCE(slsState.cov, Covariance(2. * cov), 1e-6);

  // Transport the covariance in the context of a surface
  sls.covarianceTransport(slsState, *plane);
  BOOST_TEST(slsState.cov != cov);
  BOOST_TEST(slsState.jacToGlobal != BoundToFreeMatrix::Zero());
  BOOST_TEST(slsState.jacTransport == FreeMatrix::Identity());
  BOOST_TEST(slsState.derivative == FreeVector::Zero());
}
}  // namespace Test
}  // namespace Acts