// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "common/sg/SceneGraph.h"
#include "common/sg/Renderer.h"
#include "common/sg/common/Data.h"
#include "common/sg/geometry/Geometry.h"

#include "exampleViewer/widgets/imguiViewer.h"

#include "ospcommon/tasking/tasking_system_handle.h"
#include "ospcommon/utility/StringManip.h"

#undef UNUSED
#undef _USE_MATH_DEFINES
#include "brlcad/common.h"
#include "brlcad/vmath.h"		/* vector math macros */
#include "brlcad/raytrace.h"	/* librt interface definitions */

namespace ospray {
  namespace brlcad {

    std::string filename;
    std::string objects;
    std::string rendererType = "raycast";

    struct BrlcadSGNode : public sg::Geometry
    {
      BrlcadSGNode() : Geometry("brlcad") {}

      box3f bounds() const override
      {
        return brlcadBounds;
      }

      void setFromXML(const xml::Node &node,
                      const unsigned char *binBasePtr) override {}

      box3f brlcadBounds;
    };

    using namespace ospcommon;

    rt_i *loadBrlcadGeometry(std::string filename, std::string objects)
    {
      if (filename.empty()) {
        throw std::runtime_error("No input filename provided!"
                                 " Use '-g [filename]' to specify the file.");
      }

      auto *rtip = rt_dirbuild(filename.c_str(), nullptr, 0);
      auto objNames = utility::split(objects, ',');

      if (objNames.empty()) {
        throw std::runtime_error("No objects provided! Use '-o [objects]' with"
                                 " a comma separated list of object names.");
      }

      for (const auto &obj : objNames)
        rt_gettree(rtip, obj.c_str());

      rt_prep_parallel(rtip, tasking::numTaskingThreads());
      return rtip;
    }

    static inline void parseCommandLine(int ac, const char **&av)
    {
      for (int i = 1; i < ac; i++) {
        const std::string arg = av[i];
        if (arg == "-g" || arg == "--geometry") {
          filename = av[++i];
        } else if (arg == "-o" || arg == "--objects") {
          objects = av[++i];
        } else if (arg == "-r" || arg == "--renderer") {
          rendererType = av[++i];
        }
      }
    }

    extern "C" int main(int ac, const char **av)
    {
      int init_error = ospInit(&ac, av);
      if (init_error != OSP_NO_ERROR) {
        std::cerr << "FATAL ERROR DURING INITIALIZATION!" << std::endl;
        return init_error;
      }

      auto device = ospGetCurrentDevice();
      if (device == nullptr) {
        std::cerr << "FATAL ERROR DURING GETTING CURRENT DEVICE!" << std::endl;
        return 1;
      }

      ospDeviceSetStatusFunc(device, [](const char *msg) { std::cout << msg; });
      ospDeviceSetErrorFunc(device,
                            [](OSPError e, const char *msg) {
                              std::cout << "OSPRAY ERROR [" << e << "]: "
                                        << msg << std::endl;
                              std::exit(1);
                            });

      ospDeviceCommit(device);

      // access/load symbols/sg::Nodes dynamically
      loadLibrary("ospray_sg");
      ospLoadModule("brlcad");

      box3f worldBounds;

      auto renderer_ptr = sg::createNode("renderer", "Renderer");
      auto &renderer = *renderer_ptr;

      renderer["frameBuffer"]["size"] = vec2i(1024, 768);

      auto &world = renderer["world"];

      // Load BRLCAD geometry and create BRLCAD geometry scenegraph node

      auto &brlcadInstance = world.createChild("brlcad_instance", "Instance");

      auto brlcadGeometryNode = std::make_shared<BrlcadSGNode>();
      brlcadGeometryNode->setName("loaded_example_brlcad");
      brlcadGeometryNode->setType("BrlcadSGNode");

      parseCommandLine(ac, av);
      auto *brlcadGeom = loadBrlcadGeometry(filename, objects);

      brlcadGeometryNode->brlcadBounds.lower.x = brlcadGeom->mdl_min[0];
      brlcadGeometryNode->brlcadBounds.lower.y = brlcadGeom->mdl_min[1];
      brlcadGeometryNode->brlcadBounds.lower.z = brlcadGeom->mdl_min[2];
      brlcadGeometryNode->brlcadBounds.upper.x = brlcadGeom->mdl_max[0];
      brlcadGeometryNode->brlcadBounds.upper.y = brlcadGeom->mdl_max[1];
      brlcadGeometryNode->brlcadBounds.upper.z = brlcadGeom->mdl_max[2];

      rt_free_rti(brlcadGeom);

      brlcadGeometryNode->createChild("filename", "string", filename);
      brlcadGeometryNode->createChild("objects", "string", objects);

      brlcadInstance["model"].add(brlcadGeometryNode);

      renderer["rendererType"] = rendererType;

      // Calculate sensible default camera position

      auto bbox = brlcadGeometryNode->brlcadBounds;
      vec3f diag = bbox.size();
      diag = max(diag, vec3f(0.3f * length(diag)));

      auto gaze = ospcommon::center(bbox);
      auto pos = gaze - .75f * vec3f(-.6*diag.x, -1.2f*diag.y, .8f*diag.z);
      auto up = vec3f(0.f, 0.f, 1.f);

      auto &camera = renderer["camera"];
      camera["pos"] = pos;
      camera["dir"] = normalize(gaze - pos);
      camera["up"] = up;
      camera.createChild("gaze", "vec3f", gaze);

      // Create window and launch app

      ospray::ImGuiViewer window(renderer_ptr);

      auto &viewPort = window.viewPort;
      auto dir = normalize(viewPort.at - viewPort.from);

      if (renderer["camera"].hasChild("focusdistance"))
        renderer["camera"]["focusdistance"] = length(viewPort.at-viewPort.from);

      window.create("OSPRay BRLCAD Viewer App");

      ospray::imgui3D::run();
      return 0;
    }

  } // ::ospray::brlcad
} // ::ospray
