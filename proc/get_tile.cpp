#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "rainbow/rainbow.h"
#include "filename/filename.h"
#include "err/err.h"
#include "opt/opt.h"

#include "image/image_r.h"
#include "image/io.h"
#include "proc.h"

int
main(int argc, char *argv[]){
  try{
    if (argc < 2){
      std::cerr << "usage: get_tile <key> ...\n";
      return 1;
    }
    for (int i = 1; i<argc; i++){
      std::string key = file_get_basename(argv[i]);
      std::cout << "key: " << key << "\n";

      // load alos tile
      ImageR alos = image_load(ALOS_DIR + key + ".tif");

      // apply mask if any
      apply_ovl("../" + key + ".ovl", alos);
//      apply_mask("../" + key + ".png", alos);

      // calculate color range, make color image
      iPoint arng = get_alt_rng(alos);
      make_color_img(alos, arng, key + ".tif");

      // same Ozi map-file
//      make_ref(key + ".tif", alos.width(), alos.height());

      // save alos mask if any;
      make_alos_mask(ALOS_DIR + key + "_MSK.tif", key + "_msk.png");

      // load aster tile
      ImageR aster = image_load(ASTER_DIR + key + ".tif");
      aster = rescale_aster(aster, alos.width(), alos.height());

      make_color_img(aster, arng, key + "_aster.tif");
    }
  }
  catch (Err & e) {
    if (e.str()!="")
      std::cerr << "Error: " << e.str() << "\n";
    return 1;
  }
  return 0;
}

