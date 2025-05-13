#pragma once

#include "FragmentsSettings.hpp"

#include <Model.hpp>
#include <Location.hpp>

bool ExportFragmentsFile (const ModelerAPI::Model& apiModel, const IO::Location& location, const FragmentsExportSettings& settings);
