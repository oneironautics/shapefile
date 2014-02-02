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

/*
int32_t byteswap32(int32_t value)

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

/*
void print_msg(const char* format, ...)

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
const char* shape_type_to_name(const int32_t shape_type)

Converts a numeric shape type to its corresponding string representation.

Arguments:
    const int32_t shape_type: the shape to be named.

Returns:
    A const char* corresponding to the name of the shape type.
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

/*
const char* get_shapefile_path(void)

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

/*
const SFShapes* get_shape_records(void)

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

/*
SFResult open_shapefile(const char* path)

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

/*
SFResult close_shapefile(void)

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

/*
void allocate_shapes(void)

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

    for ( x = 0; x < num_records; ++x ) {
        g_shapes->records[x] = (SFShapeRecord*)malloc(sizeof(SFShapeRecord));

        if ( g_shapes->records[x] == 0 ) {
            print_msg("Could not allocate memory for g_shapes->records[%d]!", x);
            return SF_ERROR;
        }
    }
    
    g_shapes->num_records = num_records;

    return SF_OK;
}

/*
SFResult read_shapes(void)

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

/*
const void* get_shape_record(const uint32_t index)

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

/*
SFNull* get_null_shape(const SFShapeRecord* record)

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
    fread(&null->shape_type, sizeof(SFNull), 1, g_shapefile);

#ifdef DEBUG
    print_msg("Data length: %d, shape type: %s\n", record->record_size, shape_type_to_name(record->record_type));
#endif

    return null;
}

/*
SFPoint* get_point_shape(const SFShapeRecord* record)

Retrieves a Point shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPoint* data to return.

Returns:
    A SFPoint*, or 0 if the record type did not match or an out of memory condition was encountered.
*/
SFPoint* get_point_shape(const SFShapeRecord* record)
{
    SFPoint* point;

    if ( record->record_type != stPoint ) {
        return 0;
    }

    point = (SFPoint*)malloc(sizeof(SFPoint));

    if ( point == 0 ) {
        return 0;
    }

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    fread(&point, sizeof(SFPoint), 1, g_shapefile);

#ifdef DEBUG
    print_msg("Data length: %d, shape type: %s\n", record->record_size, shape_type_to_name(record->record_type));
    print_msg("\tPoint values:\n");
    print_msg("\t\tx => %lf\n", point->x);
    print_msg("\t\ty => %lf\n", point->y);
#endif

    return point;
}

/*
const SFMultiPoint* get_multipoint_shape(const SFShapeRecord* record)

Retrieves a MultiPoint shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFMultiPoint* data to return.

Returns:
    A const SFMultiPoint*, or 0 if the record type did not match or an out of memory condition was encountered.
*/
SFMultiPoint* get_multipoint_shape(const SFShapeRecord* record)
{
    int32_t x = 0;
    SFMultiPoint* multipoint;

    if ( record->record_type != stMultiPoint ) {
        return 0;
    }

    multipoint = (SFMultiPoint*)malloc(sizeof(SFMultiPoint));

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    fread(multipoint->box, sizeof(multipoint->box), 1, g_shapefile);
    fread(&multipoint->num_points, sizeof(int32_t), 1, g_shapefile);

    multipoint->points = (SFPoint*)malloc(sizeof(SFPoint) * multipoint->num_points);
    fread(multipoint->points, sizeof(SFPoint) * multipoint->num_points, 1, g_shapefile);

#ifdef DEBUG
    print_msg("Data length: %d, shape type: %s\n", record->record_size, shape_type_to_name(record->record_type));
    print_msg("\tBox values:\n");

    for ( x = 0; x < 4; ++x ) {
        print_msg("\t\tBox [%d] => %lf\n", x, multipoint->box[x]);
    }

    print_msg("\tPoints: %d\n", multipoint->num_points);

    for ( x = 0; x < multipoint->num_points; ++x ) {
        print_msg("\t\tPoint [%d] => x: %lf, y: %lf\n", x, multipoint->points[x].x, multipoint->points[x].y);
    }
#endif

    return multipoint;
}

