#ifndef PROC_H
#define PROC_H

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

#define ALOS_DIR "/mnt/disk/DEM/ALOS2024/"
#define ASTER_DIR "/mnt/disk/DEM/ASTER_V3/"
#define VFP_DIR "/mnt/disk/DEM/VFP/"

// N56E117 -> iPoint(117,56)
iPoint parse_key(const std::string & key);

// get altitude range for DEM data
iPoint get_alt_rng(const ImageR & img, const int step=100);

// for DEM data and altitude range save a color image to fname
void make_color_img(const ImageR & dem,
   const iPoint & arng, const std::string & fname);

// read mask from png file and apply to dem data
void apply_mask(const std::string & fname, ImageR & dem);

// read mask from ovl file and apply to dem data
void apply_ovl(const std::string & fname, ImageR & dem);

// rescale aster data to alos size (for example 3601x3601 -> 1800x3600)
ImageR rescale_aster(const ImageR & dem, const size_t w, const size_t h);

// read ALOS mask (8-bit *_MSK.tif files) and save color image
void make_alos_mask(const std::string & fname, const std::string & ofile);

// make Ozi reference
void make_ref(const std::string & fname, const size_t w, const size_t h);

#endif