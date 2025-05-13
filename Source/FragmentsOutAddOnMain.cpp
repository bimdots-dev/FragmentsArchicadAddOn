#include <ACAPinc.h>

#include <Model.hpp>
#include <Sight.hpp>
#include <IAttributeReader.hpp>
#include <exp.h>

#include "DebugUtils.hpp"
#include "FragmentsExporter.hpp"
#include "ResourceIds.hpp"

static const GSType FileTypeId = 1;

#ifdef DEBUG

static GSErrCode MenuCommandHandler (const API_MenuParams* menuParams)
{
    switch (menuParams->menuItemRef.menuResID) {
        case ID_ADDON_MENU:
            switch (menuParams->menuItemRef.itemIndex) {
                case ID_ADDON_MENU_DUMP_FRAG:
                    DumpFragmentsFile ();
                    break;
            }
            break;
    }

    return NoError;
}

#endif

GSErrCode GetAPIModel (Modeler::SightPtr sight, ModelerAPI::Model* model)
{
    GS::Owner<Modeler::IAttributeReader> attributeReader (ACAPI_Attribute_GetCurrentAttributeSetReader ());
    if (EXPGetModel (sight, model, attributeReader.Get ()) != NoError) {
        return Error;
    }
    return NoError;
}

static GSErrCode ExportFragmentsFromSaveAs (const API_IOParams* ioParams, Modeler::SightPtr sight)
{
    ModelerAPI::Model model;
    if (GetAPIModel (sight, &model) != NoError) {
        return APIERR_GENERAL;
    }

    FragmentsExportSettings settings;
    settings.compressionMode = CompressionMode::Raw;
    if (!ExportFragmentsFile (model, *ioParams->fileLoc, settings)) {
        return APIERR_GENERAL;
    }

    return NoError;
}

static GSErrCode FragmentsFileTypeHandler (const API_IOParams* ioParams, Modeler::SightPtr sight)
{
    GSErrCode result = Error;
    if (ioParams->method == IO_SAVEAS3D) {
        result = ExportFragmentsFromSaveAs (ioParams, sight);
    }
    return result;
}

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
    RSGetIndString (&envir->addOnInfo.name, ID_ADDON_INFO, 1, ACAPI_GetOwnResModule ());
    RSGetIndString (&envir->addOnInfo.description, ID_ADDON_INFO, 2, ACAPI_GetOwnResModule ());

    return APIAddon_Normal;
}

GSErrCode RegisterInterface (void)
{
#ifdef DEBUG
    ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU, 0, MenuCode_Tools, MenuFlag_Default);
#endif

    GSErrCode err = ACAPI_AddOnIntegration_RegisterFileType (
        FileTypeId,
        'FRAG',
        'GSAC',
        "frag",
        ID_ADDON_ICON,
        ID_ADDON_STRS,
        ID_ADDON_STR_FORMAT_NAME,
        SaveAs3DSupported
    );
    if (err != NoError) {
        return err;
    }

    return NoError;
}

GSErrCode Initialize (void)
{
#ifdef DEBUG
    ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU, MenuCommandHandler);
#endif

    GSErrCode err = ACAPI_AddOnIntegration_InstallFileTypeHandler3D (FileTypeId, FragmentsFileTypeHandler);
    if (err != NoError) {
        return err;
    }

    return NoError;
}

GSErrCode FreeData (void)
{
    return NoError;
}