/*
SFPolyLine* get_polyline_shape(const SFShapeRecord* record)

Retrieves a PolyLine shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPolyLine* data to return.

Returns:
    A SFPolyLine*, or 0 if the record type did not match or an out of memory condition was encountered.
*/
SFPolyLine* get_polyline_shape(const SFShapeRecord* record)
{
    int32_t x = 0;
    SFPolyLine* polyline;

    if ( record->record_type != stPolyline ) {
        return 0;
    }

    polyline = (SFPolyLine*)malloc(sizeof(SFPolyLine));

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    fread(polyline->box, sizeof(polyline->box), 1, g_shapefile);
    fread(&polyline->num_parts, sizeof(int32_t), 1, g_shapefile);
    fread(&polyline->num_points, sizeof(int32_t), 1, g_shapefile);

    polyline->parts = (int32_t*)malloc(sizeof(int32_t) * polyline->num_parts);
    polyline->points = (SFPoint*)malloc(sizeof(SFPoint) * polyline->num_points);

    for ( x = 0; x < polyline->num_parts; ++x ) {
        fread(&polyline->parts[x], sizeof(int32_t), 1, g_shapefile);
    }

    for ( x = 0; x < polyline->num_points; ++x ) {
        fread(&polyline->points[x], sizeof(SFPoint), 1, g_shapefile);
    }

#ifdef DEBUG
    print_msg("Data length: %d, shape type: %s\n", record->record_size, shape_type_to_name(record->record_type));
    print_msg("\tBox values:\n");

    for ( x = 0; x < 4; ++x ) {
        print_msg("\t\tBox [%d] => %lf\n", x, polyline->box[x]);
    }

    print_msg("\tParts: %d\n", polyline->num_parts);

    for ( x = 0; x < polyline->num_parts; ++x ) {
        print_msg("\t\tPart [%d] => %d\n", x, polyline->parts[x]);
    }

    print_msg("\tPoints: %d\n", polyline->num_points);

    for ( x = 0; x < polyline->num_points; ++x ) {
        print_msg("\t\tPoint [%d] => x: %lf, y: %lf\n", x, polyline->points[x].x, polyline->points[x].y);
    }
#endif

    return polyline;
}

/*
SFPolygon* get_polygon_shape(const SFShapeRecord* record)

Retrieves a Polygon shape from the specified record. The caller is responsible for freeing 


Arguments:
    const SFShapeRecord* record: the record that contains SFPolygon* data to return.

Returns:
    A SFPolygon*, or 0 if the record type did not match or an out of memory condition was encountered.
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
    print_msg("Data length: %d, shape type: %s\n", record->record_size, shape_type_to_name(record->record_type));
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
        print_msg("\t\tPoint [%d] => x: %lf, y: %lf\n", x, polygon->points[x].x, polygon->points[x].y);
    }
#endif

    return polygon;
}

/*
SFPointM* get_pointm_shape(const SFShapeRecord* record)

Retrieves a PointM shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPointM* data to return.

Returns:
    A SFPointM*, or 0 if the record type did not match or an out of memory condition was encountered.
*/
SFPointM* get_pointm_shape(const SFShapeRecord* record)
{
    SFPointM* pointm;

    if ( record->record_type != stPointM ) {
        return 0;
    }

    pointm = (SFPointM*)malloc(sizeof(SFPointM));

    if ( pointm == 0 ) {
        return 0;
    }

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    fread(&pointm, sizeof(SFPointM), 1, g_shapefile);

#ifdef DEBUG
    print_msg("Data length: %d, shape type: %s\n", record->record_size, shape_type_to_name(record->record_type));
    print_msg("\tPointM values:\n");
    print_msg("\t\tx => %lf\n", pointm->x);
    print_msg("\t\ty => %lf\n", pointm->y);
#endif

    return pointm;
}

