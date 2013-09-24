/*
The MIT License (MIT)

Copyright (c) 2012 Oneironautics (Mike Smith) https://oneironautics.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef __SHAPEFILE_H__
#define __SHAPEFILE_H__

#include <stdint.h>

/*  Return codes. */
typedef enum SFError
{
    SF_ERROR = 0,
    SF_OK = 1
} SFError;

/*  Shape types. */
enum ShapeType
{
    stNull = 0,
    stPoint = 1,
    stPolyline = 3,
    stPolygon = 5,
    stMultiPoint = 8,
    stPointZ = 11,
    stPolyLineZ = 13,
    stPolygonZ = 15,
    stMultiPointZ = 18,
    stPointM = 21,
    stPolyLineM = 23,
    stPolygonM = 25,
    stMultiPointM = 28,
    stMultiPatch = 31
};

/*
 Integer: Signed 32-bit integer (4 bytes)
 Double: Signed 64-bit IEEE double-precision floating point number (8 bytes 

 Position Field Value Type Order
 Byte 0 File Code 9994 Integer Big
 Byte 4 Unused 0 Integer Big
 Byte 8 Unused 0 Integer Big
 Byte 12 Unused 0 Integer Big
 Byte 16 Unused 0 Integer Big
 Byte 20 Unused 0 Integer Big
 Byte 24 File Length File Length Integer Big
 Byte 28 Version 1000 Integer Little
 Byte 32 Shape Type Shape Type Integer Little
 Byte 36 Bounding Box Xmin Double Little
 Byte 44 Bounding Box Ymin Double Little
 Byte 52 Bounding Box Xmax Double Little
 Byte 60 Bounding Box Ymax Double Little
 Byte 68* Bounding Box Zmin Double Little
 Byte 76* Bounding Box Zmax Double Little
 Byte 84* Bounding Box Mmin Double Little
 Byte 92* Bounding Box Mmax Double Little
 * Unused, with value 0.0, if not Measured or Z type
*/
#ifndef _WIN32
typedef struct __attribute__((__packed__)) SFMainFileHeader 
{
    /*  Start big endian. */
    int32_t file_code;
    int32_t unused_0;
    int32_t unused_1;
    int32_t unused_2;
    int32_t unused_3;
    int32_t unused_4;
    int32_t file_length;
    /*  End big endian. */
    /*  Start little endian. */
    int32_t version;
    int32_t shape_type;
    double bb_xmin;
    double bb_ymin;
    double bb_xmax;
    double bb_ymax;
    double bb_zmin;
    double bb_zmax;
    double bb_mmin;
    double bb_mmax;
    /*  End little endian. */
} SFMainFileHeader;
#else
#pragma pack(push, 1)
typedef struct SFMainFileHeader 
{
    /*  Start big endian. */
    int32_t file_code;
    int32_t unused_0;
    int32_t unused_1;
    int32_t unused_2;
    int32_t unused_3;
    int32_t unused_4;
    int32_t file_length;
    /*  End big endian. */
    /*  Start little endian. */
    int32_t version;
    int32_t shape_type;
    double bb_xmin;
    double bb_ymin;
    double bb_xmax;
    double bb_ymax;
    double bb_zmin;
    double bb_zmax;
    double bb_mmin;
    double bb_mmax;
    /*  End little endian. */
} SFMainFileHeader;
#pragma pack(pop)
#endif

/*
Byte 0 Record Number Record Number Integer Big
Byte 4 Content Length Content Length Integer Big
*/
typedef struct SFMainFileRecordHeader
{
    /*  Start big endian. */
    int32_t record_number;
    int32_t content_length;
    /*  End big endian. */
} SFMainFileRecordHeader;

/*
 Byte 0 Offset Offset Integer Big
 Byte 4 Content Length Content Length Integer Big
*/
typedef struct SFIndexRecordHeader
{
    int32_t offset;
    int32_t content_length;
} SFIndexRecordHeader;

/* Null shape.
 
Byte 0 Shape Type 0 Integer 1 Little
 
 */

typedef struct SFNull
{
    int32_t shape_type;
} SFNull;

/* Point shape.
 
Byte 0 Shape Type 1 Integer 1 Little
Byte 4 X X Double 1 Little
Byte 12 Y Y Double 1 Little
 
 */

typedef struct SFPoint
{
    double x;
    double y;
} SFPoint;

