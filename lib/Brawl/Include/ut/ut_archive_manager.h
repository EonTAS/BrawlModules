#pragma once
#include <nw4r/g3d/g3d_resfile.h>
#include <so/so_archive_db.h>
class utArchiveManager : public soArchiveManager
{
public:
    nw4r::g3d::ResFile *getResFileFromId(int id, int i, int x, int c, int k);
};
