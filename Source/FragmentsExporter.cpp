#include "FragmentsExporter.hpp"

#include <ModelElement.hpp>
#include <ModelMeshBody.hpp>
#include <ModelMaterial.hpp>
#include <ConvexPolygon.hpp>
#include <AttributeIndex.hpp>

#include <File.hpp>

#include <Transformation3D.hpp>

#include <miniz.h>

#include "Schema/index_generated.h"
#include "PropertyUtils.hpp"

static const Transform IdentityTransform (DoubleVector (0.0, 0.0, 0.0), FloatVector (1.0f, 0.0f, 0.0f), FloatVector (0.0f, 1.0f, 0.0f));
static Geometry::Transformation3D SetUpVectorToY = Geometry::Transformation3D::CreateRotationX (-PI * 0.5);

class BodyVertex
{
public:
    BodyVertex (Int32 bodyIndex, Int32 vertexIndex) :
        bodyIndex (bodyIndex),
        vertexIndex (vertexIndex)
    {

    }

    ULong GenerateHashValue () const
    {
        return GS::CalculateHashValue (bodyIndex, vertexIndex);
    }

    bool operator== (const BodyVertex& rhs) const
    {
        return bodyIndex == rhs.bodyIndex && vertexIndex == rhs.vertexIndex;
    }

    Int32 bodyIndex;
    Int32 vertexIndex;
};

class BodyPolygon
{
public:
    BodyPolygon (Int32 bodyIndex, const ModelerAPI::MeshBody& body, const ModelerAPI::Polygon& polygon) :
        bodyIndex (bodyIndex),
        body (body),
        polygon (polygon)
    {

    }

    Int32 bodyIndex;
    ModelerAPI::MeshBody body;
    ModelerAPI::Polygon polygon;
};

namespace std
{

template <>
struct hash<ModelerAPI::AttributeIndex>
{
    size_t operator() (const ModelerAPI::AttributeIndex& val) const noexcept
    {
        return val.GenerateHashValue ();
    }
};

template <>
struct hash<BodyVertex>
{
    size_t operator() (const BodyVertex& val) const noexcept
    {
        return val.GenerateHashValue ();
    }
};

}

static bool IsEmptyElement (const ModelerAPI::Element& element)
{
    for (Int32 bodyIndex = 1; bodyIndex <= element.GetTessellatedBodyCount (); ++bodyIndex) {
        ModelerAPI::MeshBody body;
        element.GetTessellatedBody (bodyIndex, &body);
        if (body.GetPolygonCount () > 0) {
            return false;
        }
    }
    return true;
}

class MeshListBuilder
{
public:
    MeshListBuilder (flatbuffers::FlatBufferBuilder& fbBuilder, const ModelerAPI::Model& model) :
        fbBuilder (fbBuilder),
        model (model),
        usedMaterials (),
        fbCoordinates (IdentityTransform),
        fbMeshesItems (),
        fbSamples (),
        fbRepresentations (),
        fbMaterials (),
        fbCircleExtrusions (),
        fbShells (),
        fbLocalTransforms ({ IdentityTransform }),
        fbGlobalTransforms ()
    {

    }