/* MultiPoint shape.
 
 Byte 0 Shape Type 8 Integer 1 Little
 Byte 4 Box Box Double 4 Little
 Byte 36 NumPoints NumPoints Integer 1 Little
 Byte 40 Points Points Point NumPoints Little
 
*/

typedef struct SFMultiPoint
{
    double box[4];
    int32_t num_points;
    SFPoint* points;
} SFMultiPoint;

/* PolyLine shape.
 
 Byte 0 Shape Type 3 Integer 1 Little
 Byte 4 Box Box Double 4 Little
 Byte 36 NumParts NumParts Integer 1 Little
 Byte 40 NumPoints NumPoints Integer 1 Little
 Byte 44 Parts Parts Integer NumParts Little
 Byte X Points Points Point NumPoints Little
 
*/

typedef struct SFPolyLine
{
    double box[4];
    int32_t num_parts;
    int32_t num_points;
    int32_t* parts;
    SFPoint* points;    
} SFPolyLine;

/* Polygon shape.
 
 Byte 0 Shape Type 5 Integer 1 Little
 Byte 4 Box Box Double 4 Little
 Byte 36 NumParts NumParts Integer 1 Little
 Byte 40 NumPoints NumPoints Integer 1 Little
 Byte 44 Parts Parts Integer NumParts Little
 Byte X Points Points Point NumPoints Little
 
 */

typedef struct SFPolygon
{
    double box[4];
    int32_t num_parts;
    int32_t num_points;
    int32_t* parts;
    SFPoint* points;
} SFPolygon;


/* PointM shape.
 
 Byte 0 Shape Type 21 Integer 1 Little
 Byte 4 X X Double 1 Little
 Byte 12 Y Y Double 1 Little
 Byte 20 M M Double 1 Little
 
*/

typedef struct SFPointM
{
    double x;
    double y;
    double m;
} SFPointM;

/* MultiPointM shape.
 
 Byte 0 Shape Type 28 Integer 1 Little
 Byte 4 Box Box Double 4 Little
 Byte 36 NumPoints NumPoints Integer 1 Little
 Byte 40 Points Points Point NumPoints Little
 Byte X* Mmin Mmin Double 1 Little
 Byte X+8* Mmax Mmax Double 1 Little
 Byte X+16* Marray Marray Double NumPoints Little
 
*/

typedef struct SFMultiPointM
{
    double box[4];
    int32_t num_points;
    SFPoint* points;
    double m_range[2];
    double* m_array;
} SFMultiPointM;

/* PolyLineM shape.
  
 Byte 0 Shape Type 23 Integer 1 Little
 Byte 4 Box Box Double 4 Little
 Byte 36 NumParts NumParts Integer 1 Little
 Byte 40 NumPoints NumPoints Integer 1 Little
 Byte 44 Parts Parts Integer NumParts Little
 Byte X Points Points Point NumPoints Little
 Byte Y* Mmin Mmin Double 1 Little
 Byte Y + 8* Mmax Mmax Double 1 Little
 Byte Y + 16* Marray Marray Double NumPoints Little
 
*/

typedef struct SFPolyLineM
{
    double box[4];
    int32_t num_parts;
    int32_t num_points;
    int32_t* parts;
    SFPoint* points;
    double m_range[2];
    double* m_array;
} SFPolyLineM;

/* PolygonM shape.
 
 Byte 0 Shape Type 25 Integer 1 Little
 Byte 4 Box Box Double 4 Little
 Byte 36 NumParts NumParts Integer 1 Little
 Byte 40 NumPoints NumPoints Integer 1 Little
 Byte 44 Parts Parts Integer NumParts Little
 Byte X Points Points Point NumPoints Little
 Byte Y* Mmin Mmin Double 1 Little
 Byte Y + 8* Mmax Mmax Double 1 Little
 Byte Y + 16* Marray Marray Double NumPoints Little
 
 */

typedef struct SFPolygonM
{
    double box[4];
    int32_t num_parts;
    int32_t num_points;
    int32_t parts;
    SFPoint* points;
    double m_range[2];
    double* m_array;
} SFPolygonM;

/* PointZ shape.
 
 Byte 0 Shape Type 11 Integer 1 Little
 Byte 4 X X Double 1 Little
 Byte 12 Y Y Double 1 Little
 Byte 20 Z Z Double 1 Little
 Byte 28 Measure M Double 1 Little
 
*/

typedef struct SFPointZ
{
    double x;
    double y;
    double z;
    double m;
} SFPointZ;

