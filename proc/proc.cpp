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

#include "geo_data/geo_io.h"

#include "proc.h"

iPoint
parse_key(const std::string & key){
  iPoint p(0,0);
  char mode = ' ';
  for (const auto c:key){
    if (c=='W' || c=='E'){
      p.x=0; mode = c;
      continue;
    }
    if (c=='N' || c=='S'){
      p.y=0; mode = c;
      continue;
    }
    if (c>='0' && c<'9'){
      switch (mode){
        case 'E': p.x = p.x*10 + (int)(c-'0'); break;
        case 'W': p.x = p.x*10 - (int)(c-'0'); break;
        case 'N': p.y = p.y*10 + (int)(c-'0'); break;
        case 'S': p.y = p.y*10 - (int)(c-'0'); break;
      }
      continue;
    }
    mode = ' ';
  }
  return p;
}

/************************************************/
iPoint
get_alt_rng(const ImageR & dem, const int step){
  if (dem.type()!=IMAGE_16) throw Err() << "wrong DEM image type";
  auto w = dem.width(), h = dem.height();
  std::map<int, size_t> cols;
  // build hystgram with 100m steps
  for (size_t x = 0; x<w; x++){
    for (size_t y = 0; y<h; y++){
      int16_t v = dem.get16(x,y)/step;
      cols[v] = cols.count(v) ? cols[v]+1 : 0;
    }
  }
  iPoint ret(+10000, -1000);
  for (const auto & c:cols){
    if (c.first < -1000/step || c.first>10000/step || c.second<100) continue;
    if (ret.x > c.first) ret.x = c.first;
    if (ret.y < c.first) ret.y = c.first;
  }
  ret.y++;
  ret*=step;
  std::cout << "  altitude range: " << ret << " m\n";
  return ret;
}

/************************************************/
void
make_color_img(const ImageR & dem, const iPoint & arng, const std::string & fname){
  if (dem.type()!=IMAGE_16) throw Err() << "wrong DEM image type";
  auto w = dem.width(), h = dem.height();
  ImageR cimg(w, h, IMAGE_32ARGB);

  iPoint pt = parse_key(fname);

  Rainbow R(arng.x, arng.y, RAINBOW_NORMAL);
  for (size_t x = 0; x<w; x++){
    for (size_t y = 0; y<h; y++){
      int16_t v = dem.get16(x,y);
      if (v == -32768) {
        cimg.set32(x,y, 0);
        continue;
      }

      // get gradient
      double dx=0, dy=0;
      double v1,v2,v3,v4;
      v1=v2=v3=v4=v;
      if (x>0)   {v1 = dem.get16(x-1,y); dx+=1;}
      if (x<w-1) {v2 = dem.get16(x+1,y); dx+=1;}
      if (y>0)   {v3 = dem.get16(x,y-1); dy+=1;}
      if (y<h-1) {v4 = dem.get16(x,y+1); dy+=1;}

      double lat = pt.y + 1.0 - (double)y/h;
      dx *= 1.0/w * 6380e3 * M_PI/180;
      dy *= 1.0/h * 6380e3 * M_PI/180;
      dx *= cos(M_PI*lat/180.0);
      double U = 0;
      if (dx!=0 && dy!=0) U = hypot((v2-v1)/dx, (v4-v3)/dy);
      U = atan(U)*180.0/M_PI;
      cimg.set32(x,y, color_shade(R.get(v), 1-U/90.0));
    }
  }
  image_save(cimg, fname);
}

/************************************************/
void
apply_mask(const std::string & fname, ImageR & dem){
  if (!file_exists(fname)) return;
  std::cerr << "  applying existing mask: ";

  if (dem.type()!=IMAGE_16) throw Err() << "wrong DEM image type";
  auto w = dem.width(), h = dem.height();

  ImageR msk = image_load(fname);
  if (msk.type()!=IMAGE_8PAL) throw Err() << "wrong mask type: " << msk.type();
  if (msk.width() != w || msk.height() != h)
    throw Err() << "wrong mask dimensions: " << msk.width() << "x" << msk.height() << "\n";

  size_t i=0;
  for (size_t x = 0; x<w; x++){
    for (size_t y = 0; y<h; y++){
      if (msk.get_rgb(x,y) == 0xFF000000){
        dem.set16(x, y, -32768);
        i++;
      }
    }
  }
  std::cerr << i << " points\n";
}

