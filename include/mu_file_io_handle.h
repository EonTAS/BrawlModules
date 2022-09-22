#pragma once

#include <types.h>
#include <memory.h>
#include <gf/gf_file_io.h>
class muFileIOHandle : public gfFileIOHandle
{
public:
    // 0
    muFileIOHandle(){};
    ~muFileIOHandle(){};
};