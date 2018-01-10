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

#undef UNUSED
#undef _USE_MATH_DEFINES
#include "brlcad/common.h"
#include "brlcad/vmath.h"		/* vector math macros */
#include "brlcad/raytrace.h"	/* librt interface definitions */

namespace ospray {
  namespace brlcad {

    std::string filename;
    std::string object;

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

    rt_i *loadBrlcadGeometry(std::string filename, std::string object)
    {
      auto *rtip = rt_dirbuild(filename.c_str(), nullptr, 0);
      rt_gettree(rtip, object.c_str());
      rt_prep_parallel(rtip, tasking::numTaskingThreads());
      return rtip;
    }

    static inline void parseCommandLine(int ac, const char **&av)
    {
      for (int i = 1; i < ac; i++) {
        const std::string arg = av[i];
        if (arg == "-g" || arg == "--geometry") {
          filename = av[++i];
        } else if (arg == "-o" || arg == "--object") {
          object = av[++i];
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

      ospray::imgui3D::init(&ac,av);

      box3f worldBounds;

      auto renderer_ptr = sg::createNode("renderer", "Renderer");
      auto &renderer = *renderer_ptr;

      auto &win_size = ospray::imgui3D::ImGui3DWidget::defaultInitSize;
      renderer["frameBuffer"]["size"] = win_size;

      renderer["rendererType"] = std::string("raycast");

      auto &world = renderer["world"];

      auto &brlcadInstance = world.createChild("brlcad_instance", "Instance");

      auto brlcadGeometryNode = std::make_shared<BrlcadSGNode>();
      brlcadGeometryNode->setName("loaded_example_brlcad");
      brlcadGeometryNode->setType("BrlcadSGNode");

      parseCommandLine(ac, av);
      auto *brlcadGeom = loadBrlcadGeometry(filename, object);

      brlcadGeometryNode->brlcadBounds.lower.x = brlcadGeom->mdl_min[0];
      brlcadGeometryNode->brlcadBounds.lower.y = brlcadGeom->mdl_min[1];
      brlcadGeometryNode->brlcadBounds.lower.z = brlcadGeom->mdl_min[2];
      brlcadGeometryNode->brlcadBounds.upper.x = brlcadGeom->mdl_max[0];
      brlcadGeometryNode->brlcadBounds.upper.y = brlcadGeom->mdl_max[1];
      brlcadGeometryNode->brlcadBounds.upper.z = brlcadGeom->mdl_max[2];

      auto brlcadDataNode =
          std::make_shared<sg::DataArrayRAW>((byte_t*)brlcadGeom, 1, false);
      brlcadDataNode->setName("brlcad_handle");
      brlcadDataNode->setType("DataArrayRAW");
      brlcadGeometryNode->add(brlcadDataNode);
      brlcadInstance["model"].add(brlcadGeometryNode);

      ospray::ImGuiViewer window(renderer_ptr);

      auto &viewPort = window.viewPort;
      auto dir = normalize(viewPort.at - viewPort.from);

      renderer["camera"]["dir"] = dir;
      renderer["camera"]["pos"] = viewPort.from;
      renderer["camera"]["up"]  = viewPort.up;
      renderer["camera"]["fovy"] = viewPort.openingAngle;
      renderer["camera"]["apertureRadius"] = viewPort.apertureRadius;
      if (renderer["camera"].hasChild("focusdistance"))
        renderer["camera"]["focusdistance"] = length(viewPort.at - viewPort.from);

      window.create("OSPRay BRLCAD Viewer App");

      ospray::imgui3D::run();
      return 0;
    }

  } // ::ospray::brlcad
} // ::ospray
