#import "Mapbox.h"

#import "MBXViewController.h"

#import "MBXAppDelegate.h"
#import "MBXCustomCalloutView.h"
#import "MBXOfflinePacksTableViewController.h"
#import "MBXAnnotationView.h"
#import "MBXUserLocationAnnotationView.h"
#import "MBXEmbeddedMapViewController.h"
#import "MBXOrnamentsViewController.h"
#import "MBXStateManager.h"
#import "MBXState.h"
#import "MLNSettings.h"

#import "MLNMapView_Private.h"

#import "CustomStyleLayerExample.h"

#import "ExampleCustomDrawableStyleLayer.h"

#import "MBXFrameTimeGraphView.h"
#import "MLNMapView_Experimental.h"
#import <objc/runtime.h>

// Plug In Examples
#import "PluginLayerExample.h"
#import "PluginLayerExampleMetalRendering.h"
#import "MLNPluginStyleLayer.h"

static const CLLocationCoordinate2D WorldTourDestinations[] = {
    { .latitude = 38.8999418, .longitude = -77.033996 },
    { .latitude = 37.7884307, .longitude = -122.3998631 },
    { .latitude = 52.5003103, .longitude = 13.4197763 },
    { .latitude = 60.1712627, .longitude = 24.9378866 },
    { .latitude = 53.8948782, .longitude = 27.5558476 },
};

static const MLNCoordinateBounds colorado = {
    .sw = { .latitude = 36.986207, .longitude = -109.049896},
    .ne = { .latitude = 40.989329, .longitude = -102.062592},
};

static NSString * const MBXViewControllerAnnotationViewReuseIdentifer = @"MBXViewControllerAnnotationViewReuseIdentifer";

typedef NS_ENUM(NSInteger, MBXSettingsSections) {
    MBXSettingsDebugTools = 0,
    MBXSettingsAnnotations,
    MBXSettingsRuntimeStyling,
    MBXSettingsMiscellaneous,
};

typedef NS_ENUM(NSInteger, MBXSettingsDebugToolsRows) {
    MBXSettingsDebugToolsResetPosition = 0,
    MBXSettingsDebugToolsTileBoundaries,
    MBXSettingsDebugToolsTileInfo,
    MBXSettingsDebugToolsTimestamps,
    MBXSettingsDebugToolsCollisionBoxes,
    MBXSettingsDebugToolsOverdrawVisualization,
    MBXSettingsDebugToolsShowZoomLevel,
    MBXSettingsDebugToolsShowFrameTimeGraph,
    MBXSettingsDebugToolsShowReuseQueueStats,
    MBXSettingsDebugToolsShowRenderingStats
};

typedef NS_ENUM(NSInteger, MBXSettingsAnnotationsRows) {
    MBXSettingsAnnotations100Views = 0,
    MBXSettingsAnnotations1000Views,
    MBXSettingsAnnotations10000Views,
    MBXSettingsAnnotations100Sprites,
    MBXSettingsAnnotations1000Sprites,
    MBXSettingsAnnotations10000Sprites,
    MBXSettingsAnnotationAnimation,
    MBXSettingsAnnotationsTestShapes,
    MBXSettingsAnnotationsManyTestShapes,
    MBXSettingsAnnotationsCustomCallout,
    MBXSettingsAnnotationsQueryAnnotations,
    MBXSettingsAnnotationsCustomUserDot,
    MBXSettingsAnnotationsRemoveAnnotations,
    MBXSettingsAnnotationSelectRandomOffscreenPointAnnotation,
    MBXSettingsAnnotationCenterSelectedAnnotation,
    MBXSettingsAnnotationAddVisibleAreaPolyline
};

typedef NS_ENUM(NSInteger, MBXSettingsRuntimeStylingRows) {
    MBXSettingsRuntimeStylingBuildingExtrusions = 0,
    MBXSettingsRuntimeStylingWater,
    MBXSettingsRuntimeStylingRoads,
    MBXSettingsRuntimeStylingRaster,
    MBXSettingsRuntimeStylingShape,
    MBXSettingsRuntimeStylingSymbols,
    MBXSettingsRuntimeStylingBuildings,
    MBXSettingsRuntimeStylingFerry,
    MBXSettingsRuntimeStylingParks,
    MBXSettingsRuntimeStylingFilteredFill,
    MBXSettingsRuntimeStylingFilteredLines,
    MBXSettingsRuntimeStylingNumericFilteredFill,
    MBXSettingsRuntimeStylingStyleQuery,
    MBXSettingsRuntimeStylingFeatureSource,
    MBXSettingsRuntimeStylingPointCollection,
    MBXSettingsRuntimeStylingUpdateShapeSourceData,
    MBXSettingsRuntimeStylingUpdateShapeSourceURL,
    MBXSettingsRuntimeStylingUpdateShapeSourceFeatures,
    MBXSettingsRuntimeStylingVectorTileSource,
    MBXSettingsRuntimeStylingRasterTileSource,
    MBXSettingsRuntimeStylingImageSource,
    MBXSettingsRuntimeStylingRouteLine,
    MBXSettingsRuntimeStylingAddCustomTriangleLayer,
    MBXSettingsRuntimeStylingDDSPolygon,
    MBXSettingsRuntimeStylingCustomLatLonGrid,
    MBXSettingsRuntimeStylingLineGradient,
    MBXSettingsRuntimeStylingCustomDrawableLayer,
    MBXSettingsRuntimeStylingAddFoursquarePOIsPMTiles
};

typedef NS_ENUM(NSInteger, MBXSettingsMiscellaneousRows) {
    MBXSettingsMiscellaneousWorldTour,
    MBXSettingsMiscellaneousRandomTour,
    MBXSettingsMiscellaneousScrollView,
    MBXSettingsMiscellaneousToggleTwoMaps,
    MBXSettingsMiscellaneousLocalizeLabels,
    MBXSettingsMiscellaneousShowSnapshots,
    MBXSettingsMiscellaneousMissingIcon,
    MBXSettingsMiscellaneousShouldLimitCameraChanges,
    MBXSettingsMiscellaneousSetContentInsets,
    MBXSettingsMiscellaneousShowCustomLocationManager,
    MBXSettingsMiscellaneousOrnamentsPlacement,
    MBXSettingsMiscellaneousLatLngBoundsWithPadding,
    MBXSettingsMiscellaneousCycleTileLOD,
    MBXSettingsMiscellaneousPrintLogFile,
    MBXSettingsMiscellaneousDeleteLogFile
};

typedef NS_ENUM(NSInteger, MBXTileLodMode) {
    MBXTileLodModeNoLod = 0,
    MBXTileLodModeDefault,
    MBXTileLodModeReduced,
    MBXTileLodModeAggressive,
    MBXTileLodModeCount
};

// Utility methods
CLLocationCoordinate2D coordinateCentered(CLLocationCoordinate2D origin, CLLocationDegrees bearing, CLLocationDistance distance) {

    // Convert to radians
    double multiplier      = M_PI / 180.0;
    double sourceLatitude  = multiplier * origin.latitude;
    double sourceLongitude = multiplier * origin.longitude;
    bearing *= multiplier;
    distance /= 6378137.0;

    // Pulled from MLNRadianCoordinateAtDistanceFacingDirection:
    double latitude = asin((sin(sourceLatitude) * cos(distance)) +
                           (cos(sourceLatitude) * sin(distance) * cos(bearing)));

    double longitude = sourceLongitude + atan2((sin(bearing) * sin(distance) * cos(sourceLatitude)),
                                               cos(distance) - (sin(sourceLatitude) * sin(latitude)));

    CLLocationCoordinate2D result;
    result.latitude  = fmin(85.0, fmax(-85.0, (latitude / multiplier)));
    result.longitude = longitude / multiplier;
    return result;
}

CLLocationCoordinate2D randomWorldCoordinate(void) {

    static const struct {
        CLLocationCoordinate2D coordinate;
        CLLocationDistance radius;
    } landmasses[] = {
        // Rough land masses
        {{ 38.328531,   94.778736 },    4100000 },  // Asia
        {{ 1.477244,    18.138111 },    4100000 },  // Africa
        {{ 52.310059,   22.295425 },    2000000 },  // Europe
        {{ 42.344216,   -96.532700 },   3000000 },  // N America
        {{ -11.537273,  -57.035181 },   2220000 },  // S America
        {{ -20.997030,  134.660541 },   2220000 },  // Australia

        // A few cities
        {{ 51.504787,   -0.106977 },    33000 },    // London
        {{ 37.740186,   -122.437086 },  8500 },     // SF
        {{ 52.509978,   13.406510 },    12000 },    // Berlin
        {{ 12.966246,   77.586505 },    19000 }     // Bengaluru
    };

    NSInteger index                   = arc4random_uniform(sizeof(landmasses)/sizeof(landmasses[0]));
    CLLocationCoordinate2D coordinate = landmasses[index].coordinate;
    CLLocationDistance radius         = landmasses[index].radius;

    // Now create a world coord
    CLLocationDegrees heading          = (CLLocationDegrees)arc4random_uniform(360);
    CLLocationDistance distance        = (CLLocationDistance)arc4random_uniform(radius);
    CLLocationCoordinate2D newLocation = coordinateCentered(coordinate, heading, distance);
    return newLocation;
}





@interface MBXDroppedPinAnnotation : MLNPointAnnotation
@end

@implementation MBXDroppedPinAnnotation
@end

@interface MBXCustomCalloutAnnotation : MLNPointAnnotation
@property (nonatomic, assign) BOOL anchoredToAnnotation;
@property (nonatomic, assign) BOOL dismissesAutomatically;
@end

@implementation MBXCustomCalloutAnnotation
@end

@interface MBXSpriteBackedAnnotation : MLNPointAnnotation
@end

@implementation MBXSpriteBackedAnnotation
@end

@interface MBXViewController () <UITableViewDelegate,
                                 UITableViewDataSource,
                                 MLNMapViewDelegate,
                                 MLNComputedShapeSourceDataSource>


@property (nonatomic) IBOutlet MLNMapView *mapView;
@property (nonatomic) MBXState *currentState;
@property (weak, nonatomic) IBOutlet UIButton *hudLabel;
@property (weak, nonatomic) IBOutlet MBXFrameTimeGraphView *frameTimeGraphView;
@property (nonatomic) NSInteger styleIndex;
@property (nonatomic) NSMutableArray* styleNames;
@property (nonatomic) NSMutableArray* styleURLs;
@property (nonatomic) BOOL customUserLocationAnnnotationEnabled;
@property (nonatomic, getter=isLocalizingLabels) BOOL localizingLabels;
@property (nonatomic) BOOL reuseQueueStatsEnabled;
@property (nonatomic) BOOL frameTimeGraphEnabled;
@property (nonatomic) BOOL renderingStatsEnabled;
@property (nonatomic) BOOL shouldLimitCameraChanges;
@property (nonatomic) BOOL randomWalk;
@property (nonatomic) BOOL zoomLevelOrnamentEnabled;
@property (nonatomic) NSMutableArray<UIWindow *> *helperWindows;
@property (nonatomic) NSMutableArray<UIView *> *contentInsetsOverlays;
@property (nonatomic, copy) void (^locationBlock)(void);
@end

@interface MLNMapView (MBXViewController)
@property (nonatomic) NSDictionary *annotationViewReuseQueueByIdentifier;
@end

@implementation MBXViewController
{
    BOOL _isTouringWorld;
    BOOL _contentInsetsEnabled;
    UIEdgeInsets _originalContentInsets;

    NSLayoutConstraint* _firstMapLayout;
    NSArray<NSLayoutConstraint*>* _secondMapLayout;

    NSDictionary* _pointFeatures;
    NSLock* _loadLock;

    MBXTileLodMode _tileLodMode;
}

// MARK: - Setup & Teardown

- (instancetype)init {
    if (self = [super init]) {
        _firstMapLayout = nil;
        _secondMapLayout = nil;
        _pointFeatures = nil;
        _loadLock = [NSLock new];
    }
    return self;
}

// This will add the plug-in layers.  This is a demo of how
// extensible layers for the style can be added to the map view
-(void)addPluginLayers {

    [self.mapView addPluginLayerType:[PluginLayerExample class]];
    [self.mapView addPluginLayerType:[PluginLayerExampleMetalRendering class]];

}

- (void)viewDidLoad
{
    [super viewDidLoad];

    [self addPluginLayers];

    // Keep track of current map state and debug preferences,
    // saving and restoring when the application's state changes.
    self.currentState =  [MBXStateManager sharedManager].currentState;

    if (!self.currentState) {
        // Create a new state with the below default values
        self.currentState = [[MBXState alloc] init];

        self.mapView.showsUserHeadingIndicator = YES;
        self.mapView.showsScale = YES;
        self.zoomLevelOrnamentEnabled = NO;
        self.frameTimeGraphEnabled = NO;
    } else {
        // Revert to the previously saved state
        [self restoreMapState:nil];
    }

    [self updateHUD];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(saveCurrentMapState:) name:UIApplicationDidEnterBackgroundNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(restoreMapState:) name:UIApplicationWillEnterForegroundNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(saveCurrentMapState:) name:UIApplicationWillTerminateNotification object:nil];

    self.styleIndex = -1;
    [self setStyles];
    [self cycleStyles:self];

    self.mapView.experimental_enableFrameRateMeasurement = YES;
    self.hudLabel.titleLabel.font = [UIFont monospacedDigitSystemFontOfSize:10 weight:UIFontWeightRegular];

    // Add fall-through single tap gesture recognizer. This will be called when
    // the map view's tap recognizers fail.
    UITapGestureRecognizer *singleTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleSingleTap:)];
    for (UIGestureRecognizer *gesture in self.mapView.gestureRecognizers) {
        if ([gesture isKindOfClass:[UITapGestureRecognizer class]]) {
            [singleTap requireGestureRecognizerToFail:gesture];
        }
    }
    [self.mapView addGestureRecognizer:singleTap];

    // Display a secondary map on any connected external display.
    // https://developer.apple.com/documentation/uikit/windows_and_screens/displaying_content_on_a_connected_screen?language=objc
    self.helperWindows = [NSMutableArray array];
    [[NSNotificationCenter defaultCenter] addObserverForName:UIScreenDidConnectNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
        UIScreen *helperScreen = note.object;
        UIWindow *helperWindow = [[UIWindow alloc] initWithFrame:helperScreen.bounds];
        helperWindow.screen = helperScreen;
        UIViewController *helperViewController = [[UIViewController alloc] init];
        MLNMapView *helperMapView = [[MLNMapView alloc] initWithFrame:helperWindow.bounds styleURL:[[MLNStyle predefinedStyle:@"Hybrid"] url]];
        helperMapView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        helperMapView.camera = self.mapView.camera;
        helperMapView.compassView.hidden = YES;
        helperViewController.view = helperMapView;
        helperWindow.rootViewController = helperViewController;
        helperWindow.hidden = NO;
        [self.helperWindows addObject:helperWindow];
    }];
    [[NSNotificationCenter defaultCenter] addObserverForName:UIScreenDidDisconnectNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
        UIScreen *helperScreen = note.object;
        for (UIWindow *window in self.helperWindows) {
            if (window.screen == helperScreen) {
                [self.helperWindows removeObject:window];
            }
        }
    }];
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskAll;
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(__unused id)sender {
    if ([segue.identifier isEqualToString:@"ShowOfflinePacks"]) {
        MBXOfflinePacksTableViewController *controller = [segue destinationViewController];
        controller.mapView = self.mapView;
    }
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

// MARK: - Debugging Interface

- (IBAction)showSettings:(__unused id)sender
{
    self.randomWalk = NO;

    UITableViewController *settingsViewController = [[UITableViewController alloc] initWithStyle:UITableViewStyleGrouped];
    settingsViewController.tableView.delegate = self;
    settingsViewController.tableView.dataSource = self;
    settingsViewController.title = @"Debugging";
    settingsViewController.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(dismissSettings:)];
    UINavigationController *wrapper = [[UINavigationController alloc] initWithRootViewController:settingsViewController];
    wrapper.navigationBar.tintColor = self.navigationController.navigationBar.tintColor;
    [self.navigationController presentViewController:wrapper animated:YES completion:nil];
}

