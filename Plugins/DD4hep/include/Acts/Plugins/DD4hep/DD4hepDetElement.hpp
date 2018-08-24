// This file is part of the Acts project.
//
// Copyright (C) 2017-2018 Acts project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

///////////////////////////////////////////////////////////////////
// DD4hepDetElement.h, Acts project, DD4hepDetector plugin
///////////////////////////////////////////////////////////////////

#pragma once
#include "Acts/Plugins/TGeo/TGeoDetectorElement.hpp"
#include "DD4hep/DetElement.h"

namespace Acts {

/// @class DD4hepDetElement
///
/// @brief DetectorElement class implementation for DD4hep geometry
///
/// DetectorElement plugin for DD4hep detector elements. DD4hep is based on TGeo
/// shapes, therefore the DD4hepDetElement inherits from TGeoDetectorElement.
/// The
/// full geometrical information is provided by the TGeoDetectorElement. The
/// DD4hepDetElement extends the TGeoDetectorElement by containing a
/// segmentation
/// for the readout.

/// @todo what if shape conversion failes? add implementation of more than one
/// surface per module, implementing also for other shapes->Cone,ConeSeg,Tube?
/// what
/// if not used with DD4hep?
/// @todo segmentation

class DD4hepDetElement : public TGeoDetectorElement
{
public:
  /// Constructor
  /// @param detElement The DD4hep DetElement which should be linked to a
  /// surface
  /// @param axes is the axis orientation with respect to the tracking frame
  ///        it is a string of the three characters x, y and z (standing for the
  ///        three axes)
  ///        There is a distinction between
  /// capital and lower case
  /// characters :
  /// 	- capital      -> positive orientation of the axis
  ///		- lower case   -> negative oriantation of the axis
  ///
  ///
  /// Example options are:
  /// 	- "XYZ" -> identical frame definition (default value)
  /// 	- "YZX" -> node y axis is tracking x axis, etc.
  ///		- "XzY" -> negative node z axis is tracking y axis, etc.
  /// @param scalor is the scale factor for unit conversion if needed
  /// @param isDisc in case the sensitive detector module should be translated
  ///        as disc (e.g. for endcaps) this flag should be set to true
  /// @note In the translation from a 3D geometry (TGeo) which only knows tubes
  ///       to a 2D geometry (Tracking geometry) a distinction if the module
  ///       should be described as a cylinder or a disc surface needs to be
  ///       done. Since this information can not be taken just from the geometry
  ///       description (both can be described as TGeoTubeSeg), one needs to
  ///       set the flag 'isDisc' in case a volume with shape \c TGeoTubeSeg
  ///       should be translated to a disc surface. Per default it will be
  ///       translated into a cylindrical surface.
  /// @param material Possible material of detector element
  DD4hepDetElement(const dd4hep::DetElement                     detElement,
                   const std::string&                           axes   = "XYZ",
                   double                                       scalor = 1.,
                   bool                                         isDisc = false,
                   std::shared_ptr<const Acts::SurfaceMaterial> material
                   = nullptr);
  /// Desctructor
  virtual ~DD4hepDetElement() = default;

private:
  /// DD4hep detector element
  dd4hep::DetElement m_detElement;
  /// DD4hep segmentation
  dd4hep::Segmentation m_segmentation;
};
}