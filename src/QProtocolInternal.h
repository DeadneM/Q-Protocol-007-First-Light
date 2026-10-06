#pragma once

namespace qp {

// Shared diagnostic sink used by optional runtime probes.
// Writes to the existing QProtocol.log owned by the core.
void QpDiagnosticLogLine(const char* line);

} // namespace qp
