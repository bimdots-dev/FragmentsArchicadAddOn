#include "FragmentsSettings.hpp"

GS::ClassInfo FragmentsExportSettings::classInfo ("FragmentsExportSettings", GS::Guid ("5E707044-9008-49A4-90CD-0BE9B6F52AE5"), GS::ClassVersion (1, 0));

FragmentsExportSettings::FragmentsExportSettings () :
    GS::Object (),
    compressionMode (CompressionMode::Raw)
{

}

GSErrCode FragmentsExportSettings::Read (GS::IChannel& ic)
{
    GS::InputFrame frame (ic, classInfo);
    ic.ReadEnum<Int32, CompressionMode> (compressionMode);
    return ic.GetInputStatus ();
}

GSErrCode FragmentsExportSettings::Write (GS::OChannel& oc) const
{
    GS::OutputFrame frame (oc, classInfo);
    oc.WriteEnum<Int32, CompressionMode> (compressionMode);
    return oc.GetOutputStatus ();
}