- (void)dismissSettings:(__unused id)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (NSArray <NSString *> *)settingsSectionTitles
{
    return @[
        @"Debug Tools",
        @"Annotations",
        @"Runtime Styling",
        @"Miscellaneous"
    ];
}

- (NSArray <NSString *> *)settingsTitlesForSection:(NSInteger)section
{
    NSMutableArray *settingsTitles = [NSMutableArray array];

    MLNMapDebugMaskOptions debugMask = self.mapView.debugMask;

    switch (section)
    {
        case MBXSettingsDebugTools:
            [settingsTitles addObjectsFromArray:@[
                @"Reset position",
                [NSString stringWithFormat:@"%@ tile boundaries",
                    (debugMask & MLNMapDebugTileBoundariesMask ? @"Hide" :@"Show")],
                [NSString stringWithFormat:@"%@ tile info",
                    (debugMask & MLNMapDebugTileInfoMask ? @"Hide" :@"Show")],
                [NSString stringWithFormat:@"%@ tile timestamps",
                    (debugMask & MLNMapDebugTimestampsMask ? @"Hide" :@"Show")],
                [NSString stringWithFormat:@"%@ collision boxes",
                    (debugMask & MLNMapDebugCollisionBoxesMask ? @"Hide" :@"Show")],
                [NSString stringWithFormat:@"%@ overdraw visualization",
                    (debugMask & MLNMapDebugOverdrawVisualizationMask ? @"Hide" :@"Show")],
                [NSString stringWithFormat:@"%@ zoom level ornament", (self.zoomLevelOrnamentEnabled ? @"Hide" :@"Show")],
                [NSString stringWithFormat:@"%@ frame time graph", (self.frameTimeGraphEnabled ? @"Hide" :@"Show")],
                [NSString stringWithFormat:@"%@ reuse queue stats", (self.reuseQueueStatsEnabled ? @"Hide" :@"Show")],
                [NSString stringWithFormat:@"%@ rendering stats", (self.renderingStatsEnabled ? @"Hide" :@"Show")]
            ]];
            break;
        case MBXSettingsAnnotations:
            [settingsTitles addObjectsFromArray:@[
                @"Add 100 Views",
                @"Add 1,000 Views",
                @"Add 10,000 Views",
                @"Add 100 Sprites",
                @"Add 1,000 Sprites",
                @"Add 10,000 Sprites",
                @"Animate an Annotation View",
                @"Add Test Shapes",
                @"Add 10x Test Shapes",
                @"Add Point With Custom Callout",
                @"Query Annotations",
                [NSString stringWithFormat:@"%@ Custom User Dot", (_customUserLocationAnnnotationEnabled ? @"Disable" : @"Enable")],
                @"Remove Annotations",
                @"Select an offscreen point annotation",
                @"Center selected annotation",
                @"Add visible area polyline"
            ]];
            break;
        case MBXSettingsRuntimeStyling:
            [settingsTitles addObjectsFromArray:@[
                @"Add Building Extrusions",
                @"Style Water With Function",
                @"Style Roads With Function",
                @"Add Raster & Apply Function",
                @"Add Shapes & Apply Fill",
                @"Style Symbol Color",
                @"Style Building Fill Color",
                @"Style Ferry Line Color",
                @"Remove Parks",
                @"Style Fill With Filter",
                @"Style Lines With Filter",
                @"Style Fill With Numeric Filter",
                @"Query and Style Features",
                @"Style Feature",
                @"Style Dynamic Point Collection",
                @"Update Shape Source: Data",
                @"Update Shape Source: URL",
                @"Update Shape Source: Features",
                @"Style Vector Tile Source",
                @"Style Raster Tile Source",
                @"Style Image Source",
                @"Add Route Line",
#if MLN_RENDER_BACKEND_METAL
                @"Add Custom Triangle Layer (Metal)",
#else
                @"Add Custom Triangle Layer (OpenGL)",
#endif
                @"Dynamically Style Polygon",
                @"Add Custom Lat/Lon Grid",
                @"Style Route line with gradient",
                @"Add Custom Drawable Layer",
                @"Add FourSquare POIs PMTiles Layer"
            ]];
            break;
        case MBXSettingsMiscellaneous:
            [settingsTitles addObjectsFromArray:@[
                @"Start World Tour",
                @"Random Tour",
                @"Embedded Map View",
                [NSString stringWithFormat:@"%@ Second Map", ([self.view viewWithTag:2] == nil ? @"Show" : @"Hide")],
                [NSString stringWithFormat:@"Show Labels in %@", (_localizingLabels ? @"Default Language" : [[NSLocale currentLocale] displayNameForKey:NSLocaleIdentifier value:[self bestLanguageForUser]])],
                @"Show Snapshots",
                @"Missing Icon",
                [NSString stringWithFormat:@"%@ Camera Changes", (_shouldLimitCameraChanges ? @"Unlimit" : @"Limit")],
                [NSString stringWithFormat:@"Turn %@ Content Insets", (_contentInsetsEnabled ? @"Off" : @"On")],
                @"View Route Simulation",
                @"Ornaments Placement",
                @"Lat Long bounds with padding",
                [NSString stringWithFormat:@"Cycle tile LOD mode: %@", [self getTileLodModeName]]
            ]];

            break;
        default:
            NSAssert(NO, @"All settings sections should be implemented");
            break;
    }

    return settingsTitles;
}

- (void)performActionForSettingAtIndexPath:(NSIndexPath *)indexPath
{
    switch (indexPath.section)
    {
        case MBXSettingsDebugTools:
            switch (indexPath.row)
            {
                case MBXSettingsDebugToolsResetPosition:
                    [self.mapView resetPosition];
                    break;
                case MBXSettingsDebugToolsTileBoundaries:
                    self.currentState.debugMask ^= MLNMapDebugTileBoundariesMask;
                    break;
                case MBXSettingsDebugToolsTileInfo:
                    self.currentState.debugMask ^= MLNMapDebugTileInfoMask;
                    break;
                case MBXSettingsDebugToolsTimestamps:
                    self.currentState.debugMask ^= MLNMapDebugTimestampsMask;
                    break;
                case MBXSettingsDebugToolsCollisionBoxes:
                    self.currentState.debugMask ^= MLNMapDebugCollisionBoxesMask;
                    break;
                case MBXSettingsDebugToolsOverdrawVisualization:
                    self.currentState.debugMask ^= MLNMapDebugOverdrawVisualizationMask;
                    break;
                case MBXSettingsDebugToolsShowZoomLevel:
                {
                    self.zoomLevelOrnamentEnabled = !self.zoomLevelOrnamentEnabled;
                    self.currentState.showsZoomLevelOrnament = self.zoomLevelOrnamentEnabled;
                    self.hudLabel.hidden = !self.zoomLevelOrnamentEnabled;
                    self.reuseQueueStatsEnabled = NO;
                    [self updateHUD];
                    break;
                }
                case MBXSettingsDebugToolsShowFrameTimeGraph:
                {
                    self.frameTimeGraphEnabled = !self.frameTimeGraphEnabled;
                    self.currentState.showsTimeFrameGraph = !self.currentState.showsTimeFrameGraph;
                    self.frameTimeGraphView.hidden = !self.frameTimeGraphEnabled;
                    [self updateHUD];
                    break;
                }
                case MBXSettingsDebugToolsShowReuseQueueStats:
                {
                    self.reuseQueueStatsEnabled = !self.currentState.reuseQueueStatsEnabled;
                    self.hudLabel.hidden = !self.currentState.reuseQueueStatsEnabled;
                    self.zoomLevelOrnamentEnabled = NO;
                    [self updateHUD];
                    break;
                }
                case MBXSettingsDebugToolsShowRenderingStats:
                {
                    self.renderingStatsEnabled = !self.renderingStatsEnabled;
                    [self.mapView enableRenderingStatsView:self.renderingStatsEnabled];
                    break;
                }
                default:
                    NSAssert(NO, @"All debug tools setting rows should be implemented");
                    break;
            }

            self.mapView.debugMask = self.currentState.debugMask;

            break;
        case MBXSettingsAnnotations:
            switch (indexPath.row)
            {
                case MBXSettingsAnnotations100Views:
                    [self parseFeaturesAddingCount:100 usingViews:YES];
                    break;
                case MBXSettingsAnnotations1000Views:
                    [self parseFeaturesAddingCount:1000 usingViews:YES];
                    break;
                case MBXSettingsAnnotations10000Views:
                    [self parseFeaturesAddingCount:10000 usingViews:YES];
                    break;
                case MBXSettingsAnnotations100Sprites:
                    [self parseFeaturesAddingCount:100 usingViews:NO];
                    break;
                case MBXSettingsAnnotations1000Sprites:
                    [self parseFeaturesAddingCount:1000 usingViews:NO];
                    break;
                case MBXSettingsAnnotations10000Sprites:
                    [self parseFeaturesAddingCount:10000 usingViews:NO];
                    break;
                case MBXSettingsAnnotationAnimation:
                    [self animateAnnotationView];
                    break;
                case MBXSettingsAnnotationsTestShapes:
                    [self addTestShapes:1];
                    break;
                case MBXSettingsAnnotationsManyTestShapes:
                    [self addTestShapes:10];
                    break;
                case MBXSettingsAnnotationsCustomCallout:
                    [self addAnnotationWithCustomCallout];
                    break;
                case MBXSettingsAnnotationsQueryAnnotations:
                    [self testQueryPointAnnotations];
                    break;
                case MBXSettingsAnnotationsCustomUserDot:
                    [self toggleCustomUserDot];
                    break;
                case MBXSettingsAnnotationsRemoveAnnotations:
                    [self.mapView removeAnnotations:self.mapView.annotations];
                    break;
                case MBXSettingsAnnotationSelectRandomOffscreenPointAnnotation:
                    [self selectAnOffscreenPointAnnotation];
                    break;
                case MBXSettingsAnnotationCenterSelectedAnnotation:
                    [self centerSelectedAnnotation];
                    break;
                case MBXSettingsAnnotationAddVisibleAreaPolyline:
                    [self addVisibleAreaPolyline];
                    break;
                default:
                    NSAssert(NO, @"All annotations setting rows should be implemented");
                    break;
            }
            break;
        case MBXSettingsRuntimeStyling:
            switch (indexPath.row)
            {
                case MBXSettingsRuntimeStylingBuildingExtrusions:
                    [self styleBuildingExtrusions];
                    break;
                case MBXSettingsRuntimeStylingWater:
                    [self styleWaterLayer];
                    break;
                case MBXSettingsRuntimeStylingRoads:
                    [self styleRoadLayer];
                    break;
                case MBXSettingsRuntimeStylingRaster:
                    [self styleRasterLayer];
                    break;
                case MBXSettingsRuntimeStylingShape:
                    [self styleShapeSource];
                    break;
                case MBXSettingsRuntimeStylingSymbols:
                    [self styleSymbolLayer];
                    break;
                case MBXSettingsRuntimeStylingBuildings:
                    [self styleBuildingLayer];
                    break;
                case MBXSettingsRuntimeStylingFerry:
                    [self styleFerryLayer];
                    break;
                case MBXSettingsRuntimeStylingParks:
                    [self removeParkLayer];
                    break;
                case MBXSettingsRuntimeStylingFilteredFill:
                    [self styleFilteredFill];
                    break;
                case MBXSettingsRuntimeStylingFilteredLines:
                    [self styleFilteredLines];
                    break;
                case MBXSettingsRuntimeStylingNumericFilteredFill:
                    [self styleNumericFilteredFills];
                    break;
                case MBXSettingsRuntimeStylingStyleQuery:
                    [self styleQuery];
                    break;
                case MBXSettingsRuntimeStylingFeatureSource:
                    [self styleFeature];
                    break;
                case MBXSettingsRuntimeStylingPointCollection:
                    [self styleDynamicPointCollection];
                    break;
                case MBXSettingsRuntimeStylingUpdateShapeSourceURL:
                    [self updateShapeSourceURL];
                    break;
                case MBXSettingsRuntimeStylingUpdateShapeSourceData:
                    [self updateShapeSourceData];
                    break;
                case MBXSettingsRuntimeStylingUpdateShapeSourceFeatures:
                    [self updateShapeSourceFeatures];
                    break;
                case MBXSettingsRuntimeStylingVectorTileSource:
                    [self styleVectorTileSource];
                    break;
                case MBXSettingsRuntimeStylingRasterTileSource:
                    [self styleRasterTileSource];
                    break;
                case MBXSettingsRuntimeStylingImageSource:
                    [self styleImageSource];
                    break;
                case MBXSettingsRuntimeStylingRouteLine:
                    [self styleRouteLine];
                    break;
                case MBXSettingsRuntimeStylingAddCustomTriangleLayer:
                    [self styleAddCustomTriangleLayer];
                    break;
                case MBXSettingsRuntimeStylingDDSPolygon:
                    [self stylePolygonWithDDS];
                    break;
                case MBXSettingsRuntimeStylingCustomLatLonGrid:
                    [self addLatLonGrid];
                    break;
                case MBXSettingsRuntimeStylingLineGradient:
                    [self styleLineGradient];
                    break;
                case MBXSettingsRuntimeStylingCustomDrawableLayer:
                    [self addCustomDrawableLayer];
                    break;
                case MBXSettingsRuntimeStylingAddFoursquarePOIsPMTiles:
                    [self addFoursquarePOIsPMTilesLayer];
                    break;
                default:
                    NSAssert(NO, @"All runtime styling setting rows should be implemented");
                    break;
            }
            break;
        case MBXSettingsMiscellaneous:
            switch (indexPath.row)
            {
                case MBXSettingsMiscellaneousLocalizeLabels:
                    [self toggleStyleLabelsLanguage];
                    break;
                case MBXSettingsMiscellaneousWorldTour:
                    [self startWorldTour];
                    break;
                case MBXSettingsMiscellaneousRandomTour:
                    [self randomWorldTour];
                    break;
                case MBXSettingsMiscellaneousScrollView:
                {
                    UIStoryboard *storyboard = [UIStoryboard storyboardWithName:@"Main" bundle:nil];
                    MBXEmbeddedMapViewController *embeddedMapViewController = (MBXEmbeddedMapViewController *)[storyboard instantiateViewControllerWithIdentifier:@"MBXEmbeddedMapViewController"];
                    [self.navigationController pushViewController:embeddedMapViewController animated:YES];
                    break;
                }
                case MBXSettingsMiscellaneousToggleTwoMaps:
                    [self toggleSecondMapView];
                    break;
                case MBXSettingsMiscellaneousShowSnapshots:
                {
                    [self performSegueWithIdentifier:@"ShowSnapshots" sender:nil];
                    break;
                }
                case MBXSettingsMiscellaneousMissingIcon:
                {
                    [self loadMissingIcon];
                    break;
                }
                case MBXSettingsMiscellaneousShowCustomLocationManager:
                {
                    [self performSegueWithIdentifier:@"ShowCustomLocationManger" sender:nil];
                    break;
                }
                case MBXSettingsMiscellaneousShouldLimitCameraChanges:
                {
                    self.shouldLimitCameraChanges = !self.shouldLimitCameraChanges;
                    if (self.shouldLimitCameraChanges) {
                        [self.mapView setCenterCoordinate:CLLocationCoordinate2DMake(39.748947, -104.995882) zoomLevel:10 direction:0 animated:NO];
                    }
                    break;
                }
                case MBXSettingsMiscellaneousSetContentInsets:
                {
                    if (!_contentInsetsEnabled) {
                        _originalContentInsets = [self.mapView contentInset];
                    }
                    _contentInsetsEnabled = !_contentInsetsEnabled;
                    _mapView.automaticallyAdjustsContentInset = !_contentInsetsEnabled;
                    UIEdgeInsets contentInsets = self.mapView.bounds.size.width > self.mapView.bounds.size.height
                        ? UIEdgeInsetsMake(_originalContentInsets.top, 0.5 * self.mapView.bounds.size.width, _originalContentInsets.bottom, 0.0)
                        : UIEdgeInsetsMake(0.25 * self.mapView.bounds.size.height, 0.0, _originalContentInsets.bottom, 0.25 * self.mapView.bounds.size.width);
                    if (_contentInsetsEnabled) {
                        if (!self.contentInsetsOverlays)
                            self.contentInsetsOverlays = [NSMutableArray array];
                        if (![self.contentInsetsOverlays count]) {
                            UIView *view = [[UIView alloc]initWithFrame:CGRectMake(0, 0, self.mapView.bounds.size.width, contentInsets.top)];
                            view.backgroundColor = [UIColor colorWithRed:0.0 green:0.3 blue:0.3 alpha:0.5];
                            [self.contentInsetsOverlays addObject:view];
                            [self.view addSubview:view];
                            view = [[UIView alloc]initWithFrame:CGRectMake(0, 0, contentInsets.left, self.mapView.bounds.size.height)];
                            view.backgroundColor = [UIColor colorWithRed:0.0 green:0.3 blue:0.3 alpha:0.5];
                            [self.contentInsetsOverlays addObject:view];
                            [self.view addSubview:view];
                            view = [[UIView alloc]initWithFrame:CGRectMake(self.mapView.bounds.size.width - contentInsets.right, 0, contentInsets.right, self.mapView.bounds.size.height)];
                            view.backgroundColor = [UIColor colorWithRed:0.0 green:0.3 blue:0.3 alpha:0.5];
                            [self.contentInsetsOverlays addObject:view];
                            [self.view addSubview:view];
                            view = [[UIView alloc]initWithFrame:CGRectMake(0, self.mapView.bounds.size.height - contentInsets.bottom, self.mapView.bounds.size.width, self.mapView.bounds.size.height)];
                            view.backgroundColor = [UIColor colorWithRed:0.0 green:0.3 blue:0.3 alpha:0.5];
                            [self.contentInsetsOverlays addObject:view];
                            [self.view addSubview:view];
                        }
                        [self.view bringSubviewToFront:self.contentInsetsOverlays[0]];
                        [self.view bringSubviewToFront:self.contentInsetsOverlays[1]];
                        [self.view bringSubviewToFront:self.contentInsetsOverlays[2]];
                        [self.view bringSubviewToFront:self.contentInsetsOverlays[3]];

                        // Denver streets parallel to cardinal directions help illustrate
                        // viewport center offset when edge insets are set.
                        MLNMapCamera *camera = [MLNMapCamera cameraLookingAtCenterCoordinate:CLLocationCoordinate2DMake(39.72707, -104.9986)
                                                                              acrossDistance:100
                                                                                       pitch:60
                                                                                     heading:0];
                        __weak MBXViewController *weakSelf = self;
                        [self.mapView setCamera:camera withDuration:0.3 animationTimingFunction:nil completionHandler:^{
                            [weakSelf.mapView setContentInset:contentInsets animated:YES completionHandler:nil];
                        }];
                    } else {
                        [self.view sendSubviewToBack:self.contentInsetsOverlays[0]];
                        [self.view sendSubviewToBack:self.contentInsetsOverlays[1]];
                        [self.view sendSubviewToBack:self.contentInsetsOverlays[2]];
                        [self.view sendSubviewToBack:self.contentInsetsOverlays[3]];
                        [self.mapView setContentInset:_originalContentInsets animated:YES completionHandler:nil];
                    }
                    break;
                }
                case MBXSettingsMiscellaneousOrnamentsPlacement:
                {
                    MBXOrnamentsViewController *ornamentsViewController = [[MBXOrnamentsViewController alloc] init];
                    [self.navigationController pushViewController:ornamentsViewController animated:YES];
                    break;
                }
                case MBXSettingsMiscellaneousLatLngBoundsWithPadding:
                    [self flyToWithLatLngBoundsAndPadding];
                    break;
                case MBXSettingsMiscellaneousCycleTileLOD:
                    [self cycleTileLodMode];
                    break;
                default:
                    NSAssert(NO, @"All miscellaneous setting rows should be implemented");
                    break;
            }
            break;
        default:
            NSAssert(NO, @"All settings sections should be implemented");
            break;
    }
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return [[self settingsSectionTitles] count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return [[self settingsTitlesForSection:section] count];
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
    return [[self settingsSectionTitles] objectAtIndex:section];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];

    cell.textLabel.text = [[self settingsTitlesForSection:indexPath.section] objectAtIndex:indexPath.row];

    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [tableView deselectRowAtIndexPath:indexPath animated:NO];

    [self dismissViewControllerAnimated:YES completion:^
    {
        [self performActionForSettingAtIndexPath:indexPath];
    }];
}

