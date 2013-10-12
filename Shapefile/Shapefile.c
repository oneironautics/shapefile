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

const char* g_path;
FILE* g_shapefile;
SFShapes* g_shapes;

/*    int32_t byteswap32(int32_t value)

A cross-platform helper for flipping endianess.

Arguments:
    value: the un-byteswapped value.

Returns:
    int32_t: the byteswapped value.

*/
int32_t byteswap32(int32_t value)
{
#ifdef _WIN32
    value = _byteswap_ulong(value);
#else
    value = __builtin_bswap32(value);
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
    
#ifdef _WIN32
    vsprintf_s(buf, 1024, format, args);
#else
    vsprintf(buf, format, args);
#endif

    printf(buf);
    fflush(stdout);

    va_end(args);
}

/*

*/

const char* shape_type_to_name(const int32_t shape_type)
{
    switch ( shape_type ) {
        case stNull:
            return "Null";
        case stPoint:
            return "Point";
        case stPolyline:
            return "Polyline";
        case stPolygon:
            return "Polygon";
        case stMultiPoint:
            return "MultiPoint";
        case stPointZ:
            return "PointZ";
        case stPolyLineZ:
            return "PolyLineZ";
        case stPolygonZ:
            return "PolygonZ";
        case stMultiPointZ:
            return "MultiPointZ";
        case stPointM:
            return "PointM";
        case stPolyLineM:
            return "PolyLineM";
        case stPolygonM:
            return "PolygonM";
        case stMultiPointM:
            return "MultiPointM";
        case stMultiPatch:
            return "MultiPatch";
        default:
            return "Unknown";
    }
}

/*    const char* get_shapefile_path(void)

Returns the path to the shapefile.

Arguments:
    N/A.
    
Returns:
    A const char* representing the path to the shapefile.
*/
const char* get_shapefile_path(void)
{
    return g_path;
}

/*    const SFShapes* get_shape_records(void)

Returns all shape records.

Arguments:
    N/A.
    
Returns:
    A const SFShapes*.
*/
const SFShapes* get_shape_records(void)
{
    return g_shapes;
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
    SFFileHeader header;
    g_path = path;

#ifdef _WIN32
    fopen_s(&g_shapefile, g_path, "rb");
#else
    g_shapefile = fopen(g_path, "rb");
#endif
    
    if ( g_shapefile == 0 ) {
        print_msg("Could not open shape file <%s>.", g_path);
        return SF_ERROR;
    }

    fread(&header, sizeof(SFFileHeader), 1, g_shapefile);
    
    if ( byteswap32(header.file_code) != SHAPEFILE_FILE_CODE || header.version != SHAPEFILE_VERSION ) {
        print_msg("File <%s> is not a shape file.\n", g_path);
        return SF_ERROR;
    }
    
    return SF_OK;
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
    uint32_t x = 0;

    /*  Close the shapefile. */
    if ( g_shapefile != 0 ) {
        fclose(g_shapefile);
        g_shapefile = 0;
    }

    /*  Free all memory. */
    if ( g_shapes != 0 ) {
        for ( ; x < g_shapes->num_records; ++x ) {
            free(g_shapes->records[x]);
        }

        free(g_shapes->records);
        free(g_shapes);
        g_shapes = 0;
    }
    
    return SF_OK;
}

