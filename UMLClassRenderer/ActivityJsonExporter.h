#pragma once
#include "ActivityModel.h"
#include "ActivityCoordinateAssigner.h"
#include <string>

void exportActivityToJson(const ActivityDiagram& diagram,
    const ActivityLayoutContext& layout,
    const std::string& outputPath);