// MARK: - Debugging Actions

- (NSDictionary*)loadPointFeatures
{
    [_loadLock lock];
    if (!_pointFeatures) {
        NSData *featuresData = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"points" ofType:@"geojson"]];
        if (featuresData) {
            id features = [NSJSONSerialization JSONObjectWithData:featuresData
                                                          options:0
                                                            error:nil];
            if ([features isKindOfClass:[NSDictionary class]])
            {
                _pointFeatures = (NSDictionary*)features;
            }
        }
    }
    [_loadLock unlock];
    return _pointFeatures;
}

- (void)parseFeaturesAddingCount:(NSUInteger)featuresCount usingViews:(BOOL)useViews
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^
    {
        if (auto *features = [self loadPointFeatures])
        {
            NSMutableArray *annotations = [NSMutableArray array];
            for (NSDictionary *feature in features[@"features"])
            {
                CLLocationCoordinate2D coordinate = CLLocationCoordinate2DMake([feature[@"geometry"][@"coordinates"][1] doubleValue],
                                                                               [feature[@"geometry"][@"coordinates"][0] doubleValue]);
                NSString *title = feature[@"properties"][@"NAME"];

                MLNPointAnnotation *annotation = (useViews ? [MLNPointAnnotation new] : [MBXSpriteBackedAnnotation new]);

                annotation.coordinate = coordinate;
                annotation.title = title;

                [annotations addObject:annotation];

                if (annotations.count == featuresCount) break;
            }

            dispatch_async(dispatch_get_main_queue(), ^
            {
                [self.mapView removeAnnotations:self.mapView.annotations];
                [self.mapView addAnnotations:annotations];
                [self.mapView showAnnotations:annotations animated:YES];
            });
        }
    });
}

- (void)animateAnnotationView
    {
        MLNPointAnnotation *annot = [[MLNPointAnnotation alloc] init];
        annot.coordinate = self.mapView.centerCoordinate;
        [self.mapView addAnnotation:annot];

        // Move the annotation to a point that is offscreen.
        CGPoint point = CGPointMake(self.view.frame.origin.x - 200, CGRectGetMidY(self.view.frame));

        CLLocationCoordinate2D coord = [self.mapView convertPoint:point toCoordinateFromView:self.view];
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            [UIView animateWithDuration:10 animations:^{
                annot.coordinate = coord;
            }];
        });
    };

- (void)addTestShapes:(NSUInteger)featuresCount
{
    for (int featureIndex = 0; featureIndex < featuresCount; ++featureIndex) {
        double deltaLongitude = featureIndex * 0.01;
        double deltaLatitude = -featureIndex * 0.01;

        // Pacific Northwest triangle
        //
        CLLocationCoordinate2D triangleCoordinates[3] =
        {
            CLLocationCoordinate2DMake(44 + deltaLatitude, -122 + deltaLongitude),
            CLLocationCoordinate2DMake(46 + deltaLatitude, -122 + deltaLongitude),
            CLLocationCoordinate2DMake(46 + deltaLatitude, -121 + deltaLongitude)
        };

        MLNPolygon *triangle = [MLNPolygon polygonWithCoordinates:triangleCoordinates count:3];

        [self.mapView addAnnotation:triangle];

        // West coast polyline
        //
        CLLocationCoordinate2D lineCoordinates[4] = {
            CLLocationCoordinate2DMake(47.6025 + deltaLatitude, -122.3327 + deltaLongitude),
            CLLocationCoordinate2DMake(45.5189 + deltaLatitude, -122.6726 + deltaLongitude),
            CLLocationCoordinate2DMake(37.7790 + deltaLatitude, -122.4177 + deltaLongitude),
            CLLocationCoordinate2DMake(34.0532 + deltaLatitude, -118.2349 + deltaLongitude)
        };
        MLNPolyline *line = [MLNPolyline polylineWithCoordinates:lineCoordinates count:4];
        [self.mapView addAnnotation:line];

        // Orcas Island, WA hike polyline
        //
        NSDictionary *hike = [NSJSONSerialization JSONObjectWithData:
                              [NSData dataWithContentsOfFile:
                               [[NSBundle mainBundle] pathForResource:@"polyline" ofType:@"geojson"]]
                                                             options:0
                                                               error:nil];

        NSArray *hikeCoordinatePairs = hike[@"features"][0][@"geometry"][@"coordinates"];

        CLLocationCoordinate2D *polylineCoordinates = (CLLocationCoordinate2D *)malloc([hikeCoordinatePairs count] * sizeof(CLLocationCoordinate2D));

        for (NSUInteger i = 0; i < [hikeCoordinatePairs count]; i++)
        {
            polylineCoordinates[i] = CLLocationCoordinate2DMake([hikeCoordinatePairs[i][1] doubleValue] + deltaLatitude, [hikeCoordinatePairs[i][0] doubleValue] + deltaLongitude);
        }

        MLNPolyline *polyline = [MLNPolyline polylineWithCoordinates:polylineCoordinates
                                                               count:[hikeCoordinatePairs count]];

        [self.mapView addAnnotation:polyline];

        free(polylineCoordinates);

        // PA/NJ/DE polygons
        //
        NSDictionary *threestates = [NSJSONSerialization JSONObjectWithData:
                                     [NSData dataWithContentsOfFile:
                                      [[NSBundle mainBundle] pathForResource:@"threestates" ofType:@"geojson"]]
                                                                    options:0
                                                                      error:nil];

        for (NSDictionary *feature in threestates[@"features"])
        {
            NSArray *stateCoordinatePairs = feature[@"geometry"][@"coordinates"];

            while ([stateCoordinatePairs count] == 1) stateCoordinatePairs = stateCoordinatePairs[0];

            CLLocationCoordinate2D *polygonCoordinates = (CLLocationCoordinate2D *)malloc([stateCoordinatePairs count] * sizeof(CLLocationCoordinate2D));

            for (NSUInteger i = 0; i < [stateCoordinatePairs count]; i++)
            {
                polygonCoordinates[i] = CLLocationCoordinate2DMake([stateCoordinatePairs[i][1] doubleValue] + deltaLatitude, [stateCoordinatePairs[i][0] doubleValue] + deltaLongitude);
            }

            MLNPolygon *polygon = [MLNPolygon polygonWithCoordinates:polygonCoordinates count:[stateCoordinatePairs count]];
            polygon.title = feature[@"properties"][@"NAME"];

            [self.mapView addAnnotation:polygon];

            free(polygonCoordinates);
        }

        // Null Island polygon with an interior hole
        //
        CLLocationCoordinate2D innerCoordinates[] = {
            CLLocationCoordinate2DMake(-5 + deltaLatitude, -5 + deltaLongitude),
            CLLocationCoordinate2DMake(-5 + deltaLatitude, 5 + deltaLongitude),
            CLLocationCoordinate2DMake(5 + deltaLatitude, 5 + deltaLongitude),
            CLLocationCoordinate2DMake(5 + deltaLatitude, -5 + deltaLongitude),
        };
        MLNPolygon *innerPolygon = [MLNPolygon polygonWithCoordinates:innerCoordinates count:sizeof(innerCoordinates) / sizeof(innerCoordinates[0])];
        CLLocationCoordinate2D outerCoordinates[] = {
            CLLocationCoordinate2DMake(-10 + deltaLatitude, -10 + deltaLongitude),
            CLLocationCoordinate2DMake(-10 + deltaLatitude, 10 + deltaLongitude),
            CLLocationCoordinate2DMake(10 + deltaLatitude, 10 + deltaLongitude),
            CLLocationCoordinate2DMake(10 + deltaLatitude, -10 + deltaLongitude),
        };
        MLNPolygon *outerPolygon = [MLNPolygon polygonWithCoordinates:outerCoordinates count:sizeof(outerCoordinates) / sizeof(outerCoordinates[0]) interiorPolygons:@[innerPolygon]];
        [self.mapView addAnnotation:outerPolygon];
    }
}

- (void)addAnnotationWithCustomCallout
{
    [self.mapView removeAnnotations:self.mapView.annotations];

    MBXCustomCalloutAnnotation *firstAnnotation = [[MBXCustomCalloutAnnotation alloc] init];
    firstAnnotation.coordinate = CLLocationCoordinate2DMake(48.8533940, 2.3775439);
    firstAnnotation.title = @"Open anchored to annotation";
    firstAnnotation.anchoredToAnnotation = YES;
    firstAnnotation.dismissesAutomatically = NO;

    MBXCustomCalloutAnnotation *secondAnnotation = [[MBXCustomCalloutAnnotation alloc] init];
    secondAnnotation.coordinate = CLLocationCoordinate2DMake(48.8543940, 2.3775439);
    secondAnnotation.title = @"Open not anchored to annotation";
    secondAnnotation.anchoredToAnnotation = NO;
    secondAnnotation.dismissesAutomatically = NO;

    MBXCustomCalloutAnnotation *thirdAnnotation = [[MBXCustomCalloutAnnotation alloc] init];
    thirdAnnotation.coordinate = CLLocationCoordinate2DMake(48.8553940, 2.3775439);
    thirdAnnotation.title = @"Dismisses automatically";
    thirdAnnotation.anchoredToAnnotation = YES;
    thirdAnnotation.dismissesAutomatically = YES;

    NSArray *annotations = @[firstAnnotation, secondAnnotation, thirdAnnotation];
    [self.mapView addAnnotations:annotations];

    [self.mapView showAnnotations:annotations animated:YES];
}

// MARK: - Foursquare PMTiles Implementation
/*
  Add the new method that creates the PMTiles vector source and circle layer.
*/
- (void)addFoursquarePOIsPMTilesLayer {
    MLNVectorTileSource *foursquareSource = [[MLNVectorTileSource alloc] initWithIdentifier:@"foursquare-10M" configurationURLString:@"pmtiles://https://oliverwipfli.ch/data/foursquare-os-places-10M-2024-11-20.pmtiles"];

    // Also works
    // MLNVectorTileSource *foursquareSource =
    //   [[MLNVectorTileSource alloc] initWithIdentifier:@"foursquare-10M"
    //                                  tileURLTemplates:@[@"pmtiles://https://oliverwipfli.ch/data/foursquare-os-places-10M-2024-11-20.pmtiles"]
    //                                           options:nil];

    [self.mapView.style addSource:foursquareSource];

    MLNCircleStyleLayer *circleLayer = [[MLNCircleStyleLayer alloc] initWithIdentifier:@"foursquare-10M" source:foursquareSource];
    circleLayer.sourceLayerIdentifier = @"place";
    circleLayer.maximumZoomLevel = 11;
    circleLayer.circleColor = [NSExpression expressionForConstantValue:[UIColor colorWithRed:0.8 green:0.2 blue:0.2 alpha:1.0]];
    circleLayer.circleOpacity = [NSExpression expressionWithMLNJSONObject:@[
        @"interpolate",
        @[@"linear"],
        @[@"zoom"],
        @6, @0.5,
        @11, @0.5,
        @14, @1.0
    ]];
    circleLayer.circleRadius = [NSExpression expressionWithMLNJSONObject:@[
        @"interpolate",
        @[@"linear"],
        @[@"zoom"],
        @6, @1,
        @11, @3,
        @16, @4,
        @18, @8
    ]];
    [self.mapView.style addLayer:circleLayer];
}

- (void)styleBuildingExtrusions
{
    MLNSource* source = [self.mapView.style sourceWithIdentifier:@"composite"];
    if (source) {

        MLNFillExtrusionStyleLayer* layer = [[MLNFillExtrusionStyleLayer alloc] initWithIdentifier:@"extrudedBuildings" source:source];
        layer.sourceLayerIdentifier = @"building";
        layer.predicate = [NSPredicate predicateWithFormat:@"extrude == 'true' AND CAST(height, 'NSNumber') > 0"];
        layer.fillExtrusionBase = [NSExpression expressionForKeyPath:@"min_height"];
        layer.fillExtrusionHeight = [NSExpression expressionForKeyPath:@"height"];

        // Set the fill color to that of the existing building footprint layer, if it exists.
        MLNFillStyleLayer* buildingLayer = (MLNFillStyleLayer*)[self.mapView.style layerWithIdentifier:@"building"];
        if (buildingLayer) {
            if (buildingLayer.fillColor) {
                layer.fillExtrusionColor = buildingLayer.fillColor;
            } else {
                layer.fillExtrusionColor = [NSExpression expressionForConstantValue:[UIColor whiteColor]];
            }

            layer.fillExtrusionOpacity = [NSExpression expressionForConstantValue:@0.75];
        }

        MLNStyleLayer* labelLayer = [self.mapView.style layerWithIdentifier:@"waterway-label"];
        if (labelLayer) {
            [self.mapView.style insertLayer:layer belowLayer:labelLayer];
        } else {
            [self.mapView.style addLayer:layer];
        }
    }
}

