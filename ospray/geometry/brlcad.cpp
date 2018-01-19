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

#include "brlcad.h"
#include "brlcad_ispc.h"

#include "ospray/common/Data.h"
#include "ospray/common/Model.h"
#include "ospray/common/Ray.h"

#include "ospcommon/tasking/tasking_system_handle.h"
#include "ospcommon/utility/StringManip.h"

#include <mutex>
#include <thread>
#include <unordered_map>

namespace ospray {
  namespace brlcad {

    static inline int getCpuId()
    {
      static std::unordered_map<std::thread::id, int> threadIds;
      static int nextId{1};
      static std::mutex idMutex;

      const auto currentThread = std::this_thread::get_id();

      auto id = threadIds[currentThread];

      if (id == 0) {
        SCOPED_LOCK(idMutex);
        threadIds[currentThread] = id = nextId++;
      }

      return id - 1;
    }

    // Local helper functions /////////////////////////////////////////////////

    template<typename T>
    inline static void extractRay(const T& rays, RTCRay &ray, int i)
    {
      ray.org[0] = rays.orgx[i];
      ray.org[1] = rays.orgy[i];
      ray.org[2] = rays.orgz[i];

      ray.dir[0] = rays.dirx[i];
      ray.dir[1] = rays.diry[i];
      ray.dir[2] = rays.dirz[i];

      ray.tnear  = rays.tnear[i];
      ray.tfar   = rays.tfar[i];

      ray.time   = rays.time[i];
      ray.mask   = rays.mask[i];

      ray.primID = rays.primID[i];
      ray.geomID = rays.geomID[i];
      ray.instID = rays.instID[i];
    }

    template<typename T>
    inline static void insertRay(const RTCRay& ray, T &rays, int i)
    {
      if (ray.geomID != RTC_INVALID_GEOMETRY_ID) {
        rays.Ngx[i] = ray.Ng[0];
        rays.Ngy[i] = ray.Ng[1];
        rays.Ngz[i] = ray.Ng[2];

        rays.primID[i] = ray.primID;
        rays.geomID[i] = ray.geomID;
        rays.instID[i] = ray.instID;
        rays.tfar[i]   = ray.tfar;
        rays.u[i]      = ray.u;
        rays.v[i]      = ray.v;
      }
    }

    static int hitCallback(application *ap,
                           partition *PartHeadp,
                           seg *segs)
    {
      /* will contain surface curvature information at the entry */
      curvature cur = RT_CURVATURE_INIT_ZERO;

      auto &ray = *static_cast<RTCRay*>(ap->a_uptr);

      /* iterate over each partition until we get back to the head.
       * each partition corresponds to a specific homogeneous region of
       * material.
       */
      for (auto *pp = PartHeadp->pt_forw; pp != PartHeadp; pp = pp->pt_forw) {
        /* entry hit point, so we type less */
        auto *hitp = pp->pt_inhit;

#if 0
        /* construct the actual (entry) hit-point from the ray and the
         * distance to the intersection point (i.e., the 't' value).
         */
        point_t pt;
        VJOIN1(pt, ap->a_ray.r_pt, hitp->hit_dist, ap->a_ray.r_dir);
#endif

        /* primitive we encountered on entry */
        auto *stp = pp->pt_inseg->seg_stp;

        /* compute the normal vector at the entry point, flipping the
         * normal if necessary.
         */
        vect_t inormal;
        RT_HIT_NORMAL(inormal, hitp, stp, &(ap->a_ray), pp->pt_inflip);

        ray.tfar = hitp->hit_dist;
        ray.Ng[0] = inormal[0];
        ray.Ng[1] = inormal[1];
        ray.Ng[2] = inormal[2];
#if 0
        /* This next macro fills in the curvature information which
         * consists on a principle direction vector, and the inverse
         * radii of curvature along that direction and perpendicular
         * to it.  Positive curvature bends toward the outward
         * pointing normal.
         */
        RT_CURVATURE(&cur, hitp, pp->pt_inflip, stp);

        /* exit point, so we type less */
        hitp = pp->pt_outhit;

        /* construct the actual (exit) hit-point from the ray and the
         * distance to the intersection point (i.e., the 't' value).
         */
        VJOIN1(pt, ap->a_ray.r_pt, hitp->hit_dist, ap->a_ray.r_dir);

        /* primitive we exited from */
        stp = pp->pt_outseg->seg_stp;

        /* compute the normal vector at the exit point, flipping the
         * normal if necessary.
         */
        vect_t onormal;
        RT_HIT_NORMAL(onormal, hitp, stp, &(ap->a_ray), pp->pt_outflip);
  #endif
      }

      // Return '1' for hit
      return 1;
    }

    static int missCallback(application *ap)
    {
      // Return '0' for miss
      return 0;
    }

    static void traceRay(const BRLCAD &geom, RTCRay& ray)
    {
      application ap;

      RT_APPLICATION_INIT(&ap);

      ap.a_rt_i = geom.rtip;
      ap.a_onehit = 1;

      ap.a_resource = &geom.resources[getCpuId()];

      VSET(ap.a_ray.r_pt, ray.org[0], ray.org[1], ray.org[2]);
      VSET(ap.a_ray.r_dir, ray.dir[0], ray.dir[1], ray.dir[2]);
      ap.a_ray.r_min = ray.tnear;
      ap.a_ray.r_max = ray.tfar;

      ap.a_hit  = hitCallback;
      ap.a_miss = missCallback;
      ap.a_uptr = &ray;

      auto hit = rt_shootray(&ap);
      if (hit) {
        ray.geomID = geom.geomID;
        ray.primID = 0;
      }
    }