/*
SFMultiPointM* get_multipointm_shape(const SFShapeRecord* record)

Retrieves a MultiPointM shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFMultiPointM* data to return.

Returns:
    A SFMultiPointM*, or 0 if the record type did not match or an out of memory condition was encountered.
*/
SFMultiPointM* get_multipointm_shape(const SFShapeRecord* record)
{
    int32_t x = 0;
    SFMultiPointM* multipointm;

    if ( record->record_type != stMultiPointM ) {
        return 0;
    }

    multipointm = (SFMultiPointM*)malloc(sizeof(SFMultiPointM));

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    fread(multipointm->box, sizeof(multipointm->box), 1, g_shapefile);
    fread(&multipointm->num_points, sizeof(int32_t), 1, g_shapefile);

    multipointm->points = (SFPoint*)malloc(sizeof(SFPoint) * multipointm->num_points);
    fread(multipointm->points, sizeof(SFPoint) * multipointm->num_points, 1, g_shapefile);

    fread(multipointm->m_range, sizeof(multipointm->m_range), 1, g_shapefile);

    for ( x = 0; x < multipointm->num_points; ++x ) {
        fread(&multipointm->m_array[x], sizeof(double), 1, g_shapefile);
    }

#ifdef DEBUG
    print_msg("Data length: %d, shape type: %s\n", record->record_size, shape_type_to_name(record->record_type));
    print_msg("\tBox values:\n");

    for ( x = 0; x < 4; ++x ) {
        print_msg("\t\tBox [%d] => %lf\n", x, multipointm->box[x]);
    }

    print_msg("\tPoints: %d\n", multipointm->num_points);

    for ( x = 0; x < multipointm->num_points; ++x ) {
        print_msg("\t\tPoint [%d] => x: %lf, y: %lf\n", x, multipointm->points[x].x, multipointm->points[x].y);
    }

    print_msg("\tM range: %lf - %lf\n", multipointm->m_range[0], multipointm->m_range[1]);

    for ( x = 0; x < multipointm->num_points; ++x ) {
        print_msg("\t\tM value for point [%d] => %lf\n", x, multipointm->m_array[x]);
    }
#endif

    return multipointm;
}

/*
SFPolyLineM* get_polylinem_shape(const SFShapeRecord* record)

Retrieves a PolyLineM shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPolyLineM* data to return.

Returns:
    A SFPolyLineM*, or 0 if the record type did not match or an out of memory condition was encountered.
*/
SFPolyLineM* get_polylinem_shape(const SFShapeRecord* record)
{
    int32_t x = 0;
    SFPolyLineM* polylinem;

    if ( record->record_type != stPolyLineM ) {
        return 0;
    }

    polylinem = (SFPolyLineM*)malloc(sizeof(SFPolyLineM));

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    fread(polylinem->box, sizeof(polylinem->box), 1, g_shapefile);
    fread(&polylinem->num_parts, sizeof(int32_t), 1, g_shapefile);
    fread(&polylinem->num_points, sizeof(int32_t), 1, g_shapefile);

    polylinem->parts = (int32_t*)malloc(sizeof(int32_t) * polylinem->num_parts);
    polylinem->points = (SFPoint*)malloc(sizeof(SFPoint) * polylinem->num_points);

    for ( x = 0; x < polylinem->num_parts; ++x ) {
        fread(&polylinem->parts[x], sizeof(int32_t), 1, g_shapefile);
    }

    for ( x = 0; x < polylinem->num_points; ++x ) {
        fread(&polylinem->points[x], sizeof(SFPoint), 1, g_shapefile);
    }

    fread(polylinem->m_range, sizeof(polylinem->m_range), 1, g_shapefile);

    for ( x = 0; x < polylinem->num_points; ++x ) {
        fread(&polylinem->m_array[x], sizeof(double), 1, g_shapefile);
    }

#ifdef DEBUG
    print_msg("Data length: %d, shape type: %s\n", record->record_size, shape_type_to_name(record->record_type));
    print_msg("\tBox values:\n");

    for ( x = 0; x < 4; ++x ) {
        print_msg("\t\tBox [%d] => %lf\n", x, polylinem->box[x]);
    }

    print_msg("\tParts: %d\n", polylinem->num_parts);

    for ( x = 0; x < polylinem->num_parts; ++x ) {
        print_msg("\t\tPart [%d] => %d\n", x, polylinem->parts[x]);
    }

    print_msg("\tPoints: %d\n", polylinem->num_points);

    for ( x = 0; x < polylinem->num_points; ++x ) {
        print_msg("\t\tPoint [%d] => x: %lf, y: %lf\n", x, polylinem->points[x].x, polylinem->points[x].y);
    }

    print_msg("\tM range: %lf - %lf\n", polylinem->m_range[0], polylinem->m_range[1]);

    for ( x = 0; x < polylinem->num_points; ++x ) {
        print_msg("\t\tM value for point [%d] => %lf\n", x, polylinem->m_array[x]);
    }
#endif

    return polylinem;
}