- (void)styleWaterLayer
{
    MLNFillStyleLayer *waterLayer = (MLNFillStyleLayer *)[self.mapView.style layerWithIdentifier:@"water"];
    NSDictionary *waterColorStops = @{@6.0f: [UIColor yellowColor],
                                      @8.0f: [UIColor blueColor],
                                      @10.0f: [UIColor redColor],
                                      @12.0f: [UIColor greenColor],
                                      @14.0f: [UIColor blueColor]};
    NSExpression *fillColorExpression = [NSExpression mgl_expressionForInterpolatingExpression:NSExpression.zoomLevelVariableExpression
                                                                                 withCurveType:MLNExpressionInterpolationModeLinear
                                                                                    parameters:nil
                                                                                         stops:[NSExpression expressionForConstantValue:waterColorStops]];
    waterLayer.fillColor = fillColorExpression;

    NSDictionary *fillAntialiasedStops = @{@11: @YES,
                                           @12: @NO,
                                           @13: @YES,
                                           @14: @NO,
                                           @15: @YES};
    waterLayer.fillAntialiased = [NSExpression mgl_expressionForSteppingExpression:NSExpression.zoomLevelVariableExpression
                                                                    fromExpression:[NSExpression expressionForConstantValue:@NO]
                                                                             stops:[NSExpression expressionForConstantValue:fillAntialiasedStops]];
}

- (void)styleRoadLayer
{
    MLNLineStyleLayer *roadLayer = (MLNLineStyleLayer *)[self.mapView.style layerWithIdentifier:@"road-primary"];
    roadLayer.lineColor = [NSExpression expressionForConstantValue:[UIColor blackColor]];

    NSDictionary *lineWidthStops = @{@5: @5,
                                     @10: @15,
                                     @15: @30};
    NSExpression *lineWidthExpression = [NSExpression expressionWithFormat:
                                         @"mgl_interpolate:withCurveType:parameters:stops:($zoomLevel, 'linear', nil, %@)",
                                         lineWidthStops];
    roadLayer.lineWidth = lineWidthExpression;
    roadLayer.lineGapWidth = lineWidthExpression;

    NSDictionary *roadLineColorStops = @{@10: [UIColor purpleColor],
                                         @13: [UIColor yellowColor],
                                         @16: [UIColor cyanColor]};
    roadLayer.lineColor = [NSExpression expressionWithFormat:
                           @"mgl_interpolate:withCurveType:parameters:stops:($zoomLevel, 'linear', nil, %@)",
                           roadLineColorStops];

    roadLayer.visible = YES;
    roadLayer.maximumZoomLevel = 15;
    roadLayer.minimumZoomLevel = 13;
}

- (void)styleRasterLayer
{
    NSURL *rasterURL = [NSURL URLWithString:@"maptiler://sources/hybrid"];
    MLNRasterTileSource *rasterTileSource = [[MLNRasterTileSource alloc] initWithIdentifier:@"my-raster-tile-source" configurationURL:rasterURL tileSize:512];
    [self.mapView.style addSource:rasterTileSource];

    MLNRasterStyleLayer *rasterLayer = [[MLNRasterStyleLayer alloc] initWithIdentifier:@"my-raster-layer" source:rasterTileSource];
    NSDictionary *opacityStops = @{@20.0f: @1.0f,
                                   @5.0f: @0.0f};
    rasterLayer.rasterOpacity = [NSExpression expressionWithFormat:
                                 @"mgl_interpolate:withCurveType:parameters:stops:($zoomLevel, 'linear', nil, %@)",
                                 opacityStops];
    [self.mapView.style addLayer:rasterLayer];
}

- (void)styleShapeSource
{
    NSString *filePath = [[NSBundle bundleForClass:self.class] pathForResource:@"amsterdam" ofType:@"geojson"];
    NSURL *geoJSONURL = [NSURL fileURLWithPath:filePath];
    MLNShapeSource *source = [[MLNShapeSource alloc] initWithIdentifier:@"ams" URL:geoJSONURL options:nil];
    [self.mapView.style addSource:source];

    MLNFillStyleLayer *fillLayer = [[MLNFillStyleLayer alloc] initWithIdentifier:@"test" source:source];
    fillLayer.fillColor = [NSExpression expressionForConstantValue:[UIColor purpleColor]];
    [self.mapView.style addLayer:fillLayer];

}

- (void)styleSymbolLayer
{
    if (auto *stateLayer = (MLNSymbolStyleLayer *)[self.mapView.style layerWithIdentifier:@"state-label-lg"])
    {
        stateLayer.textColor = [NSExpression expressionForConstantValue:[UIColor redColor]];
    }
}

- (void)styleBuildingLayer
{
    MLNTransition transition =  { 5,  1 };
    self.mapView.style.transition = transition;
    if (auto *buildingLayer = (MLNFillStyleLayer *)[self.mapView.style layerWithIdentifier:@"building"])
    {
        buildingLayer.fillColor = [NSExpression expressionForConstantValue:[UIColor purpleColor]];
    }
}

- (void)styleFerryLayer
{
    if (auto *ferryLineLayer = (MLNLineStyleLayer *)[self.mapView.style layerWithIdentifier:@"ferry"])
    {
        ferryLineLayer.lineColor = [NSExpression expressionForConstantValue:[UIColor redColor]];
    }
}

- (void)removeParkLayer
{
    if (auto *parkLayer = (MLNFillStyleLayer *)[self.mapView.style layerWithIdentifier:@"park"])
    {
        [self.mapView.style removeLayer:parkLayer];
    }
}

- (BOOL)loadStyleFromBundle:(NSString*)path
{
    NSString *resourcePath = [[NSBundle mainBundle] pathForResource:path ofType:@"json"];
    if (NSURL* url = resourcePath ? [NSURL fileURLWithPath:resourcePath] : nil) {
        [self.mapView setStyleURL:url];
        return TRUE;
    } else {
        NSString *msg = [NSString stringWithFormat:@"Missing Style %@", path];
        UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Style File" message:msg preferredStyle:UIAlertControllerStyleAlert];
        [alertController addAction:[UIAlertAction actionWithTitle:@"Ok" style:UIAlertActionStyleCancel handler:nil]];
        [self presentViewController:alertController animated:YES completion:nil];
        return FALSE;
    }
}

- (void)styleFilteredFill
{
    // set style and focus on Texas
    if (![self loadStyleFromBundle:@"fill_filter_style"]) {
        return;
    }
    [self.mapView setCenterCoordinate:CLLocationCoordinate2DMake(31, -100) zoomLevel:3 animated:NO];

    // after slight delay, fill in Texas (atypical use; we want to clearly see the change for test purposes)
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^
    {
        MLNFillStyleLayer *statesLayer = (MLNFillStyleLayer *)[self.mapView.style layerWithIdentifier:@"states"];

        // filter
        statesLayer.predicate = [NSPredicate predicateWithFormat:@"name == 'Texas'"];

        // paint properties
        statesLayer.fillColor = [NSExpression expressionForConstantValue:[UIColor redColor]];
        statesLayer.fillOpacity = [NSExpression expressionForConstantValue:@0.25];
    });
}

- (void)styleFilteredLines
{
    // set style and focus on lower 48
    if (![self loadStyleFromBundle:@"line_filter_style"]) {
        return;
    }
    [self.mapView setCenterCoordinate:CLLocationCoordinate2DMake(40, -97) zoomLevel:5 animated:NO];

    // after slight delay, change styling for all Washington-named counties  (atypical use; we want to clearly see the change for test purposes)
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^
    {
        MLNLineStyleLayer *countiesLayer = (MLNLineStyleLayer *)[self.mapView.style layerWithIdentifier:@"counties"];

        // filter
        countiesLayer.predicate = [NSPredicate predicateWithFormat:@"NAME10 == 'Washington'"];

        // paint properties
        countiesLayer.lineColor = [NSExpression expressionForConstantValue:[UIColor redColor]];
        countiesLayer.lineOpacity = [NSExpression expressionForConstantValue:@0.75];
        countiesLayer.lineWidth = [NSExpression expressionForConstantValue:@5];
    });
}

- (void)styleNumericFilteredFills
{
    // set style and focus on lower 48
    if (![self loadStyleFromBundle:@"numeric_filter_style"]) {
        return;
    }
    [self.mapView setCenterCoordinate:CLLocationCoordinate2DMake(40, -97) zoomLevel:5 animated:NO];

    // after slight delay, change styling for regions 200-299 (atypical use; we want to clearly see the change for test purposes)
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^
    {
        MLNFillStyleLayer *regionsLayer = (MLNFillStyleLayer *)[self.mapView.style layerWithIdentifier:@"regions"];

        // filter (testing both inline and format strings)
        regionsLayer.predicate = [NSPredicate predicateWithFormat:@"CAST(HRRNUM, 'NSNumber') >= %@ AND CAST(HRRNUM, 'NSNumber') < 300", @(200)];

        // paint properties
        regionsLayer.fillColor = [NSExpression expressionForConstantValue:[UIColor blueColor]];
        regionsLayer.fillOpacity = [NSExpression expressionForConstantValue:@0.5];
    });
}

- (void)styleQuery
{
    CGRect queryRect = CGRectInset(self.mapView.bounds, 100, 200);
    NSArray *visibleFeatures = [self.mapView visibleFeaturesInRect:queryRect];

    NSString *querySourceID = @"query-source-id";
    NSString *queryLayerID = @"query-layer-id";

    // RTE if you don't remove the layer first
    // RTE if you pass a nill layer to remove layer
    MLNStyleLayer *layer = [self.mapView.style layerWithIdentifier:queryLayerID];
    if (layer) {
        [self.mapView.style removeLayer:layer];
    }

    // RTE if you pass a nill source to remove source
    MLNSource *source = [self.mapView.style sourceWithIdentifier:querySourceID];
    if (source) {
        [self.mapView.style removeSource:source];
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        MLNShapeSource *shapeSource = [[MLNShapeSource alloc] initWithIdentifier:querySourceID features:visibleFeatures options:nil];
        [self.mapView.style addSource:shapeSource];

        MLNFillStyleLayer *fillLayer = [[MLNFillStyleLayer alloc] initWithIdentifier:queryLayerID source:shapeSource];
        fillLayer.fillColor = [NSExpression expressionForConstantValue:[UIColor blueColor]];
        fillLayer.fillOpacity = [NSExpression expressionForConstantValue:@0.5];
        [self.mapView.style addLayer:fillLayer];
    });
}

- (void)styleFeature
{
    self.mapView.zoomLevel = 10;
    self.mapView.centerCoordinate = CLLocationCoordinate2DMake(51.068585180672635, -114.06074523925781);

    CLLocationCoordinate2D leafCoords[] = {
        {50.9683733218221,-114.07035827636719},
        {51.02325750523972,-114.06967163085938},
        {51.009434536947786,-114.14245605468749},
        {51.030599281184124,-114.12597656249999},
        {51.060386316691016,-114.21043395996094},
        {51.063838646941576,-114.17816162109375},
        {51.08152779888779,-114.19876098632812},
        {51.08066507029602,-114.16854858398438},
        {51.09662294502995,-114.17472839355469},
        {51.07764539352731,-114.114990234375},
        {51.13670896949613,-114.12391662597656},
        {51.13369295212583,-114.09576416015624},
        {51.17546878815025,-114.07585144042969},
        {51.140155605265896,-114.04632568359375},
        {51.15049396880196,-114.01542663574219},
        {51.088860342359965,-114.00924682617186},
        {51.12205789681453,-113.94813537597656},
        {51.106539930027225,-113.94882202148438},
        {51.117747873223344,-113.92616271972656},
        {51.10093493903458,-113.92616271972656},
        {51.10697105503078,-113.90625},
        {51.09144802136697,-113.9117431640625},
        {51.04916446529361,-113.97010803222655},
        {51.045279344649146,-113.9398956298828},
        {51.022825599852496,-114.06211853027344},
        {51.045279344649146,-113.9398956298828},
        {51.022825599852496,-114.06211853027344},
        {51.022825599852496,-114.06280517578125},
        {50.968805734317804,-114.06280517578125},
        {50.9683733218221,-114.07035827636719},
    };
    NSUInteger coordsCount = sizeof(leafCoords) / sizeof(leafCoords[0]);

    MLNPolygonFeature *feature = [MLNPolygonFeature polygonWithCoordinates:leafCoords count:coordsCount];
    feature.identifier = @"leaf-feature";
    feature.attributes = @{@"color": @"red"};

    MLNShapeSource *source = [[MLNShapeSource alloc] initWithIdentifier:@"leaf-source" shape:feature options:nil];
    [self.mapView.style addSource:source];

    MLNFillStyleLayer *layer = [[MLNFillStyleLayer alloc] initWithIdentifier:@"leaf-fill-layer" source:source];
    layer.predicate = [NSPredicate predicateWithFormat:@"color = 'red'"];
    layer.fillColor = [NSExpression expressionForConstantValue:[UIColor redColor]];
    [self.mapView.style addLayer:layer];

    NSString *geoJSON = @"{\"type\": \"Feature\", \"properties\": {\"color\": \"green\"}, \"geometry\": { \"type\": \"Point\", \"coordinates\": [ -114.06847000122069, 51.050459433092655 ] }}";

    NSData *data = [geoJSON dataUsingEncoding:NSUTF8StringEncoding];
    MLNShape *shape = [MLNShape shapeWithData:data encoding:NSUTF8StringEncoding error:NULL];
    MLNShapeSource *pointSource = [[MLNShapeSource alloc] initWithIdentifier:@"leaf-point-source" shape:shape options:nil];
    [self.mapView.style addSource:pointSource];

    MLNCircleStyleLayer *circleLayer = [[MLNCircleStyleLayer alloc] initWithIdentifier:@"leaf-circle-layer" source:pointSource];
    circleLayer.circleColor = [NSExpression expressionForConstantValue:[UIColor greenColor]];
    circleLayer.predicate = [NSPredicate predicateWithFormat:@"color = 'green'"];
    [self.mapView.style addLayer:circleLayer];


    CLLocationCoordinate2D squareCoords[] = {
        {51.056070541830934, -114.0274429321289},
        {51.07937094724242, -114.0274429321289},
        {51.07937094724242, -113.98761749267578},
        {51.05607054183093, -113.98761749267578},
        {51.056070541830934, -114.0274429321289},
    };
    MLNPolygon *polygon = [MLNPolygon polygonWithCoordinates:squareCoords count:sizeof(squareCoords)/sizeof(squareCoords[0])];
    MLNShapeSource *plainShapeSource = [[MLNShapeSource alloc] initWithIdentifier:@"leaf-plain-shape-source" shape:polygon options:nil];
    [self.mapView.style addSource:plainShapeSource];

    MLNFillStyleLayer *plainFillLayer = [[MLNFillStyleLayer alloc] initWithIdentifier:@"leaf-plain-fill-layer" source:plainShapeSource];
    plainFillLayer.fillColor = [NSExpression expressionForConstantValue:[UIColor yellowColor]];
    [self.mapView.style addLayer:plainFillLayer];
}

