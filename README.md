shapefile
=========

This library was written to add basic ESRI shapefile support as a static library to iOS projects. It is not complete by any stretch of the imagination.

Usage:

open_shapefile("E:\\source\\Shapefile\\TestData\\MyPolyZ.shp");
read_shapes();
const SFShapes* shapes = get_shape_records();

for ( uint32_t x = 0; x < shapes->num_records; ++x ) {
    const SFShapeRecord* record = get_shape_record(x);
    SFPolygonZ* polygonz = get_polygonz_shape(record);

    /* Do things with the polygonz. */

    free_polygon_shape(polygonz);
    polygonz = 0;
}

close_shapefile();