    void AddElement (const ModelerAPI::Element& element)
    {
        uint32_t meshItemId = (uint32_t) fbMeshesItems.size ();
        fbMeshesItems.push_back (meshItemId);
        fbGlobalTransforms.push_back (IdentityTransform);

        std::unordered_map<ModelerAPI::AttributeIndex, std::vector<BodyPolygon>> polygonsByMaterial;
        std::vector<ModelerAPI::AttributeIndex> materials;
        GetPolygonsByMaterial (element, polygonsByMaterial, materials);

        for (const ModelerAPI::AttributeIndex& materialIndex : materials) {
            std::vector<flatbuffers::Offset<ShellProfile>> fbProfiles;
            std::vector<flatbuffers::Offset<ShellHole>> fbHoles;
            std::vector<FloatVector> fbPoints;
            std::unordered_map<BodyVertex, uint16_t> bodyVertexIndexToPoint;
            Vector3D min (MaxDouble, MaxDouble, MaxDouble);
            Vector3D max (-MaxDouble, -MaxDouble, -MaxDouble);
            const std::vector<BodyPolygon>& polygons = polygonsByMaterial.at (materialIndex);
            for (const BodyPolygon& polygon : polygons) {
                for (Int32 convexPolygonIndex = 1; convexPolygonIndex <= polygon.polygon.GetConvexPolygonCount (); ++convexPolygonIndex) {
                    ModelerAPI::ConvexPolygon convexPolygon;
                    polygon.polygon.GetConvexPolygon (convexPolygonIndex, &convexPolygon);
                    std::vector<uint16_t> fbShellProfileIndices;
                    for (Int32 vertexIndex = 1; vertexIndex <= convexPolygon.GetVertexCount (); vertexIndex++) {
                        BodyVertex bodyVertexIndex (polygon.bodyIndex, convexPolygon.GetVertexIndex (vertexIndex));
                        uint16_t fbVertexIndex = 0;
                        auto foundVertex = bodyVertexIndexToPoint.find (bodyVertexIndex);
                        if (foundVertex == bodyVertexIndexToPoint.end ()) {
                            ModelerAPI::Vertex vertex;
                            polygon.body.GetVertex (bodyVertexIndex.vertexIndex, &vertex, ModelerAPI::CoordinateSystem::World);
                            Vector3D rotated = SetUpVectorToY.Apply_V (Vector3D (vertex.x, vertex.y, vertex.z));

                            fbVertexIndex = (uint16_t) fbPoints.size ();
                            fbPoints.push_back (FloatVector ((float) rotated.x, (float) rotated.y, (float) rotated.z));
                            bodyVertexIndexToPoint.insert ({ bodyVertexIndex, fbVertexIndex });

                            min.x = GS::Min (min.x, rotated.x);
                            min.y = GS::Min (min.y, rotated.y);
                            min.z = GS::Min (min.z, rotated.z);
                            max.x = GS::Max (max.x, rotated.x);
                            max.y = GS::Max (max.y, rotated.y);
                            max.z = GS::Max (max.z, rotated.z);
                        } else {
                            fbVertexIndex = foundVertex->second;
                        }
                        fbShellProfileIndices.push_back (fbVertexIndex);
                    }
                    flatbuffers::Offset<ShellProfile> fbShellProfile = CreateShellProfileDirect (fbBuilder, &fbShellProfileIndices);
                    fbProfiles.push_back (fbShellProfile);
                }
            }

            BoundingBox fbBoundingBox (
                FloatVector ((float) min.x, (float) min.y, (float) min.z),
                FloatVector ((float) max.x, (float) max.y, (float) max.z)
            );
            Representation fbRepresentation ((uint32_t) fbRepresentations.size (), fbBoundingBox, RepresentationClass_SHELL);
            uint32_t fbRepresentationIndex = (uint32_t) fbRepresentations.size ();
            fbRepresentations.push_back (fbRepresentation);

            uint32_t fbMaterialIndex = 0;
            auto foundMaterial = usedMaterials.find (materialIndex);
            if (foundMaterial == usedMaterials.end ()) {
                ModelerAPI::Material material;
                model.GetMaterial (materialIndex, &material);
                ModelerAPI::Color color = material.GetSurfaceColor ();
                Material fbMaterial (
                    (uint8_t) (color.red * 255.0),
                    (uint8_t) (color.green * 255.0),
                    (uint8_t) (color.blue * 255.0),
                    (uint8_t) ((1.0 - material.GetTransparency ()) * 255.0),
                    RenderedFaces_TWO,
                    Stroke_DEFAULT
                );
                fbMaterials.push_back (fbMaterial);
                fbMaterialIndex = (uint32_t) fbMaterials.size () - 1;
                usedMaterials.insert ({ materialIndex, fbMaterialIndex });
            } else {
                fbMaterialIndex = foundMaterial->second;
            }
            uint32_t fbLocalTransform = 0;
            Sample fbSample (meshItemId, fbMaterialIndex, fbRepresentationIndex, fbLocalTransform);
            fbSamples.push_back (fbSample);

            flatbuffers::Offset<Shell> fbShell = CreateShellDirect (fbBuilder, &fbProfiles, &fbHoles, &fbPoints);
            fbShells.push_back (fbShell);
        }
    }

    flatbuffers::Offset<Meshes> CreateMeshes ()
    {
        return CreateMeshesDirect (
            fbBuilder,
            &fbCoordinates,
            &fbMeshesItems,
            &fbSamples,
            &fbRepresentations,
            &fbMaterials,
            &fbCircleExtrusions,
            &fbShells,
            &fbLocalTransforms,
            &fbGlobalTransforms
        );
    }

    void GetPolygonsByMaterial (
        const ModelerAPI::Element& element,
        std::unordered_map<ModelerAPI::AttributeIndex, std::vector<BodyPolygon>>& polygonsByMaterial,
        std::vector<ModelerAPI::AttributeIndex>& materials)
    {
        for (Int32 bodyIndex = 1; bodyIndex <= element.GetTessellatedBodyCount (); ++bodyIndex) {
            ModelerAPI::MeshBody body;
            element.GetTessellatedBody (bodyIndex, &body);
            for (Int32 polygonIndex = 1; polygonIndex <= body.GetPolygonCount (); ++polygonIndex) {
                ModelerAPI::Polygon polygon;
                body.GetPolygon (polygonIndex, &polygon);
                if (polygon.IsInvisible ()) {
                    continue;
                }
                ModelerAPI::AttributeIndex materialIndex;
                polygon.GetMaterialIndex (materialIndex);
                auto found = polygonsByMaterial.find (materialIndex);
                if (found == polygonsByMaterial.end ()) {
                    polygonsByMaterial.insert ({ materialIndex, { BodyPolygon (bodyIndex, body, polygon) } });
                    materials.push_back (materialIndex);
                } else {
                    found->second.push_back (BodyPolygon (bodyIndex, body, polygon));
                }
            }
        }
    }

