//
//  ShapefileTests.m
//  ShapefileTests
//
//  Created by Michael Smith on 3/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "ShapefileTests.h"
#import "Definitions.h"

@implementation ShapefileTests

static NSString* kShapefile = @"/Users/smith/Downloads/TM_WORLD_BORDERS_SIMPL-0.3/TM_WORLD_BORDERS_SIMPL-0.3.shp";

- (void)setUp
{
    [super setUp];
}

- (void)tearDown
{
    // Tear-down code here.
    
    [super tearDown];
}

- (void)testOpenShapefile
{
    SFError error = open_shapefile([kShapefile UTF8String]);
    
    if ( error == SF_ERROR ) {
        STFail(@"Could not open shapefile.");
    }
}

- (void)testValidShapefile
{
    SFError error = validate_shapefile();
    
    if ( error == SF_ERROR ) {
        STFail(@"File is not a valid ERSI shape file.");
    }
}

- (void)testReadShapes
{
    SFError error = read_shapes();
    
    if ( error == SF_ERROR ) {
        STFail(@"Could not read shapes in shape file.");
    }
}

- (void)readNullShape
{
    STFail(@"Could not read Null shape in file.");
}

- (void)readPointShape
{
    STFail(@"Could not read Point shape in file.");
}

- (void)readPolyLineShape
{
    STFail(@"Could not read PolyLine shape in file.");
}

- (void)readPolygonShape
{
    STFail(@"Could not read Polygon shape in file.");
}

- (void)printShapesInFile
{
    STFail(@"Could not print shapes in file.");
}

- (void)plotShapesInFile
{
    STFail(@"Could not plot shapes in file.");
}

@end
