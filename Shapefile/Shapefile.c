//
//  Shapefile.c
//  Shapefile
//
//  Created by Michael Smith on 3/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
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

SFError open_shapefile(const char* path)
{
    g_path = path;
    g_shapefile = fopen(g_path, "r");
    
    if ( g_shapefile == 0 ) {
        char buf[1024];
        sprintf(buf, "Could not open shape file <%s>.", g_path);
        return SF_ERROR;
    }
    
    return SF_OK;
}

SFError validate_shapefile(void)
{
    SFMainFileHeader header;
    
    fread(&header, sizeof(SFMainFileHeader), 1, g_shapefile);
    
    if ( ferror(g_shapefile) ) {
        char buf[1024];
        sprintf(buf, "Could not read shape file <%s>.\n", g_path);
        return SF_ERROR;
    }
    
    if ( byteswap32(header.file_code) != 9994 || header.version != 1000 ) {
        char buf[1024];
        sprintf(buf, "File <%s> is not a shape file.\n", g_path);
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
        printf("Record number: %d, size %d, shape type %d.\n", byteswap32(header.record_number), byteswap32(header.content_length), shape_type);
        fflush(stdout);
#endif
        switch ( shape_type ) {
            case 5:
                g_polygons[g_polygons_index] = read_polygon();
                g_polygons_index++;
                break;
            default:
                printf("Found unsupported shape type <%d>", shape_type);
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
    
    fread(box, sizeof(double) * 4, 1, g_shapefile);
    fread(&num_parts, sizeof(int32_t), 1, g_shapefile);
    fread(&num_points, sizeof(int32_t), 1, g_shapefile);
    
    SFPolygon polygon;
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
        for ( int32_t x = 0; x < g_polygons_index; ++x ) {
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
