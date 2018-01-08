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

namespace ospray {
  namespace brlcad {

    /*! constructor - will create the 'ispc equivalent' */
    BRLCAD::BRLCAD()
    {
      this->ispcEquivalent = ispc::BRLCAD_create(this);
    }

    /*! destructor - supposed to clean up all alloced memory */
    BRLCAD::~BRLCAD()
    {
      ispc::BRLCAD_destroy(ispcEquivalent);
    }

    void BRLCAD::commit()
    {
      //TODO
    }

    void BRLCAD::finalize(Model *model)
    {
      //TODO
    }

    OSP_REGISTER_GEOMETRY(BRLCAD, brlcad);

  } // ::ospray::brlcad
} // ::ospray