/*    void allocate_shapes(void)

Allocates memory for shapefile records read from the shapefile.

Arguments:
    N/A.
    
Returns:
    SF_OK: no errors occurred.
    SF_ERROR: out of memory, or something equally catastrophic.
*/
SFResult allocate_shapes(void)
{
    int32_t x = 0;
    int32_t num_records = 0;
    int32_t content_length = 0;

    /*  Read each shape record header and tally up the content length/number of records. */
    while ( !feof(g_shapefile) ) {
        SFShapeRecordHeader header;
        int32_t shape_type = 0;

        fread(&header, sizeof(SFShapeRecordHeader), 1, g_shapefile);

        if ( feof(g_shapefile) ) {
            break;
        }

        header.content_length = byteswap32(header.content_length);
        header.record_number = byteswap32(header.record_number);
        fread(&shape_type, sizeof(int32_t), 1, g_shapefile);
        /*  Note: content_length is the number of 16 bit numbers, not a byte count. Multiply by sizeof(int16_t). */
        fseek(g_shapefile, header.content_length * sizeof(int16_t) - sizeof(int32_t), SEEK_CUR);
        content_length += header.content_length * sizeof(int16_t) - sizeof(int32_t);
        num_records++;
    }

    /*  Seek back to the end of the main file header. */
    fseek(g_shapefile, sizeof(SFFileHeader), SEEK_SET);
    
    /*  Allocate enough memory for the record index. */
    g_shapes = (SFShapes*)malloc(sizeof(SFShapes));

    if ( g_shapes == 0 ) {
        print_msg("Could not allocate memory for g_shapes!");
        return SF_ERROR;
    }

    g_shapes->records = (SFShapeRecord**)malloc(sizeof(SFShapeRecord) * (num_records + 1));

    if ( g_shapes->records == 0 ) {
        print_msg("Could not allocate memory for g_shapes->records!");
        return SF_ERROR;
    }

    for ( x = 0; x < num_records + 1; ++x ) {
        g_shapes->records[x] = (SFShapeRecord*)malloc(sizeof(SFShapeRecord));

        if ( g_shapes->records[x] == 0 ) {
            print_msg("Could not allocate memory for g_shapes->records[%d]!", x);
            return SF_ERROR;
        }
    }
    
    g_shapes->num_records = num_records + 1;

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
    int32_t index = 0;

    if ( allocate_shapes() == SF_ERROR ) {
        return SF_ERROR;
    }

    while ( !feof(g_shapefile) ) {
        /*  Read a file record header. */
        SFShapeRecordHeader header;
        int32_t shape_type = 0;

        fread(&header, sizeof(SFShapeRecordHeader), 1, g_shapefile);

        if ( feof(g_shapefile) ) {
            break;
        }

        header.content_length = byteswap32(header.content_length);
        header.record_number = byteswap32(header.record_number);
        fread(&shape_type, sizeof(int32_t), 1, g_shapefile);

#ifdef DEBUG
        print_msg("Record %d, length %d (%d bytes), %s.\n", header.record_number, header.content_length, header.content_length * sizeof(int16_t), shape_type_to_name(shape_type));
#endif
        /*  Note: content_length is the number of 16 bit numbers, not a byte count. Multiply by sizeof(int16_t). */
        g_shapes->records[index]->record_size = header.content_length * sizeof(int16_t) - sizeof(int32_t);
        g_shapes->records[index]->record_type = shape_type;
        g_shapes->records[index]->record_offset = ftell(g_shapefile);
        fseek(g_shapefile, header.content_length * sizeof(int16_t) - sizeof(int32_t), SEEK_CUR);
        index++;
    }
    
    return SF_OK;
}

/*    const void* get_shape_record(const uint32_t index)

Retrieves an SFShapeRecord* at the specified index.

Arguments:
    const uint32_t index: the index of the shape record to retrieve.

Returns:
    A const SFShapeRecord*.
*/

const SFShapeRecord* get_shape_record(const uint32_t index)
{
    if ( index >= g_shapes->num_records ) {
        return 0;
    }

    return g_shapes->records[index];
}

/*    SFNull* get_null_shape(const SFShapeRecord* record)

Retrieves a Null shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFNull* data to return.

Returns:
    A SFNull*, or 0 if the record type did not match.
*/

SFNull* get_null_shape(const SFShapeRecord* record)
{
    SFNull* null;

    if ( record->record_type != stNull ) {
        return 0;
    }

    null = (SFNull*)malloc(sizeof(SFNull));

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    fread(&null->shape_type, sizeof(null->shape_type), 1, g_shapefile);

    return null;
}

/*    SFPoint* get_point_shape(const SFShapeRecord* record)

Retrieves a Point shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPoint* data to return.

Returns:
    A SFPoint*, or 0 if the record type did not match.
*/

SFPoint* get_point_shape(const SFShapeRecord* record)
{
    SFPoint* point;

    if ( record->record_type != stPoint ) {
        return 0;
    }

    point = (SFPoint*)malloc(sizeof(SFPoint));

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    fread(&point, sizeof(SFPoint), 1, g_shapefile);

    return point;
}