    static void brlcadIntersect(const BRLCAD* geom_i, RTCRay& ray, size_t item)
    {
      const BRLCAD& geom = geom_i[item];
      traceRay(geom, ray);
    }

    static void brlcadIntersectN(const int*                 valid,
                                 const BRLCAD*              geom_i,
                                 const RTCIntersectContext* context,
                                 RTCRayNp*                  rays,
                                 size_t                     N,
                                 size_t                     item)
    {
      const BRLCAD& geom = geom_i[item];

      for (int i = 0; i < N; ++i) {
        if (valid[i]) {
          RTCRay ray;
          extractRay(*rays, ray, i);
          traceRay(geom, ray);
          insertRay(ray, *rays, i);
        }
      }
    }

    template<int SIZE>
    static void brlcadIntersectNt(const int*       mask,
                                  const BRLCAD*    geom_i,
                                  RTCRayNt<SIZE>&  _rays,
                                  size_t           item)
    {
      RTCIntersectContext ctx;
      RTCRayNp rays {_rays.orgx, _rays.orgy, _rays.orgz, _rays.dirx, _rays.diry,
                     _rays.dirz, _rays.tnear, _rays.tfar, _rays.time,
                     _rays.mask, _rays.Ngx, _rays.Ngy, _rays.Ngz, _rays.u,
                     _rays.v, _rays.geomID, _rays.primID, _rays.instID};
      brlcadIntersectN(mask, geom_i, &ctx, &rays, SIZE, item);
    }

    static void brlcadBounds(void *geom_i, size_t item, RTCBounds &bounds_o)
    {
      const auto& geom = ((const BRLCAD*)geom_i)[item];
      bounds_o.lower_x = geom.bounds.lower.x;
      bounds_o.lower_y = geom.bounds.lower.y;
      bounds_o.lower_z = geom.bounds.lower.z;
      bounds_o.upper_x = geom.bounds.upper.x;
      bounds_o.upper_y = geom.bounds.upper.y;
      bounds_o.upper_z = geom.bounds.upper.z;
    }

    // BRLCAD Geometry definitions ////////////////////////////////////////////

    BRLCAD::BRLCAD()
    {
      this->ispcEquivalent = ispc::BRLCAD_create(this);
    }

    BRLCAD::~BRLCAD()
    {
      ispc::BRLCAD_destroy(ispcEquivalent);
    }

    void BRLCAD::commit()
    {
      if (rtip)
        rt_free_rti(rtip);

      std::string filename = getParamString("filename");
      std::string objects  = getParamString("objects");

      rtip = rt_dirbuild(filename.c_str(), nullptr, 0);

      auto objNames = ospcommon::utility::split(objects, ',');
      for (const auto &obj : objNames)
        rt_gettree(rtip, obj.c_str());

      rt_prep_parallel(rtip, tasking::numTaskingThreads());

      if (rtip == nullptr)
        throw std::runtime_error("BRLCAD geometry requires an existing rt_i!");

      const int nThreads = tasking::numTaskingThreads() * 2;
      resources.resize(nThreads);
      for (int i = 0; i <  nThreads; ++i)
        rt_init_resource(&resources[i], i, rtip);

      bounds.lower.x = rtip->mdl_min[0];
      bounds.lower.y = rtip->mdl_min[1];
      bounds.lower.z = rtip->mdl_min[2];
      bounds.upper.x = rtip->mdl_max[0];
      bounds.upper.y = rtip->mdl_max[1];
      bounds.upper.z = rtip->mdl_max[2];
    }

    void BRLCAD::finalize(Model *model)
    {
      auto scene = model->embreeSceneHandle;

      geomID = rtcNewUserGeometry(scene, 1);

      rtcSetUserData(scene, geomID, this);
      rtcSetBoundsFunction(scene, geomID, brlcadBounds);

      rtcSetIntersectFunction(scene, geomID,
                              (RTCIntersectFunc)&brlcadIntersect);

      rtcSetIntersectFunction4(scene, geomID,
                               (RTCIntersectFunc4)&brlcadIntersectNt<4>);

      rtcSetIntersectFunction8(scene, geomID,
                               (RTCIntersectFunc8)&brlcadIntersectNt<8>);

      rtcSetIntersectFunction16(scene, geomID,
                                (RTCIntersectFunc16)&brlcadIntersectNt<16>);

      rtcSetOccludedFunction(scene, geomID,
                             (RTCOccludedFunc)&brlcadIntersect);

      rtcSetOccludedFunction4(scene, geomID,
                              (RTCOccludedFunc4)&brlcadIntersectNt<4>);

      rtcSetOccludedFunction8(scene, geomID,
                              (RTCOccludedFunc8)&brlcadIntersectNt<8>);

      rtcSetOccludedFunction16(scene, geomID,
                               (RTCOccludedFunc16)&brlcadIntersectNt<16>);
    }

    OSP_REGISTER_GEOMETRY(BRLCAD, brlcad);

  } // ::ospray::brlcad
} // ::ospray