/*
SFPolygonM* get_polygonm_shape(const SFShapeRecord* record)

Retrieves a PolygonM shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPolygonM* data to return.

Returns:
    A SFPolygonM*, or 0 if the record type did not match or an out of memory condition was encountered.
*/
SFPolygonM* get_polygonm_shape(const SFShapeRecord* record)
{
    int32_t x = 0;
    SFPolygonM* polygonm;

    if ( record->record_type != stPolygonM ) {
        return 0;
    }

    polygonm = (SFPolygonM*)malloc(sizeof(SFPolygonM));

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    fread(polygonm->box, sizeof(polygonm->box), 1, g_shapefile);
    fread(&polygonm->num_parts, sizeof(int32_t), 1, g_shapefile);
    fread(&polygonm->num_points, sizeof(int32_t), 1, g_shapefile);

    polygonm->parts = (int32_t*)malloc(sizeof(int32_t) * polygonm->num_parts);
    polygonm->points = (SFPoint*)malloc(sizeof(SFPoint) * polygonm->num_points);

    for ( x = 0; x < polygonm->num_parts; ++x ) {
        fread(&polygonm->parts[x], sizeof(int32_t), 1, g_shapefile);
    }

    for ( x = 0; x < polygonm->num_points; ++x ) {
        fread(&polygonm->points[x], sizeof(SFPoint), 1, g_shapefile);
    }

    fread(polygonm->m_range, sizeof(polygonm->m_range), 1, g_shapefile);

    for ( x = 0; x < polygonm->num_points; ++x ) {
        fread(&polygonm->m_array[x], sizeof(double), 1, g_shapefile);
    }

#ifdef DEBUG
    print_msg("Data length: %d, shape type: %s\n", record->record_size, shape_type_to_name(record->record_type));
    print_msg("\tBox values:\n");

    for ( x = 0; x < 4; ++x ) {
        print_msg("\t\tBox [%d] => %lf\n", x, polygonm->box[x]);
    }

    print_msg("\tParts: %d\n", polygonm->num_parts);

    for ( x = 0; x < polygonm->num_parts; ++x ) {
        print_msg("\t\tPart [%d] => %d\n", x, polygonm->parts[x]);
    }

    print_msg("\tPoints: %d\n", polygonm->num_points);

    for ( x = 0; x < polygonm->num_points; ++x ) {
        print_msg("\t\tPoint [%d] => x: %lf, y: %lf\n", x, polygonm->points[x].x, polygonm->points[x].y);
    }

    print_msg("\tM range: %lf - %lf\n", polygonm->m_range[0], polygonm->m_range[1]);

    for ( x = 0; x < polygonm->num_points; ++x ) {
        print_msg("\t\tM value for point [%d] => %lf\n", x, polygonm->m_array[x]);
    }
#endif

    return polygonm;
}

/*
const SFPointZ* get_pointz_shape(const SFShapeRecord* record)

Retrieves a PointZ shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPointZ* data to return.

Returns:
    A const SFPointZ*, or 0 if the record type did not match or an out of memory condition was encountered.
*/
SFPointZ* get_pointz_shape(const SFShapeRecord* record)
{
    SFPointZ* pointz;

    if ( record->record_type != stPointZ ) {
        return 0;
    }

    pointz = (SFPointZ*)malloc(sizeof(SFPointZ));

    if ( pointz == 0 ) {
        return 0;
    }

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    fread(&pointz, sizeof(SFPointZ), 1, g_shapefile);

#ifdef DEBUG
    print_msg("Data length: %d, shape type: %s\n", record->record_size, shape_type_to_name(record->record_type));
    print_msg("\t\tx => %lf\n", pointz->x);
    print_msg("\t\ty => %lf\n", pointz->y);
#endif

    return pointz;
}

