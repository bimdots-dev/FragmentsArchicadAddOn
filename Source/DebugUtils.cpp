#include "DebugUtils.hpp"

#include <ACAPinc.h>
#include <DGFileDialog.hpp>

#include "Schema/index_generated.h"

static GS::Optional<IO::Location> SelectFile ()
{
    DG::FileDialog fileDialog (DG::FileDialog::Type::OpenFile);
    FTM::FileTypeManager fileTypeManager ("FragmentsFile");
    FTM::FileType fileType ("Fragments File", "frag", 0, 0, 0);
    FTM::TypeID fileTypeID = fileTypeManager.AddType (fileType);
    fileDialog.AddFilter (fileTypeID);
    if (!fileDialog.Invoke ()) {
        return GS::NoValue;
    }
    return fileDialog.GetSelectedFile ();
}

static bool GetFileContent (const IO::Location& location, std::vector<std::uint8_t>& content)
{
    IO::File file (location);
    if (file.GetStatus () != NoError) {
        return false;
    }

    if (file.Open (IO::File::OpenMode::ReadMode) != NoError) {
        return false;
    }

    GS::USize dataLength = 0;
    if (file.GetDataLength (&dataLength) != NoError) {
        return false;
    }

    std::uint8_t* contentBuffer = new std::uint8_t[dataLength];
    if (file.ReadBin ((char*) contentBuffer, dataLength) != NoError) {
        delete[] contentBuffer;
        return false;
    }

    content.clear ();
    content.reserve (dataLength);
    content.assign (contentBuffer, contentBuffer + dataLength);
    delete[] contentBuffer;

    file.Close ();
    return true;
}

static void WriteReport (const GS::UniString& report)
{
    ACAPI_WriteReport (report, false);
}

