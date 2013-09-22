//
//  main.c
//  ShapefileTest
//
//  Created by Michael Smith on 3/24/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include "Shapefile.h"

extern SFPolygon* g_polygons;
extern int32_t g_polygons_index;
extern FILE* g_shapefile;
const char* path = "/Users/smith/Downloads/TM_WORLD_BORDERS_SIMPL-0.3/TM_WORLD_BORDERS_SIMPL-0.3.shp";

int main(int argc, char** argv)
{
    if ( open_shapefile(path) == SF_ERROR ) {
        return 1;
    }
    
    if ( validate_shapefile() == SF_ERROR ) {
        return 1;
    }
    
    read_shapes();
    
    for ( int32_t x = 0; x < g_polygons_index; ++x ) {
        SFPolygon polygon = g_polygons[x];
        printf("Polygon %d: num_parts = %d, num_points = %d\n", x, polygon.num_parts, polygon.num_points);
        fflush(stdout);
    }
    
    close_shapefile();

    return 0;
}