/* MultiPointZ shape.
 
 Byte 0 Shape Type 18 Integer 1 Little
 Byte 4 Box Box Double 4 Little
 Byte 36 NumPoints NumPoints Integer 1 Little
 Byte 40 Points Points Point NumPoints Little
 Byte X Zmin Zmin Double 1 Little
 Byte X+8 Zmax Zmax Double 1 Little
 Byte X+16 Zarray Zarray Double NumPoints Little
 Byte Y* Mmin Mmin Double 1 Little
 Byte Y+8* Mmax Mmax Double 1 Little
 Byte Y+16* Marray Marray Double NumPoints Littl
 
*/

typedef struct SFMultiPointZ
{
    double box[4];
    int32_t num_points;
    SFPoint* points;
    double z_range[2];
    double* z_array;
    double m_range[2];
    double* m_array;
} SFMultiPointZ;

/* PolyLineZ shape.
 
 Byte 0 Shape Type 13 Integer 1 Little
 Byte 4 Box Box Double 4 Little
 Byte 36 NumParts NumParts Integer 1 Little
 Byte 40 NumPoints NumPoints Integer 1 Little
 Byte 44 Parts Parts Integer NumParts Little
 Byte X Points Points Point NumPoints Little
 Byte Y Zmin Zmin Double 1 Little
 Byte Y + 8 Zmax Zmax Double 1 Little
 Byte Y + 16 Zarray Zarray Double NumPoints Little
 Byte Z* Mmin Mmin Double 1 Little
 Byte Z+8* Mmax Mmax Double 1 Little
 Byte Z+16* Marray Marray Double NumPoints Littl
 
*/

typedef struct SFPolyLineZ
{
    double box[4];
    int32_t num_parts;
    int32_t num_points;
    int32_t* parts;
    SFPoint* points;
    double z_range[2];
    double* z_array;
    double m_range[2];
    double* m_array;
} SFPolyLineZ;

/* PolygonZ shape.
 
 Byte 0 Shape Type 15 Integer 1 Little
 Byte 4 Box Box Double 4 Little
 Byte 36 NumParts NumParts Integer 1 Little
 Byte 40 NumPoints NumPoints Integer 1 Little
 Byte 44 Parts Parts Integer NumParts Little
 Byte X Points Points Point NumPoints Little
 Byte Y Zmin Zmin Double 1 Little
 Byte Y+8 Zmax Zmax Double 1 Little
 Byte Y+16 Zarray Zarray Double NumPoints Little
 Byte Z* Mmin Mmin Double 1 Little
 Byte Z+8* Mmax Mmax Double 1 Little
 Byte Z+16* Marray Marray Double NumPoints Littl
 
*/

typedef struct SFPolygonZ
{
    double box[4];
    int32_t num_parts;
    int32_t num_points;
    int32_t* parts;
    SFPoint* points;
    double z_range[2];
    double* z_array;
    double m_range[2];
    double* m_array;
} SFPolygonZ;

/*  MultiPatch shape.
 
 Byte 0 Shape Type 31 Integer 1 Little
 Byte 4 Box Box Double 4 Little
 Byte 36 NumParts NumParts Integer 1 Little
 Byte 40 NumPoints NumPoints Integer 1 Little
 Byte 44 Parts Parts Integer NumParts Little
 Byte W PartTypes PartTypes Integer NumParts Little
 Byte X Points Points Point NumPoints Little
 Byte Y Zmin Zmin Double 1 Little
 Byte Y+8 Zmax Zmax Double 1 Little
 Byte Y+16 Zarray Zarray Double NumPoints Little
 Byte Z* Mmin Mmin Double 1 Little
 Byte Z+8* Mmax Mmax Double 1 Little
 Byte Z+16* Marray Marray Double NumPoints Littl
 
*/

typedef struct SFMultiPatch
{
    double box[4];
    int32_t num_parts;
    int32_t num_points;
    int32_t* parts;
    int32_t* part_types;
    SFPoint* points;
    double z_range[2];
    double* z_array;
    double m_range[2];
    double* m_array;
} SFMultiPatch;

#ifdef __cplusplus
extern "C"
{
#endif

/*  Utility functions. */
int32_t byteswap32(int32_t value);
void print_msg(const char* format, ...);

/*  Shape file functions. */
SFError open_shapefile(const char* path);
SFError close_shapefile(void);
SFError validate_shapefile(void);
SFError read_shapes(void);
SFPolygon read_polygon(void);

#ifdef __cplusplus
}
#endif

/*	__SHAPEFILE_H__ */
#endif
