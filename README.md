shapefile
=========

This library intends to be a cross-platform static library for reading ESRI shapefiles.

Usage example:

```c
    /* Opens the file and ensures it's a valid ESRI shapefile. */
    FILE* pShapefile = open_shapefile(path);

	if ( pShapefile == 0) {
		return 1;
	}

    /* Reads the shape records and returns them as an SFShape*. */
    SFShapes* pShapes = read_shapes(pShapefile);

    /* We can now iterate over each shape record. */
	for ( uint32_t x = 0; x < pShapes->num_records; ++x ) {
	    /* SFShapeRecord describes the shape type, offset, and size in the file. */
		const SFShapeRecord* record = pShapes->records[x];
		/* Read the desired record into an SFPolyLine*. The caller is responsible for freeing this. */
		SFPolyLine* polyline = get_polyline_shape(pShapefile, record);
	
		/* Do stuff with the polyline. */
		render_polyline(polyline);

        /* Done with the polyline; it is our responsibility to free it. */
		free_polyline_shape(polyline);
		polyline = NULL;
	}

    /* We're done with our shape records; ensure that they're freed. */
    free_shapes(pShapes);
    /* We're done with the shapefile; close it. */
	close_shapefile(pShapefile);
```
