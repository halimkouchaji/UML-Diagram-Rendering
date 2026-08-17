#pragma once
#include "Model.h"
#include <string>

// Parses an XMI file into our internal ClassDiagram model.
// Returns an empty ClassDiagram if the file can't be read
// or doesn't contain the expected structure.
ClassDiagram parseXmiFile(const std::string& filepath);