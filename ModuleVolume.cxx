/******************************************************************************

  This source file is part of the TEM tomography project.

  Copyright Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/
#include "ModuleVolume.h"

#include "DataSource.h"
#include "pqProxiesWidget.h"
#include "Utilities.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

namespace TEM
{

//-----------------------------------------------------------------------------
ModuleVolume::ModuleVolume(QObject* parentObject) : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
ModuleVolume::~ModuleVolume()
{
  this->finalize();
}

//-----------------------------------------------------------------------------
QIcon ModuleVolume::icon() const
{
  return QIcon(":/pqWidgets/Icons/pqVolumeData16.png");
}

//-----------------------------------------------------------------------------
bool ModuleVolume::initialize(DataSource* dataSource, vtkSMViewProxy* view)
{
  if (!this->Superclass::initialize(dataSource, view))
    {
    return false;
    }

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  vtkSMSessionProxyManager* pxm =
      dataSource->producer()->GetSessionProxyManager();

  // Create the pass through filter.
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("filters", "PassThrough"));

  this->PassThrough = vtkSMSourceProxy::SafeDownCast(proxy);
  Q_ASSERT(this->PassThrough);
  controller->PreInitializeProxy(this->PassThrough);
  vtkSMPropertyHelper(this->PassThrough, "Input").Set(dataSource->producer());
  controller->PostInitializeProxy(this->PassThrough);
  controller->RegisterPipelineProxy(this->PassThrough);

  // Create the representation for it.
  this->Representation = controller->Show(this->PassThrough, 0, view);
  Q_ASSERT(this->Representation);
  vtkSMRepresentationProxy::SetRepresentationType(this->Representation,
                                                  "Volume");

  this->updateColorMap();
  this->Representation->UpdateVTKObjects();
  return true;
}

//-----------------------------------------------------------------------------
void ModuleVolume::updateColorMap()
{
  Q_ASSERT(this->Representation);
  vtkSMPropertyHelper(this->Representation,
                      "LookupTable").Set(this->colorMap());
  vtkSMPropertyHelper(this->Representation,
                      "ScalarOpacityFunction").Set(this->opacityMap());
  this->Representation->UpdateVTKObjects();

  // BUG: volume mappers don't update property when LUT is changed and has an
  // older Mtime. Fix for now by forcing the LUT to update.
  vtkObject::SafeDownCast(this->colorMap()->GetClientSideObject())->Modified();
}

//-----------------------------------------------------------------------------
bool ModuleVolume::finalize()
{
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  controller->UnRegisterProxy(this->Representation);
  controller->UnRegisterProxy(this->PassThrough);

  this->PassThrough = NULL;
  this->Representation = NULL;
  return true;
}

//-----------------------------------------------------------------------------
bool ModuleVolume::setVisibility(bool val)
{
  Q_ASSERT(this->Representation);
  vtkSMPropertyHelper(this->Representation, "Visibility").Set(val ? 1 : 0);
  this->Representation->UpdateVTKObjects();
  return true;
}

//-----------------------------------------------------------------------------
bool ModuleVolume::visibility() const
{
  Q_ASSERT(this->Representation);
  return vtkSMPropertyHelper(this->Representation,
                             "Visibility").GetAsInt() != 0;
}

//-----------------------------------------------------------------------------
bool ModuleVolume::serialize(pugi::xml_node& ns) const
{
  QStringList list;
  list << "Visibility"
       << "ScalarOpacityUnitDistance";
  pugi::xml_node nodeR = ns.append_child("Representation");
  return (TEM::serialize(this->Representation, nodeR, list) && this->Superclass::serialize(ns));
}

//-----------------------------------------------------------------------------
bool ModuleVolume::deserialize(const pugi::xml_node& ns)
{
  vtkSMProxy* lut = vtkSMPropertyHelper(this->Representation,
                                        "LookupTable").GetAsProxy();
  vtkSMProxy* sof = vtkSMPropertyHelper(this->Representation,
                                        "ScalarOpacityFunction").GetAsProxy();

  if (!TEM::deserialize(this->Representation, ns.child("Representation")))
    {
    return false;
    }

  return this->Superclass::deserialize(ns);
}

} // end of namespace TEM
