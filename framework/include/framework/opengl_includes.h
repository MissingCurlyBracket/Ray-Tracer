#pragma once
#include "disable_all_warnings.h"
// Suppress warnings in third-party code.
DISABLE_WARNINGS_PUSH()
#if WIN32
#include <gl/glew.h>
#else
#include <GL/glew.h>
#endif
DISABLE_WARNINGS_POP()