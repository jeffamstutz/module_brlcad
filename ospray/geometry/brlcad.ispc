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
                       NULL,0,NULL);
  return self;
}

export void BRLCAD_destroy(void *uniform _self)
{
  BRLCAD *uniform self = (BRLCAD *uniform)_self;
  delete self;
}

unmasked void BRLCAD_bounds(BRLCAD *uniform self,
                            uniform size_t primID,
                            uniform box3fa &bbox)
{
#if 0
  print("bounds\n");
  Patch *uniform patch = self->patchArray+primID;
  bbox = box_extend(box_extend(box_extend(box_extend(make_box3fa_empty(),make_vec3fa(patch->v00)),
                                          make_vec3fa(patch->v01)),
                               make_vec3fa(patch->v10)),
                    make_vec3fa(patch->v11));
#endif
}

void BRLCAD_intersect(BRLCAD *uniform self,
                               varying Ray &ray,
                               uniform size_t primID)
{
#if 0
  Patch *uniform patch = self->patchArray+primID;

  // create a simple coordinate system with x and y directions
  // orthogonal to the ray
  const LinearSpace3f coordSys = frame(ray.dir);

  if (recursiveBisection(ray,coordSys,
                         patch->v00,patch->v01,patch->v10,patch->v11,
                         0.f,1.f, 0.f,1.f,
                         /* number of bisections */ 10)) {
    ray.geomID = self->super.geomID;
    ray.primID = primID;
  }
#endif
}

export void BRLCAD_finalize(void *uniform _self,
                            void *uniform _model,
                            float *uniform patchesDataMemory,
                            uniform size_t numPatches)
{
  // first, typecast to our 'real' type. since ispc can't export real
  // types to c we have to pass 'self' in as a void*, and typecast
  // here.
  BRLCAD *uniform self = (BRLCAD *uniform)_self;

  Model *uniform model  = (Model *uniform)_model;

  // create a new embree geometry with numpathces prims, in the model
  // that this goemetry is in.
  uint32 uniform geomID = 
    rtcNewUserGeometry(model->embreeSceneHandle,numPatches);
  
  // set 'us' as user data (this will be the first arg in intersect()
  // and computebounds() callbacks
  rtcSetUserData(model->embreeSceneHandle,geomID,self);
  
  // set function pointers:
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&BRLCAD_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&BRLCAD_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                         (uniform RTCOccludedFuncVarying)&BRLCAD_intersect);
}
                                    