    flatbuffers::FlatBufferBuilder& fbBuilder;
    const ModelerAPI::Model& model;
    std::unordered_map<ModelerAPI::AttributeIndex, uint32_t> usedMaterials;

    Transform fbCoordinates;
    std::vector<uint32_t> fbMeshesItems;
    std::vector<Sample> fbSamples;
    std::vector<Representation> fbRepresentations;
    std::vector<Material> fbMaterials;
    std::vector<flatbuffers::Offset<CircleExtrusion>> fbCircleExtrusions;
    std::vector<flatbuffers::Offset<Shell>> fbShells;
    std::vector<Transform> fbLocalTransforms;
    std::vector<Transform> fbGlobalTransforms;
};

static bool WriteContentToFile (const IO::Location& location, const std::uint8_t* content, size_t size)
{
    IO::File file (location, IO::File::OnNotFound::Create);
    if (file.Open (IO::File::OpenMode::WriteEmptyMode) != NoError) {
        return false;
    }

    if (file.WriteBin ((char*) content, (GS::USize) size) != NoError) {
        return false;
    }

    file.Close ();
    return true;
}

bool ExportFragmentsFile (const ModelerAPI::Model& model, const IO::Location& location, const FragmentsExportSettings& settings)
{
    flatbuffers::FlatBufferBuilder builder;
    MeshListBuilder meshListBuilder (builder, model);

    GS::Guid projectGuid (GS::Guid::GenerateGuid);
    const char* fbMetaData = "{}";
    std::vector<flatbuffers::Offset<flatbuffers::String>> fbGuids;
    std::vector<uint32_t> fbGuidsItems;
    std::vector<uint32_t> fbLocalIds;
    std::vector<flatbuffers::Offset<Attribute>> fbAttributes;

    std::vector<flatbuffers::Offset<flatbuffers::String>> fbCategories;

    uint32_t elementLocalId = 1;
    for (Int32 elementIndex = 1; elementIndex <= model.GetElementCount (); ++elementIndex) {
        ModelerAPI::Element element;
        model.GetElement (elementIndex, &element);
        if (element.IsInvalid () || IsEmptyElement (element)) {
            continue;
        }

        GS::Guid elemGuid = element.GetElemGuid ();
        fbGuids.push_back (builder.CreateString (elemGuid.ToString ().ToCStr ()));
        fbGuidsItems.push_back (elementLocalId);
        fbLocalIds.push_back (elementLocalId);
        meshListBuilder.AddElement (element);

        GS::UniString ifcType = GetIfcType (elemGuid);
        fbCategories.push_back (builder.CreateString (ifcType.ToCStr (CC_UTF8).Get ()));

        std::vector<flatbuffers::Offset<flatbuffers::String>> attributeValues;
        EnumerateIfcAttributes (elemGuid, [&](const GS::UniString& name, const GS::UniString& value, const GS::UniString& type) {
            GS::UniString attributeJson = GS::UniString::Printf ("[\"%T\",\"%T\",\"%T\"]",
                name.ToPrintf (),
                value.ToPrintf (),
                type.ToPrintf ()
                );
            attributeValues.push_back (builder.CreateString (attributeJson.ToCStr (CC_UTF8).Get ()));
        });
        flatbuffers::Offset<Attribute> attribute = CreateAttributeDirect (builder, &attributeValues);
        fbAttributes.push_back (attribute);

        elementLocalId += 1;
    }

    uint32_t fbMaxLocalId = elementLocalId;
    flatbuffers::Offset<Meshes> fbMeshes = meshListBuilder.CreateMeshes ();
    flatbuffers::Offset<Model> fbModel = CreateModelDirect (
        builder,
        fbMetaData,
        &fbGuids,
        &fbGuidsItems,
        fbMaxLocalId,
        &fbLocalIds,
        &fbCategories,
        fbMeshes,
        &fbAttributes,
        nullptr,
        nullptr,
        projectGuid.ToString ().ToCStr (),
        0,
        nullptr,
        0
    );

    // Do not use FinishModelBuffer to avoid writing identifier
    builder.Finish (fbModel);

    bool successfulWrite = false;
    if (settings.compressionMode == CompressionMode::Raw) {
        successfulWrite = WriteContentToFile (location, builder.GetBufferPointer (), builder.GetSize ());
    } else if (settings.compressionMode == CompressionMode::Compressed) {
        mz_ulong compressedBound = mz_compressBound (builder.GetSize ());
        std::uint8_t* compressedBuffer = new std::uint8_t[compressedBound];
        mz_ulong compressedLength = compressedBound;
        int compressStatus = mz_compress (compressedBuffer, &compressedLength, builder.GetBufferPointer (), builder.GetSize ());
        if (compressStatus == MZ_OK) {
            successfulWrite = WriteContentToFile (location, compressedBuffer, compressedLength);
        }
        delete[] compressedBuffer;
    }

    return successfulWrite;
}
