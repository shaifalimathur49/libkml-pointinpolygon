// Copyright 2008, Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file contains the implementation of some utility functions used
// from within the Regionator which are also available for use in classes
// derived from RegionHandler.

#include "kml/regionator/regionator_util.h"
#include <string>
#include "kml/dom.h"
#include "kml/engine/clone.h"

namespace kmlregionator {

using std::string;
using kmldom::AbstractLatLonBoxPtr;
using kmldom::CoordinatesPtr;
using kmldom::DocumentPtr;
using kmldom::FolderPtr;
using kmldom::KmlFactory;
using kmldom::LatLonAltBoxPtr;
using kmldom::LineStringPtr;
using kmldom::LinkPtr;
using kmldom::LodPtr;
using kmldom::NetworkLinkPtr;
using kmldom::PlacemarkPtr;
using kmldom::PointPtr;
using kmldom::RegionPtr;

// This creates a new LatLonAltBox setting all fields set in the original.
LatLonAltBoxPtr CloneLatLonAltBox(const LatLonAltBoxPtr& orig) {
  return AsLatLonAltBox(kmlengine::Clone(orig));
}

// This creates a new Lod setting all fields set in the original.
LodPtr CloneLod(const LodPtr& orig) {
  return AsLod(kmlengine::Clone(orig));
}

// This creates a new Region based on the LatLonAltBox and/or Lod found in the
// original Region.
RegionPtr CloneRegion(const RegionPtr& orig) {
  return AsRegion(kmlengine::Clone(orig));
}

// This creates a new child Region of the given quadrant of the parent.
// The altitude fields of LatLonAltBox are cloned unchanged as are
// all fields in the Lod if there is one.
RegionPtr CreateChildRegion(const RegionPtr& parent, quadrant_t quadrant) {
  KmlFactory* factory = KmlFactory::GetFactory();

  RegionPtr child = factory->CreateRegion();
  if (parent->has_latlonaltbox()) {
    double mid_lat, mid_lon;
    GetCenter(parent->get_latlonaltbox(), &mid_lat, &mid_lon);
    LatLonAltBoxPtr latlonaltbox =
        CloneLatLonAltBox(parent->get_latlonaltbox());
    switch (quadrant) {
      case NW:
        latlonaltbox->set_south(mid_lat);
        latlonaltbox->set_east(mid_lon);
        break;
      case NE:
        latlonaltbox->set_south(mid_lat);
        latlonaltbox->set_west(mid_lon);
        break;
      case SW:
        latlonaltbox->set_north(mid_lat);
        latlonaltbox->set_east(mid_lon);
        break;
      case SE:
        latlonaltbox->set_north(mid_lat);
        latlonaltbox->set_west(mid_lon);
        break;
    }
    child->set_latlonaltbox(latlonaltbox);
  }
  if (parent->has_lod()) {  // Make an exact clone.
    child->set_lod(CloneLod(parent->get_lod()));
  }
  return child;
}

// This is a convenience function to create a LineString Placemark whose
// coordinates are taken from the LatLonAltBox in the Region.  If the Region
// has no LatLonAltBox no LineString is created, but a Placemark with the
// given name is created and returned.
PlacemarkPtr CreateLineStringBox(const string& name, const RegionPtr& region) {
  KmlFactory* factory = KmlFactory::GetFactory();
  PlacemarkPtr placemark = factory->CreatePlacemark();
  placemark->set_name(name);
  if (region->has_latlonaltbox()) {
    double north = region->get_latlonaltbox()->get_north();
    double south = region->get_latlonaltbox()->get_south();
    double east = region->get_latlonaltbox()->get_east();
    double west = region->get_latlonaltbox()->get_west();
    CoordinatesPtr coordinates = factory->CreateCoordinates();
    coordinates->add_latlng(north, west);
    coordinates->add_latlng(north, east);
    coordinates->add_latlng(south, east);
    coordinates->add_latlng(south, west);
    coordinates->add_latlng(south, west);
    LineStringPtr linestring = factory->CreateLineString();
    linestring->set_coordinates(coordinates);
    linestring->set_tessellate("1");
    placemark->set_geometry(linestring);
  }
  return placemark;
}

// This is a convenience function to create a Point Placemark.
PlacemarkPtr CreatePointPlacemark(const string& name, double lat, double lon) {
  KmlFactory* factory = KmlFactory::GetFactory();
  PlacemarkPtr placemark = factory->CreatePlacemark();
  placemark->set_name(name);
  CoordinatesPtr coordinates = factory->CreateCoordinates();
  coordinates->add_latlng(lat, lon);
  PointPtr point = factory->CreatePoint();
  point->set_coordinates(coordinates);
  placemark->set_geometry(point);
  return placemark;
}

// This creates a Region at the given bounding box with the given Lod range.
RegionPtr CreateRegion2d(double north, double south, double east, double west,
                         double minlodpixels, double maxlodpixels) {
  KmlFactory* factory = KmlFactory::GetFactory();
  RegionPtr region = factory->CreateRegion();
  LatLonAltBoxPtr latlonaltbox = factory->CreateLatLonAltBox();
  latlonaltbox->set_north(north);
  latlonaltbox->set_south(south);
  latlonaltbox->set_east(east);
  latlonaltbox->set_west(west);
  LodPtr lod = factory->CreateLod();
  lod->set_minlodpixels(minlodpixels);
  lod->set_maxlodpixels(maxlodpixels);
  region->set_latlonaltbox(latlonaltbox);
  region->set_lod(lod);
  return region;
}

// This is a convenience function to create a NetworkLink to the specified
// url with onRegion refresh semantics.
NetworkLinkPtr CreateRegionNetworkLink(const RegionPtr& region,
                                       const string& href) {
  KmlFactory* factory = KmlFactory::GetFactory();
  NetworkLinkPtr networklink = factory->CreateNetworkLink();
  networklink->set_region(CloneRegion(region));
  LinkPtr link = factory->CreateLink();
  link->set_href(href);
  link->set_viewrefreshmode(kmldom::VIEWREFRESHMODE_ONREGION);
  networklink->set_link(link);
  return networklink;
}

// This is a convenience function to create a new Document whose Region
// is as specified.
DocumentPtr CreateRegionDocument(const RegionPtr& region) {
  KmlFactory* factory = KmlFactory::GetFactory();
  DocumentPtr document = factory->CreateDocument();
  document->set_region(CloneRegion(region));
  return document;
}

// This returns the mid point of the given LatLonBox or LatLonAltBox.
void GetCenter(const AbstractLatLonBoxPtr& allb, double* lat, double* lon) {
  if (lat) {
    *lat = (allb->get_north() + allb->get_south())/2.0;
  }
  if (lon) {
    *lon = (allb->get_east() + allb->get_west())/2.0;
  }
}

}  // end namespace kmlregionator