/************************************************/
void
apply_ovl(const std::string & fname, ImageR & dem){
  if (!file_exists(fname)) return;
  std::cerr << "  applying existing mask: ";

  if (dem.type()!=IMAGE_16) throw Err() << "wrong DEM image type";
  auto w = dem.width(), h = dem.height();

  FILE *F = fopen(fname.c_str(), "rb");
  if (F) {
    size_t i=0;
    while (!feof(F)){
      int16_t x,y,v;
      if (fread(&x, sizeof(x),1, F) &&
          fread(&y, sizeof(y),1, F) &&
          fread(&v, sizeof(v),1, F)){
        dem.set16(x, y, -32768);
        i++;
      }
    }
    fclose(F);
    std::cerr << i << " points\n";
  }
}

/************************************************/
ImageR
rescale_aster(const ImageR & dem, const size_t w, const size_t h){
  if (dem.type()!=IMAGE_16) throw Err() << "wrong image type";
  auto ws = dem.width(), hs = dem.height();

  ImageR out(w, h, IMAGE_16);
  for (size_t x = 0; x<w; x++){
    for (size_t y = 0; y<h; y++){
      dPoint sp( ((double)x + 0.5) * (double)(ws-1)/w,
                 ((double)y + 0.5) * (double)(hs-1)/h );
      double v = dem.get_double_int4(sp);
      out.set16(x,y, rint(v));
    }
  }
  return out;
}

/************************************************/
void
make_alos_mask(const std::string & fname, const std::string & ofile){

  if (!file_exists(fname)) return;

  auto img  = image_load(fname);
  if (img.type()!=IMAGE_8) throw Err() << "wrong ALOS mask type\n";
  auto w = img.width(), h = img.height();
  ImageR imgc(w,h, IMAGE_24RGB);

  // 0000 0000 (0x00): Valid
  // 0000 0001 (0x01): Cloud and snow mask (invalid)
  // 0000 0010 (0x02): Land water and low correlation mask*4 (valid)
  // 0000 0011 (0x03): Sea mask*5 (valid)
  // 0000 0100 (0x04): GSI DTM*6 (valid)
  // 0000 1000 (0x08): SRTM-1 v3*7 (valid)
  // 0000 1100 (0x08): PRISM DSM (valid)
  // 0001 0000 (0x10): ViewFinder Panoramas DEM*8 (valid)
  // 0001 1000 (0x18): ASTER GDEM v2*9 (valid)
  // 0001 1100 (0x1C): ArcticDEM v2*10 (valid)
  // 0010 0000 (0x20):TanDEM-X 90m DEM*11 (valid)
  // 0010 0100 (0x24):ArcticDEM v3*10 (valid)
  // 0010 1000 (0x28):ASTER GDEM v3*9 (valid)
  // 0010 1100 (0x2C): REMA v1.1*12 (valid)
  // 0011 0000 (0x30):Copernicus DEM GLO-30*13(valid)
  // 0011 0100 (0x34):ArcticDEM v4*10 (valid)
  // 1111 1100 (0xFC): applied IDW method (gdal_fillnodata) (valid)

  for (size_t x = 0; x<w; x++){
    for (size_t y = 0; y<h; y++){
      uint8_t v = img.get8(x,y);
      int32_t c = 0;
      switch(v){
       case 0x00: c = 0x00FF00; break; // valid - green
       case 0x01: c = 0xFF0000; break; // invalid - red
       case 0x02: c = 0x00FFFF; break; // water/low corr - cyan
       case 0x03: c = 0x0000FF; break; // sea - blue
       case 0x04: c = 0xFFFF00; break;
       case 0x08: c = 0x800000; break;
       case 0x09: c = 0x808000; break;
       case 0x10: c = 0x008000; break;
       case 0x18: c = 0x008080; break;
       case 0x1C: c = 0x000080; break;
       case 0x20: c = 0x800080; break;
       case 0x24: c = 0x600000; break;
       case 0x28: c = 0x606000; break;
       case 0x2C: c = 0x006000; break;
       case 0x30: c = 0x006060; break;
       case 0x34: c = 0x000060; break; // ArcticDEM v4
       case 0xFC: c = 0x808080; break;
      }
      imgc.set24(x,y,c);
    }
  }
  image_save(imgc , ofile);
}

/************************************************/
void
make_ref(const std::string & fname, const size_t w, const size_t h){
  GeoMap ref;
  ref.name = ref.image = fname;
  ref.proj = "WGS";
  iPoint p0 = parse_key(fname);
  for (const auto & pt: dLine("[[0,0],[1,0],[1,1],[0,1]]")){
    dPoint pr(pt), pl(pt);
    pr.y = 1-pr.y;
    pr *= dPoint(w-1,h-1);
    pl += p0;
    ref.ref.emplace(pr, pl);
    ref.border.add_point(pr);
  }
  write_ozi_map(fname + ".map", ref);
}