- (void)updateShapeSourceData
{
    [self.mapView setCenterCoordinate:CLLocationCoordinate2DMake(40.329795743702064, -107.75390625) zoomLevel:11 animated:NO];

    NSString *geoJSON = @"{\"type\": \"FeatureCollection\",\"features\": [{\"type\": \"Feature\",\"properties\": {},\"geometry\": {\"type\": \"LineString\",\"coordinates\": [[-107.75390625,40.329795743702064],[-104.34814453125,37.64903402157866]]}}]}";

    NSData *data = [geoJSON dataUsingEncoding:NSUTF8StringEncoding];
    MLNShape *shape = [MLNShape shapeWithData:data encoding:NSUTF8StringEncoding error:NULL];
    MLNShapeSource *source = [[MLNShapeSource alloc] initWithIdentifier:@"mutable-data-source-id" shape:shape options:nil];
    [self.mapView.style addSource:source];

    MLNLineStyleLayer *layer = [[MLNLineStyleLayer alloc] initWithIdentifier:@"mutable-data-layer-id" source:source];
    [self.mapView.style addLayer:layer];

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        NSString *updatedGeoJSON = @"{\"type\": \"FeatureCollection\",\"features\": [{\"type\": \"Feature\",\"properties\": {},\"geometry\": {\"type\": \"LineString\",\"coordinates\": [[-107.75390625,40.329795743702064],[-109.34814453125,37.64903402157866]]}}]}";
        NSData *updatedData = [updatedGeoJSON dataUsingEncoding:NSUTF8StringEncoding];
        MLNShape *updatedShape = [MLNShape shapeWithData:updatedData encoding:NSUTF8StringEncoding error:NULL];
        source.shape = updatedShape;
    });
}

- (void)updateShapeSourceURL
{
    [self.mapView setCenterCoordinate:CLLocationCoordinate2DMake(48.668731, -122.857151) zoomLevel:11 animated:NO];

    NSString *filePath = [[NSBundle bundleForClass:self.class] pathForResource:@"polyline" ofType:@"geojson"];
    NSURL *geoJSONURL = [NSURL fileURLWithPath:filePath];
    MLNShapeSource *source = [[MLNShapeSource alloc] initWithIdentifier:@"mutable-data-source-url-id" URL:geoJSONURL options:nil];
    [self.mapView.style addSource:source];

    MLNLineStyleLayer *layer = [[MLNLineStyleLayer alloc] initWithIdentifier:@"mutable-data-layer-url-id" source:source];
    [self.mapView.style addLayer:layer];

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [self.mapView setCenterCoordinate:CLLocationCoordinate2DMake(41.563986787078704, -75.04843935793578) zoomLevel:8 animated:NO];

        NSString *threeStatesFilePath = [[NSBundle bundleForClass:self.class] pathForResource:@"threestates" ofType:@"geojson"];
        NSURL *updatedGeoJSONURL = [NSURL fileURLWithPath:threeStatesFilePath];

        source.URL = updatedGeoJSONURL;
    });
}

- (void)updateShapeSourceFeatures
{
    [self.mapView setCenterCoordinate:CLLocationCoordinate2DMake(-41.1520, 288.6592) zoomLevel:10 animated:NO];

    CLLocationCoordinate2D smallBox[] = {
        {-41.14763798539186, 288.68019104003906},
        {-41.140915920129665, 288.68019104003906},
        {-41.140915920129665, 288.6887741088867},
        {-41.14763798539186, 288.6887741088867},
        {-41.14763798539186, 288.68019104003906}
    };

    CLLocationCoordinate2D largeBox[] = {
        {-41.17710352162799, 288.67298126220703},
        {-41.13962313627545, 288.67298126220703},
        {-41.13962313627545, 288.7261962890625},
        {-41.17710352162799, 288.7261962890625},
        {-41.17710352162799, 288.67298126220703}
    };

    MLNPolygonFeature *smallBoxFeature = [MLNPolygonFeature polygonWithCoordinates:smallBox count:sizeof(smallBox)/sizeof(smallBox[0])];
    MLNPolygonFeature *largeBoxFeature = [MLNPolygonFeature polygonWithCoordinates:largeBox count:sizeof(largeBox)/sizeof(largeBox[0])];

    MLNShapeSource *source = [[MLNShapeSource alloc] initWithIdentifier:@"mutable-data-source-features-id"
                                                                    shape:smallBoxFeature
                                                                    options:nil];
    [self.mapView.style addSource:source];

    MLNFillStyleLayer *layer = [[MLNFillStyleLayer alloc] initWithIdentifier:@"mutable-data-layer-features-id" source:source];
    layer.fillColor = [NSExpression expressionForConstantValue:[UIColor redColor]];
    [self.mapView.style addLayer:layer];

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        source.shape = largeBoxFeature;
    });
}

- (void)styleDynamicPointCollection
{
    [self.mapView setCenterCoordinate:CLLocationCoordinate2DMake(36.9979, -109.0441) zoomLevel:14 animated:NO];

    CLLocationCoordinate2D coordinates[] = {
        {37.00145594210082, -109.04960632324219},
        {37.00173012609867, -109.0404224395752},
        {36.99453246847359, -109.04960632324219},
        {36.99508088541243, -109.04007911682129},
    };
    MLNPointCollectionFeature *feature = [MLNPointCollectionFeature pointCollectionWithCoordinates:coordinates count:4];
    MLNShapeSource *source = [[MLNShapeSource alloc] initWithIdentifier:@"wiggle-source" shape:feature options:nil];
    [self.mapView.style addSource:source];

    MLNCircleStyleLayer *layer = [[MLNCircleStyleLayer alloc] initWithIdentifier:@"wiggle-layer" source:source];
    [self.mapView.style addLayer:layer];
}

- (void)styleVectorTileSource
{
    NSURL *url = [[NSURL alloc] initWithString:@"maptiler://source/hillshade"];
    MLNVectorTileSource *vectorTileSource = [[MLNVectorTileSource alloc] initWithIdentifier:@"style-vector-tile-source-id" configurationURL:url];
    [self.mapView.style addSource:vectorTileSource];

    MLNBackgroundStyleLayer *backgroundLayer = [[MLNBackgroundStyleLayer alloc] initWithIdentifier:@"style-vector-background-layer-id"];
    backgroundLayer.backgroundColor = [NSExpression expressionForConstantValue:[UIColor blackColor]];
    [self.mapView.style addLayer:backgroundLayer];

    MLNLineStyleLayer *lineLayer = [[MLNLineStyleLayer alloc] initWithIdentifier:@"style-vector-line-layer-id" source:vectorTileSource];
    lineLayer.sourceLayerIdentifier = @"contour";
    lineLayer.lineJoin = [NSExpression expressionForConstantValue:@"round"];
    lineLayer.lineCap = [NSExpression expressionForConstantValue:@"round"];
    lineLayer.lineColor = [NSExpression expressionForConstantValue:[UIColor greenColor]];

    [self.mapView.style addLayer:lineLayer];
}

- (void)styleRasterTileSource
{
    NSString *tileURL = [NSString stringWithFormat:@"https://stamen-tiles.a.ssl.fastly.net/terrain-background/{z}/{x}/{y}%@.jpg", UIScreen.mainScreen.nativeScale > 1 ? @"@2x" : @""];
    MLNRasterTileSource *rasterTileSource = [[MLNRasterTileSource alloc] initWithIdentifier:@"style-raster-tile-source-id" tileURLTemplates:@[tileURL] options:@{
        MLNTileSourceOptionTileSize: @256,
    }];
    [self.mapView.style addSource:rasterTileSource];

    MLNRasterStyleLayer *rasterLayer = [[MLNRasterStyleLayer alloc] initWithIdentifier:@"style-raster-layer-id" source:rasterTileSource];
    [self.mapView.style addLayer:rasterLayer];
}

- (NSURL*)radarImageURL:(int)index
{
    return [NSURL URLWithString:
            [NSString stringWithFormat:@"https://maplibre.org/maplibre-gl-js/docs/assets/radar%d.gif", index]];
}

- (void)styleImageSource
{
    MLNCoordinateQuad coordinateQuad = {
        { 46.437, -80.425 },
        { 37.936, -80.425 },
        { 37.936, -71.516 },
        { 46.437, -71.516 } };

    MLNImageSource *imageSource = [[MLNImageSource alloc] initWithIdentifier:@"style-image-source-id"
                                                              coordinateQuad:coordinateQuad
                                                                         URL:[self radarImageURL:0]];

    [self.mapView.style addSource:imageSource];

    MLNRasterStyleLayer *rasterLayer = [[MLNRasterStyleLayer alloc] initWithIdentifier:@"style-raster-image-layer-id" source:imageSource];
    [self.mapView.style addLayer:rasterLayer];

    [NSTimer scheduledTimerWithTimeInterval:1.0
                                     target:self
                                   selector:@selector(updateAnimatedImageSource:)
                                   userInfo:imageSource
                                    repeats:YES];

    const CGFloat maximumPadding = 50;
    const CGSize frameSize = self.mapView.frame.size;
    const CGFloat yPadding = (frameSize.height / 5 <= maximumPadding) ? (frameSize.height / 5) : maximumPadding;
    const CGFloat xPadding = (frameSize.width / 5 <= maximumPadding) ? (frameSize.width / 5) : maximumPadding;
    [self.mapView setVisibleCoordinateBounds:MLNCoordinateBoundsMake(coordinateQuad.bottomLeft, coordinateQuad.topRight)
                         edgePadding:UIEdgeInsetsMake(yPadding, xPadding, yPadding, xPadding)
                            animated:YES
                   completionHandler:nil];
}


- (void)updateAnimatedImageSource:(NSTimer *)timer
{
    MLNImageSource *imageSource = (MLNImageSource *)timer.userInfo;
    if (![self.mapView.style sourceWithIdentifier:imageSource.identifier]) {
        // the source has been removed, probably by reloading the style, if we try to update
        // it now, we will crash with 'This source got invalidated after the style change'
        [timer invalidate];
        return;
    }

    static int radarSuffix = 0;
    [imageSource setValue:[self radarImageURL:radarSuffix] forKey:@"URL"];
    radarSuffix = (radarSuffix + 1) % 5;
}

-(void)toggleStyleLabelsLanguage
{
    _localizingLabels = !_localizingLabels;
    [self.mapView.style localizeLabelsIntoLocale:_localizingLabels ? [NSLocale localeWithLocaleIdentifier:@"mul"] : nil];
}

- (void)styleLineGradient
{
    CLLocationCoordinate2D coords[] = {
        { 43.84455590478528, 10.504238605499268 },
        { 43.84385562343126, 10.504125952720642 },
        { 43.84388657526694, 10.503299832344055 },
        { 43.84332557075269, 10.503235459327698 },
        { 43.843441641085036, 10.502264499664307 },
        { 43.84396395478592, 10.50242006778717 },
        { 43.84406067904351, 10.501744151115416 },
        { 43.84422317544319, 10.501792430877686 }
    };
    NSInteger count = sizeof(coords) / sizeof(coords[0]);

    [self.mapView setCenterCoordinate:coords[0] zoomLevel:16 animated:YES];

    MLNPolylineFeature *routeLine = [MLNPolylineFeature polylineWithCoordinates:coords count:count];

    NSDictionary *sourceOptions = @{ MLNShapeSourceOptionLineDistanceMetrics: @YES };

    MLNShapeSource *routeSource = [[MLNShapeSource alloc] initWithIdentifier:@"style-route-source" shape:routeLine options:sourceOptions];

    MLNLineStyleLayer *baseRouteLayer = [[MLNLineStyleLayer alloc] initWithIdentifier:@"style-base-route-layer" source:routeSource];
    baseRouteLayer.lineColor = [NSExpression expressionForConstantValue:[UIColor orangeColor]];
    baseRouteLayer.lineWidth = [NSExpression expressionForConstantValue:@20];
    baseRouteLayer.lineOpacity = [NSExpression expressionForConstantValue:@0.95];
    baseRouteLayer.lineCap = [NSExpression expressionForConstantValue:@"round"];
    baseRouteLayer.lineJoin = [NSExpression expressionForConstantValue:@"round"];

    MLNLineStyleLayer *routeLayer = [[MLNLineStyleLayer alloc] initWithIdentifier:@"style-route-layer" source:routeSource];
    routeLayer.lineColor = [NSExpression expressionForConstantValue:[UIColor whiteColor]];
    routeLayer.lineWidth = [NSExpression expressionForConstantValue:@15];
    routeLayer.lineOpacity = [NSExpression expressionForConstantValue:@0.8];
    routeLayer.lineCap = [NSExpression expressionForConstantValue:@"round"];
    routeLayer.lineJoin = [NSExpression expressionForConstantValue:@"round"];
    // Create stops dictionary
    NSDictionary *stops = @{
        @0: [UIColor blueColor],
        @0.1: [UIColor colorWithRed:25 / 255.0 green:41 /255.0 blue:88 / 255.0 alpha:1.0],
        @0.3: [UIColor cyanColor],
        @0.5: [UIColor greenColor],
        @0.7: [UIColor yellowColor],
        @1: [UIColor redColor],
    };
    // Create an expression that will interpolate the color of the line
    // (mgl_interpolate:withCurveType:parameters:stops:($lineProgress, 'linear', nil, %@))
    NSExpression *lineGradientExpression = [NSExpression expressionWithFormat:@"mgl_interpolate:withCurveType:parameters:stops:($lineProgress, 'linear', nil, %@)", stops];
    routeLayer.lineGradient = lineGradientExpression;

    [self removeLayer:baseRouteLayer.identifier];
    [self removeLayer:routeLayer.identifier];
    [self removeSource:routeSource.identifier];
    [self.mapView.style addSource:routeSource];
    [self.mapView.style addLayer:baseRouteLayer];
    [self.mapView.style addLayer:routeLayer];
}

- (void)addCustomDrawableLayer
{
    // Create a CustomLayer that uses the Drawable/Builder toolkit to generate and render geometry
    ExampleCustomDrawableStyleLayer* layer = [[ExampleCustomDrawableStyleLayer alloc] initWithIdentifier:@"custom-drawable-layer"];

    if (layer) {
        [self.mapView.style addLayer:layer];
    }
}

- (void)removeSource:(NSString*)ident
{
    if (MLNSource *source = [self.mapView.style sourceWithIdentifier:ident])
    {
        [self.mapView.style removeSource:source];
    }
}

- (void)removeLayer:(NSString*)ident
{
    if (MLNStyleLayer* layer = [self.mapView.style layerWithIdentifier:ident])
    {
        [self.mapView.style removeLayer:layer];
    }
}

- (void)styleRouteLine
{
    CLLocationCoordinate2D coords[] = {
        { 43.84455590478528, 10.504238605499268 },
        { 43.84385562343126, 10.504125952720642 },
        { 43.84388657526694, 10.503299832344055 },
        { 43.84332557075269, 10.503235459327698 },
        { 43.843441641085036, 10.502264499664307 },
        { 43.84396395478592, 10.50242006778717 },
        { 43.84406067904351, 10.501744151115416 },
        { 43.84422317544319, 10.501792430877686 }
    };
    NSInteger count = sizeof(coords) / sizeof(coords[0]);

    [self.mapView setCenterCoordinate:coords[0] zoomLevel:16 animated:YES];

    MLNPolylineFeature *routeLine = [MLNPolylineFeature polylineWithCoordinates:coords count:count];

    MLNShapeSource *routeSource = [[MLNShapeSource alloc] initWithIdentifier:@"style-route-source" shape:routeLine options:nil];

    MLNLineStyleLayer *baseRouteLayer = [[MLNLineStyleLayer alloc] initWithIdentifier:@"style-base-route-layer" source:routeSource];
    baseRouteLayer.lineColor = [NSExpression expressionForConstantValue:[UIColor orangeColor]];
    baseRouteLayer.lineWidth = [NSExpression expressionForConstantValue:@20];
    baseRouteLayer.lineOpacity = [NSExpression expressionForConstantValue:@0.5];
    baseRouteLayer.lineCap = [NSExpression expressionForConstantValue:@"round"];
    baseRouteLayer.lineJoin = [NSExpression expressionForConstantValue:@"round"];

    MLNLineStyleLayer *routeLayer = [[MLNLineStyleLayer alloc] initWithIdentifier:@"style-route-layer" source:routeSource];
    routeLayer.lineColor = [NSExpression expressionForConstantValue:[UIColor whiteColor]];
    routeLayer.lineWidth = [NSExpression expressionForConstantValue:@15];
    routeLayer.lineOpacity = [NSExpression expressionForConstantValue:@0.8];
    routeLayer.lineCap = [NSExpression expressionForConstantValue:@"round"];
    routeLayer.lineJoin = [NSExpression expressionForConstantValue:@"round"];

    [self removeLayer:baseRouteLayer.identifier];
    [self removeLayer:routeLayer.identifier];
    [self removeSource:routeSource.identifier];
    [self.mapView.style addSource:routeSource];
    [self.mapView.style addLayer:baseRouteLayer];
    [self.mapView.style addLayer:routeLayer];
}