/*
SFMultiPointZ* get_multipointz_shape(const SFShapeRecord* record)

Retrieves a MultiPointZ shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFMultiPointZ* data to return.

Returns:
    A SFMultiPointZ*, or 0 if the record type did not match or an out of memory condition was encountered.
*/
SFMultiPointZ* get_multipointz_shape(const SFShapeRecord* record)
{
    int32_t x = 0;
    SFMultiPointZ* multipointz;

    if ( record->record_type != stMultiPointZ ) {
        return 0;
    }

    multipointz = (SFMultiPointZ*)malloc(sizeof(SFMultiPointZ));

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    fread(multipointz->box, sizeof(multipointz->box), 1, g_shapefile);
    fread(&multipointz->num_points, sizeof(int32_t), 1, g_shapefile);

    multipointz->points = (SFPoint*)malloc(sizeof(SFPoint) * multipointz->num_points);
    fread(multipointz->points, sizeof(SFPoint) * multipointz->num_points, 1, g_shapefile);

    fread(multipointz->m_range, sizeof(multipointz->m_range), 1, g_shapefile);

    for ( x = 0; x < multipointz->num_points; ++x ) {
        fread(&multipointz->m_array[x], sizeof(double), 1, g_shapefile);
    }

    fread(multipointz->m_range, sizeof(multipointz->m_range), 1, g_shapefile);

    for ( x = 0; x < multipointz->num_points; ++x ) {
        fread(&multipointz->m_array[x], sizeof(double), 1, g_shapefile);
    }

#ifdef DEBUG
    print_msg("Data length: %d, shape type: %s\n", record->record_size, shape_type_to_name(record->record_type));
    print_msg("\tBox values:\n");

    for ( x = 0; x < 4; ++x ) {
        print_msg("\t\tBox [%d] => %lf\n", x, multipointz->box[x]);
    }

    print_msg("\tPoints: %d\n", multipointz->num_points);

    for ( x = 0; x < multipointz->num_points; ++x ) {
        print_msg("\t\tPoint [%d] => x: %lf, y: %lf\n", x, multipointz->points[x].x, multipointz->points[x].y);
    }

    print_msg("\tZ range: %lf - %lf\n", multipointz->z_range[0], multipointz->z_range[1]);

    for ( x = 0; x < multipointz->num_points; ++x ) {
        print_msg("\t\tZ value for point [%d] => %lf\n", x, multipointz->z_array[x]);
    }

    print_msg("\tM range: %lf - %lf\n", multipointz->m_range[0], multipointz->m_range[1]);

    for ( x = 0; x < multipointz->num_points; ++x ) {
        print_msg("\t\tM value for point [%d] => %lf\n", x, multipointz->m_array[x]);
    }
#endif

    return multipointz;
}

/*
const SFPolyLineZ* get_polylinez_shape(const SFShapeRecord* record)

Retrieves a PolyLineZ shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPolyLineZ* data to return.

Returns:
    A const SFPolyLineZ*, or 0 if the record type did not match or an out of memory condition was encountered.
*/
SFPolyLineZ* get_polylinez_shape(const SFShapeRecord* record)
{
    int32_t x = 0;
    SFPolyLineZ* polylinez;

    if ( record->record_type != stPolygonZ ) {
        return 0;
    }

    polylinez = (SFPolyLineZ*)malloc(sizeof(SFPolyLineZ));

    fseek(g_shapefile, record->record_offset, SEEK_SET);
    fread(polylinez->box, sizeof(polylinez->box), 1, g_shapefile);
    fread(&polylinez->num_parts, sizeof(int32_t), 1, g_shapefile);
    fread(&polylinez->num_points, sizeof(int32_t), 1, g_shapefile);

    polylinez->parts = (int32_t*)malloc(sizeof(int32_t) * polylinez->num_parts);
    polylinez->points = (SFPoint*)malloc(sizeof(SFPoint) * polylinez->num_points);
    polylinez->z_array = (double*)malloc(sizeof(double) * polylinez->num_points);
    polylinez->m_array = (double*)malloc(sizeof(double) * polylinez->num_points);

    for ( x = 0; x < polylinez->num_parts; ++x ) {
        fread(&polylinez->parts[x], sizeof(int32_t), 1, g_shapefile);
    }

    for ( x = 0; x < polylinez->num_points; ++x ) {
        fread(&polylinez->points[x], sizeof(SFPoint), 1, g_shapefile);
    }

    fread(polylinez->z_range, sizeof(polylinez->z_range), 1, g_shapefile);

    for ( x = 0; x < polylinez->num_points; ++x ) {
        fread(&polylinez->z_array[x], sizeof(double), 1, g_shapefile);
    }

    fread(polylinez->m_range, sizeof(polylinez->m_range), 1, g_shapefile);

    for ( x = 0; x < polylinez->num_points; ++x ) {
        fread(&polylinez->m_array[x], sizeof(double), 1, g_shapefile);
    }

#ifdef DEBUG
    print_msg("Data length: %d, shape type: %s\n", record->record_size, shape_type_to_name(record->record_type));
    print_msg("\tBox values:\n");

    for ( x = 0; x < 4; ++x ) {
        print_msg("\t\tBox [%d] => %lf\n", x, polylinez->box[x]);
    }

    print_msg("\tParts: %d\n", polylinez->num_parts);

    for ( x = 0; x < polylinez->num_parts; ++x ) {
        print_msg("\t\tPart [%d] => %d\n", x, polylinez->parts[x]);
    }

    print_msg("\tPoints: %d\n", polylinez->num_points);

    for ( x = 0; x < polylinez->num_points; ++x ) {
        print_msg("\t\tPoint [%d] => x: %lf, y: %lf\n", x, polylinez->points[x].x, polylinez->points[x].y);
    }

    print_msg("\tZ range: %lf - %lf\n", polylinez->z_range[0], polylinez->z_range[1]);

    for ( x = 0; x < polylinez->num_points; ++x ) {
        print_msg("\t\tZ value for point [%d] => %lf\n", x, polylinez->z_array[x]);
    }

    print_msg("\tM range: %lf - %lf\n", polylinez->m_range[0], polylinez->m_range[1]);

    for ( x = 0; x < polylinez->num_points; ++x ) {
        print_msg("\t\tM value for point [%d] => %lf\n", x, polylinez->m_array[x]);
    }
#endif

    return polylinez;
}

