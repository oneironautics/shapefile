// ShapefileTestsWindows.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include "Shapefile.h"

extern SFShapes* g_shapes;
extern FILE* g_shapefile;

int test_polygon();
int test_polygonz();

int _tmain(int argc, _TCHAR* argv[])
{
	test_polygon();
	test_polygonz();

	return 0;
}

int test_polygon()
{
	const char* path = "E:\\source\\Shapefile\\TestData\\TM_WORLD_BORDERS_SIMPL-0.3.shp";

	if (open_shapefile(path) == SF_ERROR) {
		return 1;
	}

	read_shapes();

	const SFShapes* shapes = get_shape_records();

	for (uint32_t x = 0; x < shapes->num_records; ++x) {
		const SFShapeRecord* record = get_shape_record(x);
		SFPolygon* polygon = get_polygon_shape(record);
		printf("Polygon %d: num_parts = %d, num_points = %d\n", x, polygon->num_parts, polygon->num_points);
		/* Do things with the polygonz. */

		free_polygon_shape(polygon);
		polygon = 0;

		fflush(stdout);
	}

	close_shapefile();
}

int test_polygonz()
{
	const char* path = "E:\\source\\Shapefile\\TestData\\MyPolyZ.shp";

	if (open_shapefile(path) == SF_ERROR) {
		return 1;
	}

	read_shapes();

	const SFShapes* shapes = get_shape_records();

	for (uint32_t x = 0; x < shapes->num_records; ++x) {
		const SFShapeRecord* record = get_shape_record(x);
		SFPolygonZ* polygonz = get_polygonz_shape(record);
		printf("PolygonZ %d: num_parts = %d, num_points = %d\n", x, polygonz->num_parts, polygonz->num_points);
		/* Do things with the polygonz. */

		free_polygonz_shape(polygonz);
		polygonz = 0;

		fflush(stdout);
	}

	close_shapefile();
}