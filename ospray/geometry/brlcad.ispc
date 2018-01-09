// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// ospray
#include "common/Ray.ih"
#include "common/Model.ih"
#include "ospray/geometry/Geometry.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct BRLCAD
{
  Geometry super;
};

static void BRLCAD_postIntersect(uniform Geometry *uniform geometry,
                                 uniform Model *uniform model,
                                 varying DifferentialGeometry &dg,
                                 const varying Ray &ray,
                                 uniform int64 flags)
{
  BRLCAD *uniform self = (BRLCAD *uniform)geometry;
  dg.Ng = dg.Ns = normalize(ray.Ng);

  if (flags & DG_MATERIALID) {
    dg.materialID = 0;
  }
}

/*! create a new ispc equivalent for the C++ class (cppEquivalent
    points back, if we ever need this. this returns a void * that the
    c++ side can use to call back to us when needed. Note this return
    values _has_ to be a untyped 'void*' because ISPC cannot currently
    export 'real' class types - with function pointers etcpp - to a c
    header file */
export void *uniform BRLCAD_create(void *uniform cppEquivalent)
{
  BRLCAD *uniform self = uniform new uniform BRLCAD;
  
  /* recursively call the 'inherited' constructor, and pass our
     'overridden' functions for intersect and postintersect. note we
     do not have to set the 'intersect' and 'occluded' functions
     because we use embree to do the traversal for us (see
     BRLCAD_set()) below */
  Geometry_Constructor(&self->super,cppEquivalent,
                       BRLCAD_postIntersect,
                       NULL, 0, NULL);
  return self;
}

export void BRLCAD_destroy(void *uniform _self)
{
  BRLCAD *uniform self = (BRLCAD *uniform)_self;
  delete self;
}