- (void)styleAddCustomTriangleLayer
{
    if (CustomStyleLayerExample *layer = [[CustomStyleLayerExample alloc] initWithIdentifier:@"mbx-custom"]) {
        [self.mapView.style addLayer:layer];
    }
}

- (void)stylePolygonWithDDS
{
    CLLocationCoordinate2D leftCoords[] = {
        {37.73081027834234, -122.49412536621094},
        {37.7566013348511, -122.49412536621094},
        {37.7566013348511, -122.46253967285156},
        {37.73081027834234, -122.46253967285156},
        {37.73081027834234, -122.49412536621094},
    };
    CLLocationCoordinate2D rightCoords[] = {
        {37.73135334055843, -122.44640350341795},
        {37.75741564287944, -122.44640350341795},
        {37.75741564287944, -122.41310119628906},
        {37.73135334055843, -122.41310119628906},
        {37.73135334055843, -122.44640350341795},
    };
    MLNPolygonFeature *leftFeature = [MLNPolygonFeature polygonWithCoordinates:leftCoords count:5];
    leftFeature.attributes = @{@"fill": @(YES)};

    MLNPolygonFeature *rightFeature = [MLNPolygonFeature polygonWithCoordinates:rightCoords count:5];
    rightFeature.attributes = @{@"opacity": @(0.5)};

    MLNShapeSource *shapeSource = [[MLNShapeSource alloc] initWithIdentifier:@"shape-source" features:@[leftFeature, rightFeature] options:nil];
    [self.mapView.style addSource:shapeSource];

    // source, categorical function that sets any feature with a "fill" attribute value of true to red color and anything without to green
    MLNFillStyleLayer *fillStyleLayer = [[MLNFillStyleLayer alloc] initWithIdentifier:@"fill-layer" source:shapeSource];
    fillStyleLayer.fillColor = [NSExpression mgl_expressionForConditional:[NSPredicate predicateWithFormat:@"fill == YES"]
                                                           trueExpression:[NSExpression expressionForConstantValue:[UIColor greenColor]]
                                                         falseExpresssion:[NSExpression expressionForConstantValue:[UIColor redColor]]];



    // source, identity function that sets any feature with an "opacity" attribute to use that value and anything without to 1.0
    fillStyleLayer.fillOpacity = [NSExpression mgl_expressionForConditional:[NSPredicate predicateWithFormat:@"opacity != nil"]
                                                             trueExpression:[NSExpression expressionForKeyPath:@"opacity"]
                                                           falseExpresssion:[NSExpression expressionForConstantValue:@1.0]];
    [self.mapView.style addLayer:fillStyleLayer];
}

- (void)addLatLonGrid
{
    MLNComputedShapeSource *source = [[MLNComputedShapeSource alloc] initWithIdentifier:@"latlon"
                                                                              options:@{MLNShapeSourceOptionMaximumZoomLevel:@14}];
    source.dataSource = self;
    [self.mapView.style addSource:source];
    MLNLineStyleLayer *lineLayer = [[MLNLineStyleLayer alloc] initWithIdentifier:@"latlonlines"
                                                                          source:source];
    [self.mapView.style addLayer:lineLayer];
    MLNSymbolStyleLayer *labelLayer = [[MLNSymbolStyleLayer alloc] initWithIdentifier:@"latlonlabels"
                                                                               source:source];
    labelLayer.text = [NSExpression expressionForKeyPath:@"value"];
    [self.mapView.style addLayer:labelLayer];
}

- (NSString *)bestLanguageForUser
{
    // https://www.mapbox.com/vector-tiles/mapbox-streets-v8/#name-text--name_lang-code-text
    NSArray *supportedLanguages = @[ @"ar", @"de", @"en", @"es", @"fr", @"ja", @"ko", @"pt", @"ru", @"zh", @"zh-Hans", @"zh-Hant" ];
    NSArray<NSString *> *preferredLanguages = [NSBundle preferredLocalizationsFromArray:supportedLanguages forPreferences:[NSLocale preferredLanguages]];
    NSString *mostSpecificLanguage;

    for (NSString *language in preferredLanguages)
    {
        if (language.length > mostSpecificLanguage.length)
        {
            mostSpecificLanguage = language;
        }
    }

    return mostSpecificLanguage ?: @"en";
}

- (IBAction)startWorldTour
{
    _isTouringWorld = YES;

    [self.mapView removeAnnotations:self.mapView.annotations];
    NSUInteger numberOfAnnotations = sizeof(WorldTourDestinations) / sizeof(WorldTourDestinations[0]);
    NSMutableArray *annotations = [NSMutableArray arrayWithCapacity:numberOfAnnotations];
    for (NSUInteger i = 0; i < numberOfAnnotations; i++)
    {
        MBXDroppedPinAnnotation *annotation = [[MBXDroppedPinAnnotation alloc] init];
        annotation.coordinate = WorldTourDestinations[i];
        [annotations addObject:annotation];
    }
    [self.mapView addAnnotations:annotations];
    [self continueWorldTourWithRemainingAnnotations:annotations];
}

- (void)continueWorldTourWithRemainingAnnotations:(NSMutableArray<MLNPointAnnotation *> *)annotations
{
    MLNPointAnnotation *nextAnnotation = annotations.firstObject;
    if (!nextAnnotation || !_isTouringWorld)
    {
        _isTouringWorld = NO;
        return;
    }

    [annotations removeObjectAtIndex:0];
    MLNMapCamera *camera = [MLNMapCamera cameraLookingAtCenterCoordinate:nextAnnotation.coordinate
                                                          acrossDistance:10
                                                                   pitch:arc4random_uniform(60)
                                                                 heading:arc4random_uniform(360)];
    __weak MBXViewController *weakSelf = self;
    [self.mapView flyToCamera:camera completionHandler:^{
        MBXViewController *strongSelf = weakSelf;
        [strongSelf performSelector:@selector(continueWorldTourWithRemainingAnnotations:)
                         withObject:annotations
                         afterDelay:2];
    }];
}

- (void)toggleCustomUserDot
{
    _customUserLocationAnnnotationEnabled = !_customUserLocationAnnnotationEnabled;
    self.mapView.showsUserLocation = NO;
    self.mapView.userTrackingMode = MLNUserTrackingModeFollow;
}

- (void)testQueryPointAnnotations {
    NSNumber *visibleAnnotationCount = @(self.mapView.visibleAnnotations.count);
    NSString *message;
    if ([visibleAnnotationCount integerValue] == 1) {
        message = [NSString stringWithFormat:@"There is %@ visible annotation.", visibleAnnotationCount];
    } else {
        message = [NSString stringWithFormat:@"There are %@ visible annotations.", visibleAnnotationCount];
    }

    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Visible Annotations" message:message preferredStyle:UIAlertControllerStyleAlert];
    [alertController addAction:[UIAlertAction actionWithTitle:@"Ok" style:UIAlertActionStyleCancel handler:nil]];
    [self presentViewController:alertController animated:YES completion:nil];
}

- (id<MLNAnnotation>)randomOffscreenPointAnnotation {

    NSPredicate *pointAnnotationPredicate = [NSPredicate predicateWithBlock:^BOOL(id  _Nullable evaluatedObject, NSDictionary<NSString *,id> * _Nullable bindings) {
        return [evaluatedObject isKindOfClass:[MLNPointAnnotation class]];
    }];

    NSArray *annotations = [self.mapView.annotations filteredArrayUsingPredicate:pointAnnotationPredicate];

    if (annotations.count == 0) {
        return nil;
    }

    NSArray *visibleAnnotations = [self.mapView.visibleAnnotations filteredArrayUsingPredicate:pointAnnotationPredicate];

    if (visibleAnnotations.count == annotations.count) {
        return nil;
    }

    NSMutableArray *invisibleAnnotations = [annotations mutableCopy];

    if (visibleAnnotations.count > 0) {
        [invisibleAnnotations removeObjectsInArray:visibleAnnotations];
    }

    // Now pick a random offscreen annotation.
    uint32_t index = arc4random_uniform((uint32_t)invisibleAnnotations.count);
    return invisibleAnnotations[index];
}

- (void)selectAnOffscreenPointAnnotation {
    id<MLNAnnotation> annotation = [self randomOffscreenPointAnnotation];
    if (annotation) {
        [self.mapView selectAnnotation:annotation animated:YES completionHandler:nil];

        NSAssert(self.mapView.selectedAnnotations.firstObject, @"The annotation was not selected");
    }
}

- (void)centerSelectedAnnotation {
    id<MLNAnnotation> annotation = self.mapView.selectedAnnotations.firstObject;

    if (!annotation)
        return;

    CGPoint point = [self.mapView convertCoordinate:annotation.coordinate toPointToView:self.mapView];

    // Animate, so that point becomes the the center
    CLLocationCoordinate2D center = [self.mapView convertPoint:point toCoordinateFromView:self.mapView];
    [self.mapView setCenterCoordinate:center animated:YES];
}

- (void)addVisibleAreaPolyline {
    CGRect constrainedRect = UIEdgeInsetsInsetRect(self.mapView.bounds, self.mapView.contentInset);

    CLLocationCoordinate2D lineCoords[5];

    lineCoords[0] = [self.mapView convertPoint: CGPointMake(CGRectGetMinX(constrainedRect), CGRectGetMinY(constrainedRect)) toCoordinateFromView:self.mapView];
    lineCoords[1] = [self.mapView convertPoint: CGPointMake(CGRectGetMaxX(constrainedRect), CGRectGetMinY(constrainedRect)) toCoordinateFromView:self.mapView];
    lineCoords[2] = [self.mapView convertPoint: CGPointMake(CGRectGetMaxX(constrainedRect), CGRectGetMaxY(constrainedRect)) toCoordinateFromView:self.mapView];
    lineCoords[3] = [self.mapView convertPoint: CGPointMake(CGRectGetMinX(constrainedRect), CGRectGetMaxY(constrainedRect)) toCoordinateFromView:self.mapView];
    lineCoords[4] = lineCoords[0];

    MLNPolyline *line = [MLNPolyline polylineWithCoordinates:lineCoords
                                                       count:sizeof(lineCoords)/sizeof(lineCoords[0])];
    [self.mapView addAnnotation:line];
}

- (void)loadMissingIcon
{
    self.mapView.centerCoordinate = CLLocationCoordinate2DMake(0, 0);
    self.mapView.zoomLevel = 1;
    NSURL *customStyleJSON = [[NSBundle mainBundle] URLForResource:@"missing_icon" withExtension:@"json"];
    [self.mapView setStyleURL:customStyleJSON];
}

- (UIImage *)mapView:(MLNMapView *)mapView didFailToLoadImage:(NSString *)imageName {
    UIImage *backupImage = [UIImage imageNamed:@"MissingImage"];
    return backupImage;
}

- (void)flyToWithLatLngBoundsAndPadding
{
    [self.mapView removeAnnotations:self.mapView.annotations];
    CLLocationCoordinate2D sw = CLLocationCoordinate2DMake(48, 11);
    CLLocationCoordinate2D ne = CLLocationCoordinate2DMake(50, 12);

    UIEdgeInsets padding = UIEdgeInsetsMake(200, 200, 0, 0);
    MLNMapCamera *cameraWithoutPadding = [self.mapView cameraThatFitsCoordinateBounds:MLNCoordinateBoundsMake(sw, ne)
                                                                          edgePadding:padding];


    MLNPointAnnotation *annotation = [MLNPointAnnotation new];
    annotation.coordinate = cameraWithoutPadding.centerCoordinate;
    annotation.title = @"Bounds center";
    [self.mapView addAnnotation: annotation];

    __weak MBXViewController *weakSelf = self;
    [self.mapView flyToCamera:cameraWithoutPadding edgePadding:padding withDuration:5 completionHandler:^{
        [weakSelf.mapView flyToCamera:cameraWithoutPadding edgePadding:UIEdgeInsetsZero withDuration:5 completionHandler:^{

        }];
    }];
}

-(void)cycleTileLodMode
{
    // TileLodMode::Default parameters
    static const double defaultRadius = self.mapView.tileLodMinRadius;
    static const double defaultScale = self.mapView.tileLodScale;
    static const double defaultPitchThreshold = self.mapView.tileLodPitchThreshold;

    _tileLodMode = static_cast<MBXTileLodMode>((static_cast<int>(_tileLodMode) + 1) % static_cast<int>(MBXTileLodModeCount));

    switch (_tileLodMode) {
        case MBXTileLodModeDefault:
            self.mapView.tileLodMinRadius = defaultRadius;
            self.mapView.tileLodScale = defaultScale;
            self.mapView.tileLodPitchThreshold = defaultPitchThreshold;
            break;
        case MBXTileLodModeNoLod:
            // When LOD is off we set a maximum PitchThreshold
            self.mapView.tileLodPitchThreshold = M_PI;
            break;
        case MBXTileLodModeReduced:
            self.mapView.tileLodMinRadius = 2;
            self.mapView.tileLodScale = 1.5;
            self.mapView.tileLodPitchThreshold = M_PI / 4;
            break;
        case MBXTileLodModeAggressive:
            self.mapView.tileLodMinRadius = 1;
            self.mapView.tileLodScale = 2;
            self.mapView.tileLodPitchThreshold = 0;
            break;
        default:
            break;
    }

    // update UI
    static UISlider* zoomSlider = nil;
    if (zoomSlider && _tileLodMode == MBXTileLodModeNoLod) {
        [zoomSlider removeFromSuperview];
        zoomSlider = nil;
    }

    if (!zoomSlider && _tileLodMode != MBXTileLodModeNoLod) {
        const float zoomShiftRange = 5.0f;
        zoomSlider = [[UISlider alloc] init];
        zoomSlider.minimumValue = -zoomShiftRange / 2.0f;
        zoomSlider.maximumValue = zoomShiftRange / 2.0f;
        zoomSlider.value = self.mapView.tileLodZoomShift;
        zoomSlider.continuous = NO;

        [zoomSlider addTarget:self action:@selector(updateTileLodZoom:) forControlEvents:UIControlEventValueChanged];
        [self.view addSubview:zoomSlider];

        zoomSlider.translatesAutoresizingMaskIntoConstraints = NO;
        zoomSlider.frame = CGRectMake(0, 0, 100, 100);

        NSArray<NSLayoutConstraint*>* layout = @[
                [NSLayoutConstraint constraintWithItem:zoomSlider
                                             attribute:NSLayoutAttributeCenterX
                                             relatedBy:NSLayoutRelationEqual
                                                toItem:self.view
                                             attribute:NSLayoutAttributeCenterX
                                            multiplier:1
                                              constant:0],
                [NSLayoutConstraint constraintWithItem:zoomSlider
                                             attribute:NSLayoutAttributeWidth
                                             relatedBy:NSLayoutRelationEqual
                                                toItem:self.view
                                             attribute:NSLayoutAttributeWidth
                                            multiplier:0.5
                                              constant:0],
                [NSLayoutConstraint constraintWithItem:zoomSlider
                                             attribute:NSLayoutAttributeTop
                                             relatedBy:NSLayoutRelationEqual
                                                toItem:self.view
                                             attribute:NSLayoutAttributeCenterY
                                            multiplier:1
                                              constant:0],
                [zoomSlider.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor]];

        [self.view addConstraints:layout];
    }

    [self.mapView triggerRepaint];
}

-(void)updateTileLodZoom:(id)sender {
    UISlider* slider = (UISlider*)sender;

    self.mapView.tileLodZoomShift = slider.value;
    [self.mapView triggerRepaint];

    NSLog(@"Tile LOD zoom shift: %f", self.mapView.tileLodZoomShift);
}

-(NSString*)getTileLodModeName {
    switch (_tileLodMode) {
        case MBXTileLodModeDefault:
            return @"Default";
        case MBXTileLodModeNoLod:
            return @"Disabled";
        case MBXTileLodModeReduced:
            return @"Reduced";
        case MBXTileLodModeAggressive:
            return @"Aggressive";
        default:
            return @"";
    }
}