/*    SFPolyLine* get_polyline_shape(const SFShapeRecord* record)

Retrieves a PolyLine shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPolyLine* data to return.

Returns:
    A SFPolyLine*, or 0 if the record type did not match.
*/

SFPolyLine* get_polyline_shape(const SFShapeRecord* record)
{
    SFPolyLine* polyline;

    if ( record->record_type != stPolyline ) {
        return 0;
    }

    polyline = (SFPolyLine*)malloc(sizeof(SFPolyLine));

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    
    return polyline;
}

/*    SFPolygon* get_polygon_shape(const SFShapeRecord* record)

Retrieves a Polygon shape from the specified record. The caller is responsible for freeing 


Arguments:
    const SFShapeRecord* record: the record that contains SFPolygon* data to return.

Returns:
    A SFPolygon*, or 0 if the record type did not match.
*/

SFPolygon* get_polygon_shape(const SFShapeRecord* record)
{
    int32_t x = 0;
    SFPolygon* polygon;
   
    if ( record->record_type != stPolygon ) {
        return 0;
    }

    polygon = (SFPolygon*)malloc(sizeof(SFPolygon));

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    fread(polygon->box, sizeof(polygon->box), 1, g_shapefile);
    fread(&polygon->num_parts, sizeof(int32_t), 1, g_shapefile);
    fread(&polygon->num_points, sizeof(int32_t), 1, g_shapefile);

    polygon->parts = (int32_t*)malloc(sizeof(int32_t) * polygon->num_parts);
    polygon->points = (SFPoint*)malloc(sizeof(SFPoint) * polygon->num_points);

    for ( x = 0; x < polygon->num_parts; ++x ) {
        fread(&polygon->parts[x], sizeof(int32_t), 1, g_shapefile);
    }

    for ( x = 0; x < polygon->num_points; ++x ) {
        fread(&polygon->points[x], sizeof(SFPoint), 1, g_shapefile);
    }

#ifdef DEBUG
    print_msg("Data length: %d, shape type: %d\n", record->record_size, record->record_type);
    print_msg("Polygon found:\n");
    print_msg("\tBox values:\n");

    for ( x = 0; x < 4; ++x ) {
        print_msg("\t\tBox [%d] => %lf\n", x, polygon->box[x]);
    }

    print_msg("\tParts: %d\n", polygon->num_parts);

    for ( x = 0; x < polygon->num_parts; ++x ) {
        print_msg("\t\tPart [%d] => %d\n", x, polygon->parts[x]);
    }

    print_msg("\tPoints: %d\n", polygon->num_points);

    for ( x = 0; x < polygon->num_points; ++x ) {
        SFPoint point = polygon->points[x];
        print_msg("\t\tPoint [%d] => x: %lf, y: %lf\n", x, point.x, point.y);
    }
#endif

    return polygon;
}

/*    const SFMultiPoint* get_multipoint_shape(const SFShapeRecord* record)

Retrieves a MultiPoint shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFMultiPoint* data to return.

Returns:
    A const SFMultiPoint*, or 0 if the record type did not match.
*/

SFMultiPoint* get_multipoint_shape(const SFShapeRecord* record)
{
    SFMultiPoint* multipoint;

    if ( record->record_type != stMultiPoint ) {
        return 0;
    }

    multipoint = (SFMultiPoint*)malloc(sizeof(SFMultiPoint));

    fseek(g_shapefile, record->record_offset, SEEK_SET);

    return multipoint;
}

/*    const SFPointZ* get_pointz_shape(const SFShapeRecord* record)

Retrieves a PointZ shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPointZ* data to return.

Returns:
    A const SFPointZ*, or 0 if the record type did not match.
*/

SFPointZ* get_pointz_shape(const SFShapeRecord* record)
{
    SFPointZ* pointz;

    if ( record->record_type != stPointZ ) {
        return 0;
    }

    pointz = (SFPointZ*)malloc(sizeof(SFPointZ));

    fseek(g_shapefile, record->record_offset, SEEK_SET);

    return pointz;
}

/*    const SFPolyLineZ* get_polylinez_shape(const SFShapeRecord* record)

Retrieves a PolyLineZ shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPolyLineZ* data to return.

Returns:
    A const SFPolyLineZ*, or 0 if the record type did not match.
*/

