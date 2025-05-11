#include "PropertyUtils.hpp"

#include <ACAPI/IFCObjectAccessor.hpp>
#include <ACAPI/IFCPropertyAccessor.hpp>

static const GS::UniString IfcBuildingElementProxy = "IFCBUILDINGELEMENTPROXY";

GS::Optional<GS::Guid> GetParentElemGuid (const API_Guid& elemGuid)
{
    API_HierarchicalOwnerType ownerType = API_ParentHierarchicalOwner;
    API_HierarchicalElemType ownerElemType = API_UnknownElemType;
    API_Guid ownerGuid = {};
    if (ACAPI_HierarchicalEditing_GetHierarchicalElementOwner (&elemGuid, &ownerType, &ownerElemType, &ownerGuid) != NoError) {
        return GS::NoValue;
    }
    if (ownerGuid == APINULLGuid || ownerElemType != API_ChildElemInMultipleElem) {
        return GS::NoValue;
    }
    return APIGuid2GSGuid (ownerGuid);
}

GS::UniString GetIfcType (const GS::Guid& elemGuid)
{
    API_Elem_Head elemHead = {};
    elemHead.guid = GSGuid2APIGuid (elemGuid);
    if (ACAPI_Element_GetHeader (&elemHead) != NoError) {
        return IfcBuildingElementProxy;
    }

    IFCAPI::ObjectAccessor ifcObjectAccessor = IFCAPI::GetObjectAccessor ();
    ACAPI::Result<IFCAPI::ObjectID> ifcObjectId = ifcObjectAccessor.CreateElementObjectID (elemHead);
    if (!ifcObjectId.IsOk ()) {
        return IfcBuildingElementProxy;
    }

    ACAPI::Result<IFCAPI::IFCType> ifcType = ifcObjectAccessor.GetIFCType (ifcObjectId.Unwrap ());
    if (!ifcType.IsOk ()) {
        GS::Optional<GS::Guid> parentGuid = GetParentElemGuid (elemHead.guid);
        if (parentGuid.HasValue ()) {
            return GetIfcType (parentGuid.Get ());
        }
        return IfcBuildingElementProxy;
    }

    return ifcType.Unwrap ().ToUpperCase ();
}

void EnumerateIfcAttributes (const GS::Guid& elemGuid, const std::function<void (const GS::UniString&, const GS::UniString&, const GS::UniString&)>& enumerator)
{
    API_Elem_Head elemHead = {};
    elemHead.guid = GSGuid2APIGuid (elemGuid);
    if (ACAPI_Element_GetHeader (&elemHead) != NoError) {
        return;
    }

    IFCAPI::ObjectAccessor ifcObjectAccessor = IFCAPI::GetObjectAccessor ();
    ACAPI::Result<IFCAPI::ObjectID> ifcObjectId = ifcObjectAccessor.CreateElementObjectID (elemHead);
    if (!ifcObjectId.IsOk ()) {
        return;
    }

    ACAPI::Result<std::vector<IFCAPI::Attribute>> ifcAttributes = IFCAPI::PropertyAccessor (ifcObjectId.Unwrap ()).GetAttributes ();
    if (!ifcAttributes.IsOk ()) {
        GS::Optional<GS::Guid> parentGuid = GetParentElemGuid (elemHead.guid);
        if (parentGuid.HasValue ()) {
            return EnumerateIfcAttributes (parentGuid.Get (), enumerator);
        }
        return;
    }

    for (const IFCAPI::Attribute& ifcAttribute : ifcAttributes.Unwrap ()) {
        std::optional<GS::UniString> value = ifcAttribute.GetValue ();
        if (value.has_value ()) {
            enumerator (
                ifcAttribute.GetName (),
                value.value (),
                ifcAttribute.GetValueType ().ToUpperCase ()
            );
        }
    }
}
