shapefile
=========

This library was originally written to add basic ESRI shapefile support to iOS projects. It is a work in progress.

Usage:

    open_shapefile("E:\\source\\Shapefile\\TestData\\MyPolyZ.shp");
    read_shapes();
    const SFShapes* shapes = get_shape_records();

    for ( uint32_t x = 0; x < shapes->num_records; ++x ) {
        const SFShapeRecord* record = get_shape_record(x);
        SFPolygonZ* polygonz = get_polygonz_shape(record);

        /* Do things with the polygonz. */

        free_polygonz_shape(polygonz);
        polygonz = 0;
    }

    close_shapefile();

Test;