// MARK: - Random World Tour

- (void)addAnnotations:(NSInteger)numAnnotations aroundCoordinate:(CLLocationCoordinate2D)coordinate radius:(CLLocationDistance)radius {
    NSMutableArray *annotations = [[NSMutableArray alloc] initWithCapacity:numAnnotations];
    for (NSInteger i = 0; i<numAnnotations; i++) {

        CLLocationDegrees heading          = (CLLocationDegrees)arc4random_uniform(360);
        CLLocationDistance distance        = (CLLocationDistance)arc4random_uniform(radius);
        CLLocationCoordinate2D newLocation = coordinateCentered(coordinate, heading, distance);

        MBXDroppedPinAnnotation *annotation = [[MBXDroppedPinAnnotation alloc] init];
        annotation.coordinate = newLocation;
        [annotations addObject:annotation];
    }
    [self.mapView addAnnotations:annotations];
}

- (void)randomWorldTour {
    // Consistent initial conditions (consider setting these by test params)
    srand48(0);
    [self.mapView removeAnnotations:self.mapView.annotations];
    [self.mapView setCenterCoordinate:CLLocationCoordinate2DMake(31, -100) zoomLevel:3 animated:NO];

    [self randomWorldTourInternal];
}

- (void)randomWorldTourInternal {

    self.randomWalk = YES;

    // Remove all annotations
    NSTimeInterval duration = 16.0;
    __weak MBXViewController *weakSelf = self;

    // Remove old annotations, half-way through the flight.
    NSArray *annotationsToRemove = [self.mapView.annotations copy];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(duration * 0.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [weakSelf.mapView removeAnnotations:annotationsToRemove];
    });

    MBXDroppedPinAnnotation *annotation = [[MBXDroppedPinAnnotation alloc] init];
    annotation.coordinate = randomWorldCoordinate();
    [self.mapView addAnnotation:annotation];

    // Add annotations around that coord
    [self addAnnotations:50 aroundCoordinate:annotation.coordinate radius:100000]; // 100km

    MLNMapCamera *camera = [MLNMapCamera cameraLookingAtCenterCoordinate:annotation.coordinate
                                                                altitude:10000.0
                                                                   pitch:(CLLocationDegrees)arc4random_uniform(60)
                                                                 heading:(CLLocationDegrees)arc4random_uniform(360)];
    [self.mapView flyToCamera:camera
                 withDuration:duration
                 peakAltitude:2000000.0
            completionHandler:^{
                // This completion handler is currently called BEFORE the
                // region did change delegate method, and we don't have a "reason"
                // so we can't tell if the motion was cancelled. We use the delegate
                // for that, and set self.randomWalk. But since we want a delay
                // anyway, we can just check later. Not ideal though..
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                    MBXViewController *strongSelf = weakSelf;
                    if (strongSelf.randomWalk) {
                        [strongSelf randomWorldTourInternal];
                    }
                });
            }];
}

- (void)toggleSecondMapView {
    if ([self.view viewWithTag:2] == nil) {
        MLNMapView *secondMapView = [[MLNMapView alloc] initWithFrame:
                                     CGRectMake(0, self.view.bounds.size.height / 2,
                                                self.view.bounds.size.width, self.view.bounds.size.height / 2)];
        secondMapView.showsScale = YES;
        secondMapView.translatesAutoresizingMaskIntoConstraints = NO;
        secondMapView.tag = 2;

        // Remove the main map bottom constraint
        for (NSLayoutConstraint *constraint in self.view.constraints)
        {
            if ((constraint.firstItem  == self.mapView && constraint.firstAttribute  == NSLayoutAttributeBottom) ||
                (constraint.secondItem == self.mapView && constraint.secondAttribute == NSLayoutAttributeBottom))
            {
                _firstMapLayout = constraint;
                [self.view removeConstraint:constraint];
                break;
            }
        }

        // Place the second map in the bottom half of the view
        _secondMapLayout = @[
                [NSLayoutConstraint constraintWithItem:self.mapView
                                             attribute:NSLayoutAttributeBottom
                                             relatedBy:NSLayoutRelationEqual
                                                toItem:self.view
                                             attribute:NSLayoutAttributeCenterY
                                            multiplier:1
                                              constant:0],
                [NSLayoutConstraint constraintWithItem:secondMapView
                                             attribute:NSLayoutAttributeCenterX
                                             relatedBy:NSLayoutRelationEqual
                                                toItem:self.view
                                             attribute:NSLayoutAttributeCenterX
                                            multiplier:1
                                              constant:0],
                [NSLayoutConstraint constraintWithItem:secondMapView
                                             attribute:NSLayoutAttributeWidth
                                             relatedBy:NSLayoutRelationEqual
                                                toItem:self.view
                                             attribute:NSLayoutAttributeWidth
                                            multiplier:1
                                              constant:0],
                [NSLayoutConstraint constraintWithItem:secondMapView
                                             attribute:NSLayoutAttributeTop
                                             relatedBy:NSLayoutRelationEqual
                                                toItem:self.view
                                             attribute:NSLayoutAttributeCenterY
                                            multiplier:1
                                              constant:0],
            [secondMapView.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor]];

        [self.view addSubview:secondMapView];
        [self.view addConstraints:_secondMapLayout];

        secondMapView.styleURL = _mapView.styleURL;

        __weak decltype(_mapView) weakMapView = _mapView;
        __weak decltype(secondMapView) weakSecondMap = secondMapView;
        auto moveComplete = ^{
            weakSecondMap.camera = [_mapView cameraByTiltingToPitch:weakMapView.camera.pitch];
        };

        // Navigate the new map to the same view as the main one
        [secondMapView setCenterCoordinate:_mapView.centerCoordinate
                                 zoomLevel:_mapView.zoomLevel
                                 direction:_mapView.direction
                                  animated:YES
                         completionHandler:moveComplete];

        secondMapView.accessibilityIdentifier = @"Second Map";
    } else {
        MLNMapView *secondMapView = (MLNMapView *)[self.view viewWithTag:2];

        // Reset the layout to the original state
        [self.view removeConstraints:_secondMapLayout];
        [self.view addConstraint:_firstMapLayout];
        _firstMapLayout = nil;
        _secondMapLayout = nil;

        [secondMapView removeFromSuperview];
    }
}

// MARK: - User Actions

- (void)handleSingleTap:(UITapGestureRecognizer *)singleTap {
    [self.navigationController setNavigationBarHidden:!self.navigationController.navigationBarHidden animated:YES];

    // This is how you'd get the coordinate for the point where the user tapped:
    //    CGPoint tapPoint = [singleTap locationInView:self.mapView];
    //    CLLocationCoordinate2D tapCoordinate = [self.mapView convertPoint:tapPoint toCoordinateFromView:nil];
}

- (IBAction)handleLongPress:(UILongPressGestureRecognizer *)longPress
{
    if (longPress.state == UIGestureRecognizerStateBegan)
    {
        CGPoint point = [longPress locationInView:longPress.view];
        NSArray *features = [self.mapView visibleFeaturesAtPoint:point];
        NSString *title;
        for (id <MLNFeature> feature in features) {
            if (!title) {
                title = [feature attributeForKey:@"name_en"] ?: [feature attributeForKey:@"name"];
            }
        }

        MBXDroppedPinAnnotation *pin = [[MBXDroppedPinAnnotation alloc] init];
        pin.coordinate = [self.mapView convertPoint:point
                                 toCoordinateFromView:self.mapView];
        pin.title = title ?: @"Dropped Pin";
        pin.subtitle = [[[MLNCoordinateFormatter alloc] init] stringFromCoordinate:pin.coordinate];


        // Calling `addAnnotation:` on mapView is required here (since `selectAnnotation:animated` has
        // the side effect of adding the annotation if required, but returning an incorrect callout
        // positioning rect)

        [self.mapView addAnnotation:pin];
        [self.mapView selectAnnotation:pin animated:YES completionHandler:nil];
    }
}

// MARK: - User defined Styles
- (void)setStyles
{
    self.styleNames = [NSMutableArray array];
    self.styleURLs = [NSMutableArray array];




    /// Style that does not require an `apiKey` nor any further configuration
    [self.styleNames addObject:@"MapLibre Basic"];
    [self.styleURLs addObject:[NSURL URLWithString:@"https://demotiles.maplibre.org/style.json"]];

    /// This is hte same style as above but copied locally and the three instances of the metal plug-in layer added to the style
    /// Look for "type": "plugin-layer-metal-rendering" in the PluginLayerTestStyle.json for an example of how the layer is defined
    [self.styleNames addObject:@"MapLibre Basic - Local With Plugin"];
    NSURL *url = [[NSBundle mainBundle] URLForResource:@"PluginLayerTestStyle.json" withExtension:nil];
    [self.styleURLs addObject:url];

    /// Add MapLibre Styles if an `apiKey` exists
    NSString* apiKey = [MLNSettings apiKey];
    if (apiKey.length)
    {
        static dispatch_once_t onceToken;
        dispatch_once(&onceToken, ^{

            for (MLNDefaultStyle* predefinedStyle in [MLNStyle predefinedStyles]){
                [self.styleNames addObject:predefinedStyle.name];
                [self.styleURLs addObject:predefinedStyle.url];
            }
        });
    }

    NSAssert(self.styleNames.count == self.styleURLs.count, @"Style names and URLs don’t match.");
}

- (IBAction)cycleStyles:(__unused id)sender
{
    self.styleIndex = (self.styleIndex + 1) % self.styleNames.count;

    self.mapView.styleURL = self.styleURLs[self.styleIndex];

    UIButton *titleButton = (UIButton *)self.navigationItem.titleView;
    [titleButton setTitle:self.styleNames[self.styleIndex] forState:UIControlStateNormal];
}

- (IBAction)locateUser:(id)sender
{
    [self nextTrackingMode:sender];
}


- (void)nextTrackingMode:(id)sender
{
    MLNUserTrackingMode nextMode;
    NSString *nextAccessibilityValue;
    switch (self.mapView.userTrackingMode) {
        case MLNUserTrackingModeNone:
            nextMode = MLNUserTrackingModeFollow;
            nextAccessibilityValue = @"Follow location";
            break;
        case MLNUserTrackingModeFollow:
            nextMode = MLNUserTrackingModeFollowWithHeading;
            nextAccessibilityValue = @"Follow location and heading";
            break;
        case MLNUserTrackingModeFollowWithHeading:
            nextMode = MLNUserTrackingModeFollowWithCourse;
            nextAccessibilityValue = @"Follow course";
            break;
        case MLNUserTrackingModeFollowWithCourse:
            nextMode = MLNUserTrackingModeNone;
            nextAccessibilityValue = @"Off";
            break;
    }
    self.mapView.userTrackingMode = nextMode;
    [sender setAccessibilityValue:nextAccessibilityValue];
}

// MARK: - UIViewDelegate

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
    if (_contentInsetsEnabled)
    {
        _contentInsetsEnabled = NO;
        _mapView.automaticallyAdjustsContentInset = YES;
        [self.mapView setContentInset:UIEdgeInsetsZero];
    }
    while (self.contentInsetsOverlays && [self.contentInsetsOverlays count]) {
        [[self.contentInsetsOverlays lastObject] removeFromSuperview];
        [self.contentInsetsOverlays removeLastObject];
    }
}

// MARK: - MLNMapViewDelegate

- (void)mapView:(MLNMapView *)mapView sourceDidChange:(MLNSource *)source
{
    NSLog(@"A source was updated: %@", source.identifier);
}

- (void)mapView:(MLNMapView *)mapView
    shaderWillCompile:(NSInteger)id
              backend:(NSInteger)backend
              defines:(NSString *)defines
{
    NSLog(@"A new shader is being compiled - shaderID:%ld, backend type:%ld, program configuration:%@", id, backend, defines);
}

- (void)mapView:(MLNMapView *)mapView
    shaderDidCompile:(NSInteger)id
             backend:(NSInteger)backend
             defines:(NSString *)defines
{
    NSLog(@"A shader has been compiled - shaderID:%ld, backend type:%ld, program configuration:%@", id, backend, defines);
}

- (void)mapView:(MLNMapView *)mapView
    glyphsWillLoad:(NSArray<NSString *> *)fontStack
             range:(NSRange)range
{
    NSLog(@"Glyphs are being requested for the font stack %@, ranging from %ld to %ld", fontStack, range.location, range.location + range.length);
}

- (void)mapView:(MLNMapView *)mapView
    glyphsDidLoad:(NSArray<NSString *> *)fontStack
            range:(NSRange)range
{
    NSLog(@"Glyphs have been loaded for the font stack %@, ranging from %ld to %ld", fontStack, range.location, range.location + range.length);
}

- (void)mapView:(MLNMapView *)mapView
    tileDidTriggerAction:(MLNTileOperation)operation
                       x:(NSInteger)x
                       y:(NSInteger)y
                       z:(NSInteger)z
                    wrap:(NSInteger)wrap
             overscaledZ:(NSInteger)overscaledZ
                sourceID:(NSString *)sourceID
{
    NSString* tileStr = [NSString stringWithFormat:@"(x: %ld, y: %ld, z: %ld, wrap: %ld, overscaledZ: %ld, sourceID: %@)",
                         x, y, z, wrap, overscaledZ, sourceID];

    switch (operation) {
        case MLNTileOperationRequestedFromCache:
            NSLog(@"Requesting tile %@ from cache", tileStr);
            break;

        case MLNTileOperationRequestedFromNetwork:
            NSLog(@"Requesting tile %@ from network", tileStr);
            break;

        case MLNTileOperationLoadFromCache:
            NSLog(@"Loading tile %@, requested from the cache", tileStr);
            break;

        case MLNTileOperationLoadFromNetwork:
            NSLog(@"Loading tile %@, requested from the network", tileStr);
            break;

        case MLNTileOperationStartParse:
            NSLog(@"Parsing tile %@", tileStr);
            break;

        case MLNTileOperationEndParse:
            NSLog(@"Completed parsing tile %@", tileStr);
            break;

        case MLNTileOperationError:
            NSLog(@"An error occured during proccessing for tile %@", tileStr);
            break;

        case MLNTileOperationCancelled:
            NSLog(@"Pending work on tile %@", tileStr);
            break;

        case MLNTileOperationNullOp:
            NSLog(@"An unknown tile operation was emitted for tile %@", tileStr);
            break;
    }
}

- (void)mapView:(MLNMapView *)mapView spriteWillLoad:(NSString *)id url:(NSString *)url
{
    NSLog(@"The sprite %@ has been requested from %@", id, url);
}

- (void)mapView:(MLNMapView *)mapView spriteDidLoad:(NSString *)id url:(NSString *)url
{
    NSLog(@"The sprite %@ has been loaded from %@", id, url);
}

- (MLNAnnotationView *)mapView:(MLNMapView *)mapView viewForAnnotation:(id<MLNAnnotation>)annotation
{
    if (annotation == mapView.userLocation)
    {
        if (_customUserLocationAnnnotationEnabled)
        {
            MBXUserLocationAnnotationView *annotationView = [[MBXUserLocationAnnotationView alloc] initWithFrame:CGRectZero];
            annotationView.frame = CGRectMake(0, 0, annotationView.intrinsicContentSize.width, annotationView.intrinsicContentSize.height);
            return annotationView;
        }

        return nil;
    }
    // Use GL backed pins for dropped pin annotations
    if ([annotation isKindOfClass:[MBXDroppedPinAnnotation class]] || [annotation isKindOfClass:[MBXSpriteBackedAnnotation class]])
    {
        return nil;
    }

    MBXAnnotationView *annotationView = (MBXAnnotationView *)[mapView dequeueReusableAnnotationViewWithIdentifier:MBXViewControllerAnnotationViewReuseIdentifer];
    if (!annotationView)
    {
        annotationView = [[MBXAnnotationView alloc] initWithReuseIdentifier:MBXViewControllerAnnotationViewReuseIdentifer];
        annotationView.frame = CGRectMake(0, 0, 10, 10);
        annotationView.backgroundColor = [UIColor whiteColor];

        // Note that having two long press gesture recognizers on overlapping
        // views (`self.view` & `annotationView`) will cause weird behavior.
        // Comment out the pin dropping functionality in the handleLongPress:
        // method in this class to make draggable annotation views play nice.
        annotationView.draggable = YES;
    } else {
        // orange indicates that the annotation view was reused
        annotationView.backgroundColor = [UIColor orangeColor];
    }
    return annotationView;
}