/*
SFPolygonZ* get_polygonz_shape(const SFShapeRecord* record)

Retrieves a PolygonZ shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFPolygonZ* data to return.

Returns:
    A SFPolygonZ*, or 0 if the record type did not match or an out of memory condition was encountered.
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
    print_msg("Data length: %d, shape type: %s\n", record->record_size, shape_type_to_name(record->record_type));
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
        print_msg("\t\tPoint [%d] => x: %lf, y: %lf\n", x, polygonz->points[x].x, polygonz->points[x].y);
    }

    print_msg("\tZ range: %lf - %lf\n", polygonz->z_range[0], polygonz->z_range[1]);

    for ( x = 0; x < polygonz->num_points; ++x ) {
        print_msg("\t\tZ value for point [%d] => %lf\n", x, polygonz->z_array[x]);
    }

    print_msg("\tM range: %lf - %lf\n", polygonz->m_range[0], polygonz->m_range[1]);

    for ( x = 0; x < polygonz->num_points; ++x ) {
        print_msg("\t\tM value for point [%d] => %lf\n", x, polygonz->m_array[x]);
    }
#endif

    return polygonz;
}

/*
SFMultiPatch* get_multipatch_shape(const SFShapeRecord* record)

Retrieves a MultiPatch shape from the specified record.

Arguments:
    const SFShapeRecord* record: the record that contains SFMultiPatch* data to return.

Returns:
    A SFMultiPatch*, or 0 if the record type did not match or an out of memory condition was encountered.
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

/*
void free_null_shape(SFNull* null)

Frees a SFNull* returned by get_null_shape.

Arguments:
    SFNull* null: a SFNull* returned by get_null_shape.

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

/*
void free_point_shape(SFPoint* point)

Frees a SFPoint* returned by get_point_shape.

Arguments:
    SFPoint* point: a SFPoint* returned by get_point_shape.

Returns:
    N/A.
*/
void free_point_shape(SFPoint* point)
{
    if ( point != 0 ) {
        free(point);
        point = 0;
    }
}

/*
void free_polyline_shape(SFPolyLine* polyline)

Frees a SFPolyLine* returned by get_polyline_shape.

Arguments:
    SFPolyLine* polyline: a SFPolyLine* returned by get_polyline_shape.

Returns:
    N/A.
*/
void free_polyline_shape(SFPolyLine* polyline)
{
    if ( polyline != 0 ) {
        free(polyline->parts);
        polyline->parts = 0;
        free(polyline->points);
        polyline->points = 0;
        free(polyline);
        polyline = 0;
    }
}

/*
void free_polygon_shape(SFPolygon* polygon)

Frees a SFPolygon* returned by get_polygon_shape.

Arguments:
    SFPolygon* polygon: a SFPolygon* returned by get_polygon_shape.

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

/*
void free_multipoint_shape(SFMultiPoint* multipoint)

Frees a SFMultiPoint* returned by get_multipoint_shape.

Arguments:
    SFMultiPoint* multipoint: a SFMultiPoint* returned by get_multipoint_shape.

Returns:
    N/A.
*/
void free_multipoint_shape(SFMultiPoint* multipoint)
{
    if ( multipoint != 0 ) {
        free(multipoint->points);
        multipoint->points = 0;
        free(multipoint);
        multipoint = 0;
    }
}

