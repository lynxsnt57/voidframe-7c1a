#pragma once
#include <QString>

// Converts bounded local glTF/GLB to a hierarchical Mine-imator model.
// Animation clips are not transferred; morphs are baked at default weights.
// Unsupported required extensions fail explicitly; approximations need confirmation.
// On success returns the temporary .mimodel path; otherwise returns ERROR: ...
// filterMode: 0 = all, 1 = only filterNode, 2 = all except filterNode, -1 = mesh picker, -2 = experimental rigid-bone picker, -3 = experimental smooth-bone picker.
QString importStaticGlb(const QString& fileName, int filterMode=0, int filterNode=-1, bool rigidBones=false, bool smoothBones=false);
