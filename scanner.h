#ifndef __SCANNER_H
#define __SCANNER_H

#define DIRECTIVE_ORG   0x01
#define DIRECTIVE_ERROR 0xFF

#include "token.h"

Clip_t *assembly_to_clip(const char *file_path);

#endif
