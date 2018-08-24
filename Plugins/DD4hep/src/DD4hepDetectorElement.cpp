// This file is part of the Acts project.
//
// Copyright (C) 2017-2018 Acts project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/DD4hep/DD4hepDetElement.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Utilities/Units.hpp"
#include "DD4hep/CartesianGridXY.h"

Acts::DD4hepDetElement::DD4hepDetElement(
    const dd4hep::DetElement                     detElement,
    const std::string&                           axes,
    double                                       scalor,
    bool                                         isDisc,
    std::shared_ptr<const Acts::SurfaceMaterial> material)
  : Acts::TGeoDetectorElement(Identifier(detElement.volumeID()),
                              detElement.nominal().worldTransformation(),
                              detElement.placement().ptr(),
                              axes,
                              scalor,
                              isDisc,
                              material)
  , m_detElement(std::move(detElement))

{
}