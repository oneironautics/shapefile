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

/*    int32_t byteswap32(int32_t value)

A cross-platform helper for flipping endianess.

Arguments:
    value: the un-byteswapped value.

Returns:
    int32_t: the byteswapped value.

*/
int32_t byteswap32(int32_t value)
{
#ifndef _WIN32
    value = __builtin_bswap32(value);
#else
    value = _byteswap_ulong(value);
#endif
    
    return value;
}

/*    void print_msg(const char* format, ...)

A cross-platform helper for sprintf to use sprintf_s on Windows platforms.

Arguments:
    format: the printf-esque format string.
    ...: varargs of the printf formatters.

Returns:
    N/A.

*/
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

/*    SFResult open_shapefile(const char* path)

Opens a shapefile for reading.

Arguments:
    const char* path: the path to the shapefile to open.
    
Returns:
    SF_ERROR: the shapefile could not be opened.
    SF_OK: the shapefile could be opened.
*/
SFResult open_shapefile(const char* path)
{
    g_path = path;

#ifndef _WIN32
    g_shapefile = fopen(g_path, "rb");
#else
    fopen_s(&g_shapefile, g_path, "rb");
#endif
    
    if ( g_shapefile == 0 ) {
        print_msg("Could not open shape file <%s>.", g_path);
        return SF_ERROR;
    }
    
    return SF_OK;
}

/*    SFResult validate_shapefile(void)

Validates that a file is an ESRI shapefile based on the header.

Arguments:
    N/A.
    
Returns:
    SF_ERROR: the file was not a valid ESRI shapefile.
    SF_OK: the file is a valid ESRI shapefile.
*/
SFResult validate_shapefile(void)
{
    SFMainFileHeader header;
    int32_t record_length;
    
    fread(&header, sizeof(SFMainFileHeader), 1, g_shapefile);
    
    if ( ferror(g_shapefile) ) {
        print_msg("Could not read shape file <%s>.\n", g_path);
        return SF_ERROR;
    }
    
    if ( byteswap32(header.file_code) != SHAPEFILE_FILE_CODE || header.version != SHAPEFILE_VERSION ) {
        print_msg("File <%s> is not a shape file.\n", g_path);
        return SF_ERROR;
    }
    
    g_shape_type = header.shape_type;
    g_shapefile_length = byteswap32(header.file_length);
    record_length = byteswap32(header.file_length) - sizeof(SFMainFileHeader);

    switch ( g_shape_type ) {
        case stNull:
        case stPoint:
        case stPolyline:
            print_msg("Shape type <%d> not implemented.\n", g_shape_type);
            break;
        case stPolygon:
            g_polygons[g_polygons_index] = read_polygon();
            g_polygons_index++;
            break;
        case stMultiPoint:
        case stPointZ:
        case stPolyLineZ:
        case stPolygonZ:
        case stMultiPointZ:
        case stPointM:
        case stPolyLineM:
        case stPolygonM:
        case stMultiPointM:
        case stMultiPatch:
        default:
            print_msg("Encountered unsupported shape type <%d>\n", g_shape_type);
            return SF_ERROR;
    }
    
    return SF_OK;
}

/*    SFResult read_shapes(void)

Reads shapes from a shapefile.

Arguments:
    N/A.
    
Returns:
    SF_ERROR: an error was encountered during reading of shapefiles.
    SF_OK: no errors occurred.
*/
SFResult read_shapes(void)
{
    while ( !feof(g_shapefile) ) {
        /*  Read a file record header. */
        SFMainFileRecordHeader header;
        int32_t shape_type;
        
        size_t bytes_read = fread(&header, sizeof(SFMainFileRecordHeader), 1, g_shapefile);

        if ( bytes_read != 1 ) {
            return SF_ERROR;
        }

        fread(&shape_type, sizeof(int32_t), 1, g_shapefile);

#ifdef DEBUG
        print_msg("Record number: %d, size %d, shape type %d.\n", byteswap32(header.record_number), byteswap32(header.content_length), shape_type);
        fflush(stdout);
#endif

        switch ( shape_type ) {
            case stNull:
            case stPoint:
            case stPolyline:
                print_msg("Shape type <%d> not implemented.\n", shape_type);
                fseek(g_shapefile, byteswap32(header.content_length) * sizeof(int16_t), SEEK_CUR);
                break;
            case stPolygon:
                g_polygons[g_polygons_index] = read_polygon();
                g_polygons_index++;
                break;
            case stMultiPoint:
            case stPointZ:
            case stPolyLineZ:
            case stPolygonZ:
            case stMultiPointZ:
            case stPointM:
            case stPolyLineM:
            case stPolygonM:
            case stMultiPointM:
            case stMultiPatch:
            default:
                print_msg("Encountered unsupported shape type <%d>\n", shape_type);
                fflush(stdout);
                /*  Skips to the next record. For testing purposes. */
                fseek(g_shapefile, byteswap32(header.content_length) * sizeof(int16_t), SEEK_CUR);
        }
    }
    
    return SF_OK;
}

/*    SFResult read_polygon(void)

Reads a polygon from a shapefile.

Arguments:
    N/A.
    
Returns:
    SFPolygon: the polygon read from the shapefile.
*/
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

/*    SFResult close_shapefile(void)

Closes the shapefile and frees all memory associated with it.

Arguments:
    N/A.
    
Returns:
    SF_OK: no errors occurred.
*/
SFResult close_shapefile(void)
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