- (MLNAnnotationImage *)mapView:(MLNMapView * __nonnull)mapView imageForAnnotation:(id <MLNAnnotation> __nonnull)annotation
{
    if ([annotation isKindOfClass:[MBXDroppedPinAnnotation class]] || [annotation isKindOfClass:[MBXCustomCalloutAnnotation class]])
    {
        return nil; // use default marker
    }

    NSAssert([annotation isKindOfClass:[MBXSpriteBackedAnnotation class]], @"Annotations should be sprite-backed.");

    NSString *title = [(MLNPointAnnotation *)annotation title];
    if (!title.length) return nil;
    NSString *lastTwoCharacters = [title substringFromIndex:title.length - 2];

    MLNAnnotationImage *annotationImage = [mapView dequeueReusableAnnotationImageWithIdentifier:lastTwoCharacters];

    if ( ! annotationImage)
    {
        UIColor *color;

        // make every tenth annotation blue
        if ([lastTwoCharacters hasSuffix:@"0"]) {
            color = [UIColor blueColor];
        } else {
            color = [UIColor redColor];
        }

        UIImage *image = [self imageWithText:lastTwoCharacters backgroundColor:color];
        annotationImage = [MLNAnnotationImage annotationImageWithImage:image reuseIdentifier:lastTwoCharacters];

        // don't allow touches on blue annotations
        if ([color isEqual:[UIColor blueColor]]) annotationImage.enabled = NO;
    }

    return annotationImage;
}


- (UIImage *)imageWithText:(NSString *)text backgroundColor:(UIColor *)color
{
    CGRect rect = CGRectMake(0, 0, 20, 15);

    UIGraphicsBeginImageContextWithOptions(rect.size, NO, [[UIScreen mainScreen] scale]);

    CGContextRef ctx = UIGraphicsGetCurrentContext();

    CGContextSetFillColorWithColor(ctx, [[color colorWithAlphaComponent:0.75] CGColor]);
    CGContextFillRect(ctx, rect);

    CGContextSetStrokeColorWithColor(ctx, [[UIColor blackColor] CGColor]);
    CGContextStrokeRectWithWidth(ctx, rect, 2);

    NSAttributedString *drawString = [[NSAttributedString alloc] initWithString:text attributes:@{
        NSFontAttributeName: [UIFont fontWithName:@"Arial-BoldMT" size:12],
        NSForegroundColorAttributeName: [UIColor whiteColor],
    }];
    CGSize stringSize = drawString.size;
    CGRect stringRect = CGRectMake((rect.size.width - stringSize.width) / 2,
                                   (rect.size.height - stringSize.height) / 2,
                                   stringSize.width,
                                   stringSize.height);
    [drawString drawInRect:stringRect];

    UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return image;
}

- (BOOL)mapView:(__unused MLNMapView *)mapView annotationCanShowCallout:(__unused id <MLNAnnotation>)annotation
{
    return YES;
}

- (CGFloat)mapView:(__unused MLNMapView *)mapView alphaForShapeAnnotation:(MLNShape *)annotation
{
    return ([annotation isKindOfClass:[MLNPolygon class]] ? 0.5 : 1.0);
}

- (UIColor *)mapView:(__unused MLNMapView *)mapView strokeColorForShapeAnnotation:(MLNShape *)annotation
{
    UIColor *color = [annotation isKindOfClass:[MLNPolyline class]] ? [UIColor greenColor] : [UIColor blackColor];
    return [color colorWithAlphaComponent:0.9];
}

- (UIColor *)mapView:(__unused MLNMapView *)mapView fillColorForPolygonAnnotation:(__unused MLNPolygon *)annotation
{
    UIColor *color = annotation.pointCount > 3 ? [UIColor greenColor] : [UIColor redColor];
    return [color colorWithAlphaComponent:0.5];
}

- (void)mapView:(__unused MLNMapView *)mapView didChangeUserTrackingMode:(MLNUserTrackingMode)mode animated:(__unused BOOL)animated
{
    UIImage *newButtonImage;
    NSString *newButtonTitle;

    switch (mode) {
        case MLNUserTrackingModeNone:
            newButtonImage = [UIImage imageNamed:@"TrackingLocationOffMask.png"];
            break;

        case MLNUserTrackingModeFollow:
            newButtonImage = [UIImage imageNamed:@"TrackingLocationMask.png"];
            break;

        case MLNUserTrackingModeFollowWithHeading:
            newButtonImage = [UIImage imageNamed:@"TrackingHeadingMask.png"];
            break;
        case MLNUserTrackingModeFollowWithCourse:
            newButtonImage = nil;
            newButtonTitle = @"Course";
            break;
    }

    self.navigationItem.rightBarButtonItem.title = newButtonTitle;
    [UIView animateWithDuration:0.25 animations:^{
        self.navigationItem.rightBarButtonItem.image = newButtonImage;
    }];
}

- (nullable id <MLNCalloutView>)mapView:(__unused MLNMapView *)mapView calloutViewForAnnotation:(id<MLNAnnotation>)annotation
{
    if ([annotation respondsToSelector:@selector(title)]
        && [annotation isKindOfClass:[MBXCustomCalloutAnnotation class]])
    {
        MBXCustomCalloutAnnotation *customAnnotation = (MBXCustomCalloutAnnotation *)annotation;
        MBXCustomCalloutView *calloutView = [[MBXCustomCalloutView alloc] init];
        calloutView.representedObject = annotation;
        calloutView.anchoredToAnnotation = customAnnotation.anchoredToAnnotation;
        calloutView.dismissesAutomatically = customAnnotation.dismissesAutomatically;
        return calloutView;
    }
    return nil;
}

- (UIView *)mapView:(__unused MLNMapView *)mapView leftCalloutAccessoryViewForAnnotation:(__unused id<MLNAnnotation>)annotation
{
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
    button.frame = CGRectZero;
    [button setTitle:@"Left" forState:UIControlStateNormal];
    [button sizeToFit];
    return button;
}

- (UIView *)mapView:(__unused MLNMapView *)mapView rightCalloutAccessoryViewForAnnotation:(__unused id<MLNAnnotation>)annotation
{
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
    button.frame = CGRectZero;
    [button setTitle:@"Right" forState:UIControlStateNormal];
    [button sizeToFit];
    return button;
}

- (void)mapView:(MLNMapView *)mapView tapOnCalloutForAnnotation:(id <MLNAnnotation>)annotation
{
    if ( ! [annotation isKindOfClass:[MLNPointAnnotation class]])
    {
        return;
    }

    MLNPointAnnotation *point = (MLNPointAnnotation *)annotation;
    point.coordinate = [self.mapView convertPoint:self.mapView.center toCoordinateFromView:self.mapView];
}

- (void)mapView:(MLNMapView *)mapView didFinishLoadingStyle:(MLNStyle *)style
{
    // Default MapLibre styles use {name_en} as their label language, which means
    // that a device with an English-language locale is already effectively
    // using locale-based country labels.
    _localizingLabels = [[self bestLanguageForUser] isEqualToString:@"en"];
}

- (BOOL)mapView:(MLNMapView *)mapView shouldChangeFromCamera:(MLNMapCamera *)oldCamera toCamera:(MLNMapCamera *)newCamera {
    if (_shouldLimitCameraChanges) {
        // Get the current camera to restore it after.
        MLNMapCamera *currentCamera = mapView.camera;

        // From the new camera obtain the center to test if it’s inside the boundaries.
        CLLocationCoordinate2D newCameraCenter = newCamera.centerCoordinate;

        // Set the map’s visible bounds to newCamera.
        mapView.camera = newCamera;
        MLNCoordinateBounds newVisibleCoordinates = mapView.visibleCoordinateBounds;

        // Revert the camera.
        mapView.camera = currentCamera;

        // Test if the newCameraCenter and newVisibleCoordinates are inside Colorado.
        BOOL inside = MLNCoordinateInCoordinateBounds(newCameraCenter, colorado);
        BOOL intersects = MLNCoordinateInCoordinateBounds(newVisibleCoordinates.ne, colorado) && MLNCoordinateInCoordinateBounds(newVisibleCoordinates.sw, colorado);

        return inside && intersects;
    } else {
        return YES;
    }
}

- (void)mapViewRegionIsChanging:(MLNMapView *)mapView
{
    [self updateHUD];
    [self updateHelperMapViews];
}

- (void)mapView:(MLNMapView *)mapView regionDidChangeWithReason:(MLNCameraChangeReason)reason animated:(BOOL)animated
{
    if (reason != MLNCameraChangeReasonProgrammatic) {
        self.randomWalk = NO;
    }

    [self updateHUD];
    [self updateHelperMapViews];
}

- (void)mapView:(MLNMapView *)mapView didUpdateUserLocation:(MLNUserLocation *)userLocation {
    [self updateHUD];
}

- (void)updateHelperMapViews {
    for (UIWindow *window in self.helperWindows) {
        MLNMapView *mapView = (MLNMapView *)window.rootViewController.view;
        mapView.camera = self.mapView.camera;
    }
}

- (void)updateHUD {

    if (self.reuseQueueStatsEnabled == NO && self.zoomLevelOrnamentEnabled == NO) {
        return;
    }

    NSString *hudString;

    if (self.reuseQueueStatsEnabled) {
        NSUInteger queuedAnnotations = 0;
        for (NSArray *queue in self.mapView.annotationViewReuseQueueByIdentifier.allValues) {
            queuedAnnotations += queue.count;
        }
        hudString = [NSString stringWithFormat:@"Visible: %ld  Queued: %ld", (unsigned long)self.mapView.visibleAnnotations.count, (unsigned long)queuedAnnotations];
    } else if (self.zoomLevelOrnamentEnabled) {
        hudString = [NSString stringWithFormat:@"%.f FPS (%.1fms) ∕ %.2f ∕ ↕\U0000FE0E%.f° ∕ %.f°",
                     roundf(self.mapView.averageFrameRate), self.mapView.averageFrameTime,
                     self.mapView.zoomLevel, self.mapView.camera.pitch, self.mapView.direction];
    }

    [self.hudLabel setTitle:hudString forState:UIControlStateNormal];
}

// MARK: - MLNComputedShapeSourceDataSource

- (NSArray<id <MLNFeature>>*)featuresInCoordinateBounds:(MLNCoordinateBounds)bounds zoomLevel:(NSUInteger)zoom {
    double gridSpacing;
    if(zoom >= 13) {
        gridSpacing = 0.01;
    } else if(zoom >= 11) {
        gridSpacing = 0.05;
    } else if(zoom == 10) {
        gridSpacing = .1;
    } else if(zoom == 9) {
        gridSpacing = 0.25;
    } else if(zoom == 8) {
        gridSpacing = 0.5;
    } else if (zoom >= 6) {
        gridSpacing = 1;
    } else if(zoom == 5) {
        gridSpacing = 2;
    } else if(zoom >= 4) {
        gridSpacing = 5;
    } else if(zoom == 2) {
        gridSpacing = 10;
    } else {
        gridSpacing = 20;
    }

    NSMutableArray <id <MLNFeature>> * features = [NSMutableArray array];
    CLLocationCoordinate2D coords[2];

    for (double y = ceil(bounds.ne.latitude / gridSpacing) * gridSpacing; y >= floor(bounds.sw.latitude / gridSpacing) * gridSpacing; y -= gridSpacing) {
        coords[0] = CLLocationCoordinate2DMake(y, bounds.sw.longitude);
        coords[1] = CLLocationCoordinate2DMake(y, bounds.ne.longitude);
        MLNPolylineFeature *feature = [MLNPolylineFeature polylineWithCoordinates:coords count:2];
        feature.attributes = @{@"value": @(y)};
        [features addObject:feature];
    }

    for (double x = floor(bounds.sw.longitude / gridSpacing) * gridSpacing; x <= ceil(bounds.ne.longitude / gridSpacing) * gridSpacing; x += gridSpacing) {
        coords[0] = CLLocationCoordinate2DMake(bounds.sw.latitude, x);
        coords[1] = CLLocationCoordinate2DMake(bounds.ne.latitude, x);
        MLNPolylineFeature *feature = [MLNPolylineFeature polylineWithCoordinates:coords count:2];
        feature.attributes = @{@"value": @(x)};
        [features addObject:feature];
    }

    return features;
}

- (void)mapViewDidFinishRenderingFrame:(MLNMapView *)mapView
                         fullyRendered:(BOOL)fullyRendered
                        renderingStats:(nonnull MLNRenderingStats *)renderingStats {
    if (self.frameTimeGraphEnabled) {
        [self.frameTimeGraphView updatePathWithFrameDuration:renderingStats.encodingTime];
    }
}

- (void)mapView:(nonnull MLNMapView *)mapView didChangeLocationManagerAuthorization:(nonnull id<MLNLocationManager>)manager {
    if (@available(iOS 14, *)) {
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 140000
        if (manager.authorizationStatus == kCLAuthorizationStatusDenied || manager.accuracyAuthorization == CLAccuracyAuthorizationReducedAccuracy) {
            [self alertAccuracyChanges];
        }
#endif
    }
}

- (void)alertAccuracyChanges {
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"MapLibre works best with your precise location."
                                   message:@"You'll get turn-by-turn directions."
                                   preferredStyle:UIAlertControllerStyleAlert];

    UIAlertAction *settingsAction = [UIAlertAction actionWithTitle:@"Turn On in Settings" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        [[UIApplication sharedApplication] openURL:[NSURL URLWithString:UIApplicationOpenSettingsURLString] options:@{} completionHandler:^(BOOL success) {
            if (success) {
                 NSLog(@"Opened url");
            }
        }];
    }];

    UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"Keep Precise Location Off" style:UIAlertActionStyleDefault
       handler:nil];

    [alert addAction:settingsAction];
    [alert addAction:defaultAction];
    [self presentViewController:alert animated:YES completion:nil];
}

- (BOOL)isUITesting {
    NSString* value = NSProcessInfo.processInfo.environment[@"UITesting"];
    return value && [value isEqual: @"YES"];
}

- (void)saveCurrentMapState:(__unused NSNotification *)notification {

    // saved changes to the settings can break UI tests, so always start from the defaults when testing
    if ([self isUITesting]) {
        return;
    }

    // The following properties can change after the view loads so we need to save their
    // state before exiting the view controller.
    self.currentState.camera = self.mapView.camera;
    self.currentState.showsUserLocation = self.mapView.showsUserLocation;
    self.currentState.userTrackingMode = self.mapView.userTrackingMode;
    self.currentState.showsUserHeadingIndicator = self.mapView.showsUserHeadingIndicator;
    self.currentState.showsMapScale = self.mapView.showsScale;
    self.currentState.showsZoomLevelOrnament = self.zoomLevelOrnamentEnabled;
    self.currentState.showsTimeFrameGraph = self.frameTimeGraphEnabled;
    self.currentState.debugMask = self.mapView.debugMask;
    self.currentState.reuseQueueStatsEnabled = self.reuseQueueStatsEnabled;

    [[MBXStateManager sharedManager] saveState:self.currentState];
}

- (void)restoreMapState:(__unused NSNotification *)notification {

    if ([self isUITesting]) {
        return;
    }

    MBXState *currentState = [MBXStateManager sharedManager].currentState;

    self.mapView.camera = currentState.camera;
    self.mapView.showsUserLocation = currentState.showsUserLocation;
    self.mapView.userTrackingMode = currentState.userTrackingMode;
    self.mapView.showsUserHeadingIndicator = currentState.showsUserHeadingIndicator;
    self.mapView.showsScale = currentState.showsMapScale;
    self.zoomLevelOrnamentEnabled = currentState.showsZoomLevelOrnament;
    self.frameTimeGraphEnabled = currentState.showsTimeFrameGraph;
    self.mapView.debugMask = currentState.debugMask;
    self.reuseQueueStatsEnabled = currentState.reuseQueueStatsEnabled;

    self.currentState = currentState;
}

@end
