// ShapefileTestsWindows.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include "Shapefile.h"

int test_polygon();
int test_polygonz();
int test_polyline();

int _tmain(int argc, _TCHAR* argv[])
{
    test_polygon();
    test_polygonz();
    test_polyline();

    return 0;
}

int test_polygon()
{
    const char* path = "E:\\source\\Shapefile\\TestData\\TM_WORLD_BORDERS_SIMPL-0.3.shp";

    FILE* pShapefile = open_shapefile(path);

    if ( pShapefile == 0 ) {
        return 1;
    }

    SFShapes* pShapes = read_shapes(pShapefile);

    for ( uint32_t x = 0; x < pShapes->num_records; ++x ) {
        const SFShapeRecord* record = pShapes->records[x];
        SFPolygon* polygon = get_polygon_shape(pShapefile, record);
        printf("Polygon %d: num_parts = %d, num_points = %d\n", x, polygon->num_parts, polygon->num_points);
        /* Do things with the polygon. */

        free_polygon_shape(polygon);
        polygon = 0;

        fflush(stdout);
    }

    free_shapes(pShapes);
    close_shapefile(pShapefile);

    return 0;
}

int test_polyline()
{
    const char* path = "E:\\source\\Shapefile\\TestData\\tgr48201lkH.shp";

    FILE* pShapefile = open_shapefile(path);

    if ( pShapefile == 0) {
        return 1;
    }

    SFShapes* pShapes = read_shapes(pShapefile);

    for ( uint32_t x = 0; x < pShapes->num_records; ++x ) {
        const SFShapeRecord* record = pShapes->records[x];
        SFPolyLine* polyline = get_polyline_shape(pShapefile, record);
        printf("Polyline %d: num_parts = %d, num_points = %d\n", x, polyline->num_parts, polyline->num_points);
        /* Do things with the polygon. */

        free_polyline_shape(polyline);
        polyline = 0;

        fflush(stdout);
    }

    free_shapes(pShapes);
    close_shapefile(pShapefile);

    return 0;
}

int test_polygonz()
{
    const char* path = "E:\\source\\Shapefile\\TestData\\MyPolyZ.shp";

    FILE* pShapefile = open_shapefile(path);

    if ( pShapefile == 0) {
        return 1;
    }

    SFShapes* pShapes = read_shapes(pShapefile);

    for ( uint32_t x = 0; x < pShapes->num_records; ++x ) {
        const SFShapeRecord* record = get_shape_record(pShapes, x);
        SFPolygonZ* polygonz = get_polygonz_shape(pShapefile, record);
        printf("PolygonZ %d: num_parts = %d, num_points = %d\n", x, polygonz->num_parts, polygonz->num_points);
        /* Do things with the polygonz. */

        free_polygonz_shape(polygonz);
        polygonz = 0;

        fflush(stdout);
    }

    free_shapes(pShapes);
    close_shapefile(pShapefile);

    return 0;
}