SFPolyLineZ* get_polylinez_shape(const SFShapeRecord* record)
{
    SFPolyLineZ* polylinez;

    if ( record->record_type != stPolyLineZ ) {
        return 0;
    }

    polylinez = (SFPolyLineZ*)malloc(sizeof(SFPolyLineZ));

    fseek(g_shapefile, record->record_offset, SEEK_SET);

    return polylinez;
}

/*    SFPolygonZ* get_polygonz_shape(const SFShapeRecord* record)

Retrieves a PolygonZ shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPolygonZ* data to return.

Returns:
    A SFPolygonZ*, or 0 if the record type did not match.
*/

SFPolygonZ* get_polygonz_shape(const SFShapeRecord* record)
{
    int32_t x = 0;
    SFPolygonZ* polygonz;

    if ( record->record_type != stPolygonZ ) {
        return 0;
    }

    polygonz = (SFPolygonZ*)malloc(sizeof(SFPolygonZ));

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    fread(polygonz->box, sizeof(polygonz->box), 1, g_shapefile);
    fread(&polygonz->num_parts, sizeof(int32_t), 1, g_shapefile);
    fread(&polygonz->num_points, sizeof(int32_t), 1, g_shapefile);

    polygonz->parts = (int32_t*)malloc(sizeof(int32_t) * polygonz->num_parts);
    polygonz->points = (SFPoint*)malloc(sizeof(SFPoint) * polygonz->num_points);
    polygonz->z_array = (double*)malloc(sizeof(double) * polygonz->num_points);
    polygonz->m_array = (double*)malloc(sizeof(double) * polygonz->num_points);

    for ( x = 0; x < polygonz->num_parts; ++x ) {
        fread(&polygonz->parts[x], sizeof(int32_t), 1, g_shapefile);
    }

    for ( x = 0; x < polygonz->num_points; ++x ) {
        fread(&polygonz->points[x], sizeof(SFPoint), 1, g_shapefile);
    }

    fread(polygonz->z_range, sizeof(polygonz->z_range), 1, g_shapefile);

    for ( x = 0; x < polygonz->num_points; ++x ) {
        fread(&polygonz->z_array[x], sizeof(double), 1, g_shapefile);
    }

    fread(polygonz->m_range, sizeof(polygonz->m_range), 1, g_shapefile);

    for ( x = 0; x < polygonz->num_points; ++x ) {
        fread(&polygonz->m_array[x], sizeof(double), 1, g_shapefile);
    }

#ifdef DEBUG
    print_msg("Data length: %d, shape type: %d\n", record->record_size, record->record_type);
    print_msg("PolygonZ found:\n");
    print_msg("\tBox values:\n");

    for ( x = 0; x < 4; ++x ) {
        print_msg("\t\tBox [%d] => %lf\n", x, polygonz->box[x]);
    }

    print_msg("\tParts: %d\n", polygonz->num_parts);

    for ( x = 0; x < polygonz->num_parts; ++x ) {
        print_msg("\t\tPart [%d] => %d\n", x, polygonz->parts[x]);
    }

    print_msg("\tPoints: %d\n", polygonz->num_points);

    for ( x = 0; x < polygonz->num_points; ++x ) {
        SFPoint point = polygonz->points[x];
        print_msg("\t\tPoint [%d] => x: %lf, y: %lf\n", x, point.x, point.y);
    }

    print_msg("\tZ range: %lf - %lf\n", polygonz->z_range[0], polygonz->z_range[1]);

    for ( x = 0; x < polygonz->num_points; ++x ) {
        double z = polygonz->z_array[x];
        print_msg("\t\tZ value for point [%d] => %lf\n", x, z);
    }

    print_msg("\tM range: %lf - %lf\n", polygonz->m_range[0], polygonz->m_range[1]);

    for ( x = 0; x < polygonz->num_points; ++x ) {
        double m = polygonz->m_array[x];
        print_msg("\t\tM value for point [%d] => %lf\n", x, m);
    }
#endif

    return polygonz;
}

/*    SFMultiPointZ* get_multipointz_shape(const SFShapeRecord* record)

Retrieves a MultiPointZ shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFMultiPointZ* data to return.

Returns:
    A SFMultiPointZ*, or 0 if the record type did not match.
*/

