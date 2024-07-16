#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "rainbow/rainbow.h"
#include "filename/filename.h"
#include "err/err.h"
#include "opt/opt.h"
#include "image/image_r.h"
#include "image/io.h"

/************************************************/
ImageR
rescale_aster(const ImageR & img, const size_t w, const size_t h){
  if (img.type()!=IMAGE_16) throw Err() << "wrong image type";
  auto ws = img.width(), hs = img.height();

  ImageR dimg(w, h, IMAGE_16);
  for (size_t x = 0; x<w; x++){
    for (size_t y = 0; y<h; y++){
      double sx = ((double)x + 0.5) * (double)(ws-1)/w;
      double sy = ((double)y + 0.5) * (double)(hs-1)/h;
      int x1 = floor(sx), x2 = floor(sx)+1;
      int y1 = floor(sy), y2 = floor(sy)+1;
      double dx = sx-x1, dy = sy-y1;
      auto v1 = img.get16(x1,y1);
      auto v2 = img.get16(x1,y2);
      auto v3 = img.get16(x2,y1);
      auto v4 = img.get16(x2,y2);
      dimg.set16(x,y, rint(v1*(1-dx)*(1-dy) + v2*(1-dx)*dy + v3*dx*(1-dy) + v4*dx*dy));
    }
  }
  return dimg;
}

/************************************************/
int
main(int argc, char *argv[]){
  try{

    if (argc < 2){
      std::cerr << "usage: get_tile <key> ...\n";
      return 1;
    }
    for (int i = 1; i<argc; i++){
      std::string key = file_get_basename(argv[i]);
      std::cerr << key << "\n";
      // load file
      auto alos = image_load(key + ".tif");
      auto w = alos.width(), h = alos.height();
      if (alos.type()!=IMAGE_32ARGB) throw Err() << "wrong image type\n";

      // save MASK image
      std::map<iPoint, int> ovl;
      ImageR msk(w,h, IMAGE_1);
      for (size_t x = 0; x<w; x++){
        for (size_t y = 0; y<h; y++){
           bool v = (alos.get32(x,y) >> 24) < 0xFF;
           msk.set1(x,y, v);
        }
      }
      image_save(msk, key + ".png");

      // load aster file
      ImageR aster = image_load("/mnt/disk/DEM/ASTER_V3/" + key + ".tif");
      aster = rescale_aster(aster, alos.width(), alos.height());

      std::ofstream out(key + ".ovl");
      for (int16_t x = 0; x<w; x++){
        for (int16_t y = 0; y<h; y++){
           if (!msk.get1(x,y)) continue;
           int16_t v=aster.get16(x,y);
           out.write((const char*)&x, sizeof(v));
           out.write((const char*)&y, sizeof(v));
           out.write((const char*)&v, sizeof(v));
        }
      }
    }
  }
  catch (Err & e) {
    if (e.str()!="")
      std::cerr << "Error: " << e.str() << "\n";
    return 1;
  }
  return 0;
}

///\endcond