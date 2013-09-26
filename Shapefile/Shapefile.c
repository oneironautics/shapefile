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
    sprintf_s(buf, 1024, format, args);
#else
    sprintf(buf, format, args);
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
    
    if ( ferror(g_shapefile) ) {
        print_msg("Could not read shape file <%s>.\n", g_path);
        return SF_ERROR;
    }
    
    if ( byteswap32(header.file_code) != SHAPEFILE_FILE_CODE || header.version != SHAPEFILE_VERSION ) {
        print_msg("File <%s> is not a shape file.\n", g_path);
        return SF_ERROR;
    }
    
    return SF_OK;
}

/*    void allocate_shapes(void)

Allocates memory for all shapes to be read from the shapefile.

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
        fseek(g_shapefile, header.content_length * 2 - sizeof(int32_t), SEEK_CUR);

        content_length += header.content_length * 2 - sizeof(int32_t);
        num_records++;
    }

    /*  Seek back to the end of the main file header. */
    fseek(g_shapefile, sizeof(SFFileHeader), SEEK_SET);

    /*  Allocate, allocate, and allocate again. */
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

    for ( ; x < num_records + 1; ++x ) {
        g_shapes->records[x] = (SFShapeRecord*)malloc(sizeof(SFShapeRecord) * (num_records + 1));

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
        header.content_length = byteswap32(header.content_length);
        header.record_number = byteswap32(header.record_number);
        fread(&shape_type, sizeof(int32_t), 1, g_shapefile);

#ifdef DEBUG
        print_msg("Record number: %d, size %d, shape type %d.\n", byteswap32(header.record_number), byteswap32(header.content_length), shape_type);
#endif
        /*  Dump the data into the respective record index in g_shapes->records. */
        g_shapes->records[index]->record_size = header.content_length * 2 - sizeof(int32_t);
        g_shapes->records[index]->record_type = shape_type;
        g_shapes->records[index]->data = malloc(header.content_length * 2 - sizeof(int32_t));
        fread(g_shapes->records[index]->data, header.content_length * 2 - sizeof(int32_t), 1, g_shapefile);
        index++;
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
    int32_t x = 0;

    /*  Close the shapefile. */
    if ( g_shapefile != 0 ) {
        fclose(g_shapefile);
        g_shapefile = 0;
    }

    /*  Free all memory. */
    if ( g_shapes != 0 ) {
        for ( ; x < g_shapes->num_records; ++x ) {
            SFShapeRecord* shape_record = g_shapes->records[x];
            free(shape_record->data);
            free(g_shapes->records[x]);
        }

        free(g_shapes->records);
        free(g_shapes);
        g_shapes = 0;
    }
    
    return SF_OK;
}
