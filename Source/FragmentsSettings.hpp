#pragma once

#include <Object.hpp>

enum class CompressionMode : Int32
{
    Raw = 0,
    Compressed = 1,
};

class FragmentsExportSettings : public GS::Object
{
    DECLARE_CLASS_INFO;

public:
    FragmentsExportSettings ();

    virtual GSErrCode Read (GS::IChannel& ic) override;
    virtual GSErrCode Write (GS::OChannel& oc) const override;

    CompressionMode compressionMode;
};
