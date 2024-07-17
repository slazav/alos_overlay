#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <deque>
#include "rainbow/rainbow.h"
#include "filename/filename.h"
#include "err/err.h"
#include "opt/opt.h"
#include "image/image_r.h"
#include "image/io.h"
#include "geom/point_int.h"

#include "proc.h"

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
      auto img = image_load(key + ".tif");
      auto w = img.width(), h = img.height();
      if (img.type()!=IMAGE_32ARGB) throw Err() << "wrong image type\n";

      // save MASK image
      std::map<iPoint, int> ovl;
      ImageR msk(w,h, IMAGE_1);
      for (size_t x = 0; x<w; x++){
        for (size_t y = 0; y<h; y++){
           bool v = (img.get32(x,y) >> 24) < 0xFF;
           msk.set1(x,y, v);
        }
      }
      image_save(msk, key + ".png");

      // load alos and aster files
      ImageR alos = image_load(ALOS_DIR + key + ".tif");
      ImageR aster = image_load(ASTER_DIR + key + ".tif");
      aster = rescale_aster(aster, alos.width(), alos.height());
      if (alos.width()!=w || alos.height()!=h)
        throw Err() << "bad image dimensions";


      std::ofstream out(key + ".ovl");
      for (int16_t x = 0; x<w; x++){
        for (int16_t y = 0; y<h; y++){
          if (!msk.get1(x,y)) continue;

          // find hole as a set of points
          std::set<iPoint> S;
          std::deque<iPoint> Q;
          iPoint p(x,y);
          S.insert(p);
          Q.push_back(p);
          while (Q.size()){
            p = *Q.begin();
            Q.pop_front();
            for (int i=0; i<8; i++){
              iPoint p2 = adjacent(p, i);
              if (!msk.check_crd(p2.x,p2.y) || S.count(p2) ||
                  !msk.get1(p2.x,p2.y)) continue;
              S.insert(p2);
              Q.push_back(p2);
            }
          }

          // find border of the set
          // for each point find difference between aster and alos.
          std::set<iPoint> B;
          for (const auto & p:S){
            for (int i=0; i<8; i++){
              iPoint p2=adjacent(p, i);
              if (S.count(p2) || !msk.check_crd(p2.x,p2.y)) continue;
              p2.z = aster.get16(p2.x,p2.y) - alos.get16(p2.x,p2.y);
              B.insert(p2);
            }
          }

          // make overlay points, fill the hole in the mask
          for (const auto & p:S){
            // aster value
            int16_t v=aster.get16(p.x,p.y);
            //correction: interpolation of aster-alos
            double s0=0.0, s1=0.0;
            for (const auto & p2:B){
              double dd = pow(dist2d(p,p2),2);
              s0 += 1.0/dd;
              s1 += p2.z/dd;
            }
            v -= rint(s1/s0);
            msk.set1(p.x, p.y, 0);
            int16_t x1 = p.x, y1=p.y;
            out.write((const char*)&x1, sizeof(v));
            out.write((const char*)&y1, sizeof(v));
            out.write((const char*)&v, sizeof(v));
          }
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