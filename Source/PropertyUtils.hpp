#pragma once

#include <ACAPinc.h>

GS::UniString GetIfcType (const GS::Guid& elemGuid);
void EnumerateIfcAttributes (const GS::Guid& elemGuid, const std::function<void (const GS::UniString&, const GS::UniString&, const GS::UniString&)>& enumerator);
