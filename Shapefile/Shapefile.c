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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "Shapefile.h"

FILE* g_shapefile;
int32_t g_shape_type;
int32_t g_shapefile_length;
const char* g_path;
SFPolygon* g_polygons;
int32_t g_polygons_index;

int32_t byteswap32(int32_t value)
{
#ifndef _WIN32
    value = __builtin_bswap32(value);
#else
    value = _byteswap_ulong(value);
#endif
    
    return value;
}

void print_msg(const char* format, ...)
{
	char buf[1024];

	va_list args;
	va_start(args, format);
	
#ifndef _WIN32
	sprintf(buf, format, args);
#else
	sprintf_s(buf, 1024, format, args);
#endif

	va_end(args);
}

SFError open_shapefile(const char* path)
{
    g_path = path;

#ifndef _WIN32
    g_shapefile = fopen(g_path, "r");
#else
	fopen_s(&g_shapefile, g_path, "r");
#endif
    
    if ( g_shapefile == 0 ) {
        print_msg("Could not open shape file <%s>.", g_path);
        return SF_ERROR;
    }
    
    return SF_OK;
}

SFError validate_shapefile(void)
{
    SFMainFileHeader header;
    
    fread(&header, sizeof(SFMainFileHeader), 1, g_shapefile);
    
    if ( ferror(g_shapefile) ) {
        print_msg("Could not read shape file <%s>.\n", g_path);
        return SF_ERROR;
    }
    
    if ( byteswap32(header.file_code) != 9994 || header.version != 1000 ) {
        print_msg("File <%s> is not a shape file.\n", g_path);
        return SF_ERROR;
    }
    
    g_shape_type = header.shape_type;
    g_shapefile_length = byteswap32(header.file_length);
    
    if ( g_shape_type == stPolygon ) {
        g_polygons_index = 0;
        g_polygons = (SFPolygon*)malloc(sizeof(SFPolygon) * 247);
    }
    
    return SF_OK;
}

SFError read_shapes(void)
{
    while ( !feof(g_shapefile) ) {
        /*  Read a file record header. */
        SFMainFileRecordHeader header;
        int32_t shape_type;
        
        size_t bytes_read = fread(&header, sizeof(SFMainFileRecordHeader), 1, g_shapefile);
        if ( bytes_read != 1 ) {
            /*  End of file reached, or something more catastrophic. */
            break;
        }
        fread(&shape_type, sizeof(int32_t), 1, g_shapefile);
        
#ifdef DEBUG
        print_msg("Record number: %d, size %d, shape type %d.\n", byteswap32(header.record_number), byteswap32(header.content_length), shape_type);
        fflush(stdout);
#endif
        switch ( shape_type ) {
            case 5:
                g_polygons[g_polygons_index] = read_polygon();
                g_polygons_index++;
                break;
            default:
                print_msg("Found unsupported shape type <%d>", shape_type);
                fflush(stdout);
                /*  Skips to the next record. For testing purposes. */
                fseek(g_shapefile, byteswap32(header.content_length) * sizeof(int16_t), SEEK_CUR);
        }
    }
    
    return SF_OK;
}

SFPolygon read_polygon(void)
{
    double box[4];
    int32_t num_parts = 0;
    int32_t num_points = 0;
	SFPolygon polygon;
    
    fread(box, sizeof(double) * 4, 1, g_shapefile);
    fread(&num_parts, sizeof(int32_t), 1, g_shapefile);
    fread(&num_points, sizeof(int32_t), 1, g_shapefile);
    
    polygon.box[0] = box[0];
    polygon.box[1] = box[1];
    polygon.box[2] = box[2];
    polygon.box[3] = box[3];
    polygon.num_parts = num_parts;
    polygon.num_points = num_points;

    polygon.parts = (int32_t*)malloc(sizeof(int32_t) * num_parts);
    polygon.points = (SFPoint*)malloc(sizeof(SFPoint) * num_points);
    
    fread(polygon.parts, sizeof(int32_t), num_parts, g_shapefile);
    fread(polygon.points, sizeof(SFPoint), num_points, g_shapefile);
    
    return polygon;
}

SFError close_shapefile()
{
    if ( g_shapefile != NULL ) {
        fclose(g_shapefile);
        g_shapefile = NULL;
    }
    if ( g_polygons != NULL ) {
		int32_t x = 0;
        for ( ; x < g_polygons_index; ++x ) {
            SFPolygon polygon = g_polygons[x];
            if ( polygon.parts != NULL ) {
                free(polygon.parts);
                polygon.parts = NULL;
            }
            if ( polygon.points != NULL ) {
                free(polygon.points);
                polygon.points = NULL;
            }
        }
        
        free(g_polygons);
        g_polygons = NULL;
    }
    
    return SF_OK;
}
