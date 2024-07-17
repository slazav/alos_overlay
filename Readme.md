### alos_overlay -- fixing ALOS DEM data

I'm using ALOS DEM data for my mountain maps
( https://slazav.xyz/maps/hr.htm ).

Unforunately, they have quite many data artefacts in some regions. Here
I'm fixing ALOS V4.1 data by cutting holes manually (see png masks) and
filling them by some other data (ASTER DEM V3).

Result is published as `*.png` mask files and `*.ovl` files with
fixed data.

Format of `*.ovl` files is simple: it's a binary file with a sequence of
records, one for each fixed point. Each record contains three `int16_t`
numbers: `x,y,z`.

I'm using libraries from mapsoft2 project ( https://github.com/slazav/mapsoft2 )
ovl files can be used there to have access to the fixed data.

### Example

Pamir mountains (part of N38E072), original and fixed data:

![example 1](https://raw.githubusercontent.com/slazav/alos_overlay/main/ex.gif)