/*
void free_pointz_shape(SFPointZ* pointz)

Frees a SFPointZ* returned by get_pointz_shape.

Arguments:
    SFPointZ* pointz: a SFPointZ* returned by get_pointz_shape.

Returns:
    N/A.
*/
void free_pointz_shape(SFPointZ* pointz)
{
    if ( pointz != 0 ) {
        free(pointz);
        pointz = 0;
    }
}

/*
void free_polylinez_shape(SFPolyLineZ* polylinez)

Frees a SFPolyLineZ* returned by get_polylinez_shape.

Arguments:
    SFPolyLineZ* polylinez: a SFPolyLineZ* returned by get_polylinez_shape.

Returns:
    N/A.
*/
void free_polylinez_shape(SFPolyLineZ* polylinez)
{
    if ( polylinez != 0 ) {
        free(polylinez);
        polylinez = 0;
    }
}

/*
void free_polygonz_shape(SFPolygonZ* polygonz)

Frees a SFPolygonZ* returned by get_polygonz_shape.

Arguments:
    SFPolygonZ* polygon: a SFPolygonZ* returned by get_polygonz_shape.

Returns:
    N/A.
*/
void free_polygonz_shape(SFPolygonZ* polygonz)
{
    if ( polygonz != 0 ) {
        free(polygonz->parts);
        polygonz->parts = 0;
        free(polygonz->points);
        polygonz->points = 0;
        free(polygonz->z_array);
        polygonz->z_array = 0;
        free(polygonz->m_array);
        polygonz->m_array = 0;
        free(polygonz);
        polygonz = 0;
    }
}

/*
void free_multipointz_shape(SFMultiPointZ* multipointz)

Frees a SFMultiPointZ* returned by get_multipointz_shape.

Arguments:
    SFMultiPointZ* multipointz: a SFMultiPointZ* returned by get_multipointz_shape.

Returns:
    N/A.
*/
void free_multipointz_shape(SFMultiPointZ* multipointz)
{
    if ( multipointz != 0 ) {
        free(multipointz);
        multipointz = 0;
    }
}

/*
void free_pointm_shape(SFPointM* pointm)

Frees a SFPointM* returned by get_pointm_shape.

Arguments:
    SFPointM* pointm: a SFPointM* returned by get_pointm_shape.

Returns:
    N/A.
*/
void free_pointm_shape(SFPointM* pointm)
{
    if ( pointm != 0 ) {
        free(pointm);
        pointm = 0;
    }
}

/*
void free_polylinem_shape(SFPolyLineM* polylinem)

Frees a SFPolyLineM* returned by get_polylinem_shape.

Arguments:
    SFPolyLineM* polylinem: a SFPolyLineM* returned by get_polylinem_shape.

Returns:
    N/A.
*/
void free_polylinem_shape(SFPolyLineM* polylinem)
{
    if ( polylinem != 0 ) {
        free(polylinem);
        polylinem = 0;
    }
}

/*
void free_polygonm_shape(SFPolygonM* polygonm)

Frees a SFPolygonM* returned by get_polygonm_shape.

Arguments:
    SFPolygonM* polygonm: a SFPolygonM* returned by get_polygonm_shape.

Returns:
    N/A.
*/
void free_polygonm_shape(SFPolygonM* polygonm)
{
    if ( polygonm != 0 ) {
        free(polygonm);
        polygonm = 0;
    }
}

/*
void free_multipointm_shape(SFMultiPointM* multipointm)

Frees a SFMultiPointM* returned by get_multipointm_shape.

Arguments:
    SFMultiPointM* multipointm: a SFMultiPointM* returned by get_multipointm_shape.

Returns:
    N/A.
*/
void free_multipointm_shape(SFMultiPointM* multipointm)
{
    if ( multipointm != 0 ) {
        free(multipointm);
        multipointm = 0;
    }
}

/*
void free_multipatch_shape(SFMultiPatch* multipatch)

Frees a SFMultiPatch* returned by get_multipatch_shape.

Arguments:
    SFMultiPatch* multipatch: a SFMultiPatch* returned by get_multipatch_shape.

Returns:
    N/A.
*/
void free_multipatch_shape(SFMultiPatch* multipatch)
{
    if ( multipatch != 0 ) {
        free(multipatch);
        multipatch = 0;
    }
}