void DumpFragmentsFile ()
{
    GS::Optional<IO::Location> location = SelectFile ();
    if (!location.HasValue ()) {
        return;
    }

    std::vector<std::uint8_t> content;
    if (!GetFileContent (location.Get (), content)) {
        return;
    }

    flatbuffers::Verifier verifier (content.data (), content.size ());
    // Do not use VerifyModelBuffer to avoid checking identifier
    if (!verifier.VerifyBuffer<Model> (nullptr)) {
        WriteReport ("Invalid fragments file.");
        return;
    }

    const Model* model = GetModel (content.data ());

    WriteReport ("--- guid ---");
    auto guid = model->guid ();
    if (guid != nullptr) {
        WriteReport (guid->c_str ());
    }

    WriteReport ("--- metadata ---");
    auto metadata = model->metadata ();
    if (metadata != nullptr) {
        WriteReport (metadata->c_str ());
    }

    WriteReport ("--- guids ---");
    auto guids = model->guids ();
    if (guids != nullptr) {
        for (flatbuffers::uoffset_t i = 0; i < guids->size (); i++) {
            auto item = guids->Get (i);
            WriteReport (item->c_str ());
        }
    }

    WriteReport ("--- guids_items ---");
    auto guids_items = model->guids_items ();
    if (guids_items != nullptr) {
        for (flatbuffers::uoffset_t i = 0; i < guids_items->size (); i++) {
            auto item = guids_items->Get (i);
            WriteReport (GS::ValueToUniString (item));
        }
    }

    WriteReport ("--- max_local_id ---");
    WriteReport (GS::ValueToUniString (model->max_local_id ()));

    WriteReport ("--- local_ids ---");
    auto local_ids = model->local_ids ();
    if (local_ids != nullptr) {
        for (flatbuffers::uoffset_t i = 0; i < local_ids->size (); i++) {
            auto item = local_ids->Get (i);
            WriteReport (GS::ValueToUniString (item));
        }
    }

    WriteReport ("--- categories ---");
    auto categories = model->categories ();
    if (categories != nullptr) {
        for (flatbuffers::uoffset_t i = 0; i < categories->size (); i++) {
            auto item = categories->Get (i);
            WriteReport (item->c_str ());
        }
    }

    WriteReport ("--- meshes ---");
    auto meshes = model->meshes ();
    if (meshes != nullptr) {
        WriteReport ("--- meshes_items ---");
        auto meshes_items = meshes->meshes_items ();
        if (meshes_items != nullptr) {
            for (flatbuffers::uoffset_t i = 0; i < meshes_items->size (); i++) {
                auto item = meshes_items->Get (i);
                WriteReport (GS::ValueToUniString (item));
            }
        }

        WriteReport ("--- samples ---");
        auto samples = meshes->samples ();
        if (samples != nullptr) {
            for (flatbuffers::uoffset_t i = 0; i < samples->size (); i++) {
                auto item = samples->Get (i);
                WriteReport (GS::UniString::Printf ("item: %d, material: %d, representation: %d, local_transform: %d",
                    item->item (),
                    item->material (),
                    item->representation (),
                    item->local_transform ()
                ));
            }
        }

        WriteReport ("--- representations ---");
        auto representations = meshes->representations ();
        if (representations != nullptr) {
            for (flatbuffers::uoffset_t i = 0; i < representations->size (); i++) {
                auto item = representations->Get (i);
                WriteReport (GS::UniString::Printf ("id: %d, representation_class: %d",
                    item->id (),
                    item->representation_class ()
                ));
            }
        }

        WriteReport ("--- materials ---");
        auto materials = meshes->materials ();
        if (materials != nullptr) {
            for (flatbuffers::uoffset_t i = 0; i < materials->size (); i++) {
                auto item = materials->Get (i);
                WriteReport (GS::UniString::Printf ("r: %d, g: %d, b: %d, a: %d",
                    item->r (),
                    item->g (),
                    item->b (),
                    item->a ()
                ));
            }
        }

        WriteReport ("--- shells ---");
        auto shells = meshes->shells ();
        if (shells != nullptr) {
            for (flatbuffers::uoffset_t i = 0; i < shells->size (); i++) {
                auto item = shells->Get (i);
                WriteReport (GS::UniString::Printf ("profiles: %d, holes: %d, points: %d",
                    item->profiles ()->size (),
                    item->holes ()->size (),
                    item->points ()->size ()
                ));
#ifdef DUMP_SHELL_DETAILS
                WriteReport ("points:");
                for (flatbuffers::uoffset_t j = 0; j < item->points ()->size (); j++) {
                    auto point = item->points ()->Get (j);
                    WriteReport (GS::UniString::Printf ("%f, %f, %f",
                        point->x (),
                        point->y (),
                        point->z ()
                    ));
                }
                WriteReport ("profiles:");
                for (flatbuffers::uoffset_t j = 0; j < item->profiles ()->size (); j++) {
                    auto profile = item->profiles ()->Get (j);
                    GS::UniString profileIndices;
                    for (flatbuffers::uoffset_t k = 0; k < profile->indices ()->size (); k++) {
                        auto index = profile->indices ()->Get (k);
                        profileIndices += GS::ValueToUniString (index) + " ";
                    }
                    WriteReport (profileIndices);
                }
#endif
            }
        }

        WriteReport ("--- coordinates ---");
        auto coordinates = meshes->coordinates ();
        if (coordinates != nullptr) {
            WriteReport (GS::UniString::Printf ("pos: %f, %f, %f, x_dir: %f, %f, %f, y_dir: %f, %f, %f",
                coordinates->position ().x (),
                coordinates->position ().y (),
                coordinates->position ().z (),
                coordinates->x_direction ().x (),
                coordinates->x_direction ().y (),
                coordinates->x_direction ().z (),
                coordinates->y_direction ().x (),
                coordinates->y_direction ().y (),
                coordinates->y_direction ().z ()
            ));
        }

        WriteReport ("--- local_transforms ---");
        auto local_transforms = meshes->local_transforms ();
        if (local_transforms != nullptr) {
            for (flatbuffers::uoffset_t i = 0; i < local_transforms->size (); i++) {
                auto transform = local_transforms->Get (i);
                WriteReport (GS::UniString::Printf ("pos: %f, %f, %f, x_dir: %f, %f, %f, y_dir: %f, %f, %f",
                    transform->position ().x (),
                    transform->position ().y (),
                    transform->position ().z (),
                    transform->x_direction ().x (),
                    transform->x_direction ().y (),
                    transform->x_direction ().z (),
                    transform->y_direction ().x (),
                    transform->y_direction ().y (),
                    transform->y_direction ().z ()
                ));
            }
        }

        WriteReport ("--- global_transforms ---");
        auto global_transforms = meshes->global_transforms ();
        if (global_transforms != nullptr) {
            for (flatbuffers::uoffset_t i = 0; i < global_transforms->size (); i++) {
                auto transform = global_transforms->Get (i);
                WriteReport (GS::UniString::Printf ("pos: %f, %f, %f, x_dir: %f, %f, %f, y_dir: %f, %f, %f",
                    transform->position ().x (),
                    transform->position ().y (),
                    transform->position ().z (),
                    transform->x_direction ().x (),
                    transform->x_direction ().y (),
                    transform->x_direction ().z (),
                    transform->y_direction ().x (),
                    transform->y_direction ().y (),
                    transform->y_direction ().z ()
                ));
            }
        }

        ACAPI_Dialog_ActivateSessionReport ();
    }

    WriteReport ("--- attributes ---");
    auto attributes = model->attributes ();
    if (attributes != nullptr) {
        for (flatbuffers::uoffset_t i = 0; i < attributes->size (); i++) {
            WriteReport ("attributes:");
            auto data = attributes->Get (i)->data ();
            for (flatbuffers::uoffset_t j = 0; j < data->size (); j++) {
                WriteReport (data->Get (j)->c_str ());
            }
        }
    }
}