SFMultiPointZ* get_multipointz_shape(const SFShapeRecord* record)
{
    SFMultiPointZ* multipointz;

    if ( record->record_type != stMultiPointZ ) {
        return 0;
    }

    multipointz = (SFMultiPointZ*)malloc(sizeof(SFMultiPointZ));

    fseek(g_shapefile, record->record_offset, SEEK_SET);

    return multipointz;
}

/*    SFPointM* get_pointm_shape(const SFShapeRecord* record)

Retrieves a PointM shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPointM* data to return.

Returns:
    A SFPointM*, or 0 if the record type did not match.
*/

SFPointM* get_pointm_shape(const SFShapeRecord* record)
{
    SFPointM* pointm;

    if ( record->record_type != stPointM ) {
        return 0;
    }

    pointm = (SFPointM*)malloc(sizeof(SFPointM));

    fseek(g_shapefile, record->record_offset, SEEK_SET);

    return pointm;
}

/*    SFPolyLineM* get_polylinem_shape(const SFShapeRecord* record)

Retrieves a PolyLineM shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPolyLineM* data to return.

Returns:
    A SFPolyLineM*, or 0 if the record type did not match.
*/

SFPolyLineM* get_polylinem_shape(const SFShapeRecord* record)
{
    SFPolyLineM* polylinem;

    if ( record->record_type != stPolyLineM ) {
        return 0;
    }

    polylinem = (SFPolyLineM*)malloc(sizeof(SFPolyLineM));

    fseek(g_shapefile, record->record_offset, SEEK_SET);

    return polylinem;
}

/*    SFPolygonM* get_polygonm_shape(const SFShapeRecord* record)

Retrieves a PolygonM shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPolygonM* data to return.

Returns:
    A SFPolygonM*, or 0 if the record type did not match.
*/

SFPolygonM* get_polygonm_shape(const SFShapeRecord* record)
{
    SFPolygonM* polygonm;

    if ( record->record_type != stPolygonM ) {
        return 0;
    }

    polygonm = (SFPolygonM*)malloc(sizeof(SFPolygonM));

    fseek(g_shapefile, record->record_offset, SEEK_SET);

    return polygonm;
}

/*    SFMultiPointM* get_multipointm_shape(const SFShapeRecord* record)

Retrieves a MultiPointM shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFMultiPointM* data to return.

Returns:
    A SFMultiPointM*, or 0 if the record type did not match.
*/

SFMultiPointM* get_multipointm_shape(const SFShapeRecord* record)
{
    SFMultiPointM* multipointm;

    if ( record->record_type != stMultiPointM ) {
        return 0;
    }

    multipointm = (SFMultiPointM*)malloc(sizeof(SFMultiPointM));

    fseek(g_shapefile, record->record_offset, SEEK_SET);

    return multipointm;
}

/*    SFMultiPatch* get_multipatch_shape(const SFShapeRecord* record)

Retrieves a MultiPatch shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFMultiPatch* data to return.

Returns:
    A SFMultiPatch*, or 0 if the record type did not match.
*/

SFMultiPatch* get_multipatch_shape(const SFShapeRecord* record)
{
    SFMultiPatch* multipatch;

    if ( record->record_type != stMultiPatch ) {
        return 0;
    }

    multipatch = (SFMultiPatch*)malloc(sizeof(SFMultiPatch));

    fseek(g_shapefile, record->record_offset, SEEK_SET);

    return multipatch;
}

/*    void free_null_shape(SFNull* null)

Frees an SFNull* returned by get_null_shape.

Arguments:
    SFNull* null: an SFNull* returned by get_null_shape.

Returns:
    N/A.
*/

void free_null_shape(SFNull* null)
{
    if ( null != 0 ) {
        free(null);
        null = 0;
    }
}

/*    void free_polygon_shape(SFPolygon* polygon)

Frees an SFPolygon* returned by get_polygon_shape.

Arguments:
    SFPolygon* polygon: an SFPolygon* returned by get_polygon_shape.

Returns:
    N/A.
*/

void free_polygon_shape(SFPolygon* polygon)
{
    if ( polygon != 0 ) {
        free(polygon->parts);
        polygon->parts = 0;
        free(polygon->points);
        polygon->points = 0;
        free(polygon);
        polygon = 0;
    }
}