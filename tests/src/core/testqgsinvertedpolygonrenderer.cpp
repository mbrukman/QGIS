/***************************************************************************
     testqgsinvertedpolygonrenderer.cpp
     --------------------------------------
    Date                 : 23 may 2014
    Copyright            : (C) 2014 by Hugo Mercier / Oslandia
    Email                : hugo dot mercier at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>

//qgis includes...
#include <qgsmaplayer.h>
#include <qgsvectorlayer.h>
#include <qgsapplication.h>
#include <qgsproviderregistry.h>
#include <qgsmaplayerregistry.h>
//qgis test includes
#include "qgsmultirenderchecker.h"

/** \ingroup UnitTests
 * This is a unit test for the different renderers for vector layers.
 */
class TestQgsInvertedPolygon : public QObject
{
    Q_OBJECT

  public:
    TestQgsInvertedPolygon();

  private slots:
    void initTestCase();// will be called before the first testfunction is executed.
    void cleanupTestCase();// will be called after the last testfunction was executed.
    void init() {} // will be called before each testfunction is executed.
    void cleanup() {} // will be called after every testfunction.

    void singleSubRenderer();
    void graduatedSubRenderer();
    void preprocess();
    void projectionTest();
#if defined(GDAL_VERSION_NUM) && GDAL_VERSION_MAJOR >= 2
    void curvedPolygons();
#endif

  private:
    bool mTestHasError;
    bool setQml( QgsVectorLayer* vlayer, const QString& qmlFile );
    bool imageCheck( const QString& theType, const QgsRectangle* = 0 );
    QgsMapSettings mMapSettings;
    QgsVectorLayer * mpPolysLayer;
    QString mTestDataDir;
    QString mReport;
};


TestQgsInvertedPolygon::TestQgsInvertedPolygon()
    : mTestHasError( false )
    , mpPolysLayer( nullptr )
{

}

void TestQgsInvertedPolygon::initTestCase()
{
  mTestHasError = false;
  // init QGIS's paths - true means that all path will be inited from prefix
  QgsApplication::init();
  QgsApplication::initQgis();
  QgsApplication::showSettings();

  QString myDataDir( TEST_DATA_DIR ); //defined in CmakeLists.txt
  mTestDataDir = myDataDir + '/';

  //
  //create a poly layer that will be used in all tests...
  //
  QString myPolysFileName = mTestDataDir + "polys_overlapping.shp";
  QFileInfo myPolyFileInfo( myPolysFileName );
  mpPolysLayer = new QgsVectorLayer( myPolyFileInfo.filePath(),
                                     myPolyFileInfo.completeBaseName(), QStringLiteral( "ogr" ) );
  QgsVectorSimplifyMethod simplifyMethod;
  simplifyMethod.setSimplifyHints( QgsVectorSimplifyMethod::NoSimplification );
  mpPolysLayer->setSimplifyMethod( simplifyMethod );

  // Register the layer with the registry
  QgsMapLayerRegistry::instance()->addMapLayers( QList<QgsMapLayer *>() << mpPolysLayer );

  mMapSettings.setLayers( QStringList() << mpPolysLayer->id() );
  mReport += QLatin1String( "<h1>Inverted Polygon Renderer Tests</h1>\n" );
}

void TestQgsInvertedPolygon::cleanupTestCase()
{
  QString myReportFile = QDir::tempPath() + "/qgistest.html";
  QFile myFile( myReportFile );
  if ( myFile.open( QIODevice::WriteOnly | QIODevice::Append ) )
  {
    QTextStream myQTextStream( &myFile );
    myQTextStream << mReport;
    myFile.close();
  }

  QgsApplication::exitQgis();
}

void TestQgsInvertedPolygon::singleSubRenderer()
{
  mReport += QLatin1String( "<h2>Inverted polygon renderer, single sub renderer test</h2>\n" );
  QVERIFY( setQml( mpPolysLayer, "inverted_polys_single.qml" ) );
  QVERIFY( imageCheck( "inverted_polys_single" ) );
}

void TestQgsInvertedPolygon::graduatedSubRenderer()
{
  mReport += QLatin1String( "<h2>Inverted polygon renderer, graduated sub renderer test</h2>\n" );
  QVERIFY( setQml( mpPolysLayer, "inverted_polys_graduated.qml" ) );
  QVERIFY( imageCheck( "inverted_polys_graduated" ) );
}

void TestQgsInvertedPolygon::preprocess()
{
  // FIXME will have to find some overlapping polygons
  mReport += QLatin1String( "<h2>Inverted polygon renderer, preprocessing test</h2>\n" );
  QVERIFY( setQml( mpPolysLayer, "inverted_polys_preprocess.qml" ) );
  QVERIFY( imageCheck( "inverted_polys_preprocess" ) );
}

void TestQgsInvertedPolygon::projectionTest()
{
  mReport += QLatin1String( "<h2>Inverted polygon renderer, projection test</h2>\n" );
  mMapSettings.setDestinationCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:2154" ) ) );
  mMapSettings.setCrsTransformEnabled( true );
  QgsRectangle extent( QgsPoint( -8639421, 8382691 ), QgsPoint( -3969110, 12570905 ) );
  QVERIFY( setQml( mpPolysLayer, "inverted_polys_single.qml" ) );
  QVERIFY( imageCheck( "inverted_polys_projection", &extent ) );
  QVERIFY( setQml( mpPolysLayer, "inverted_polys_preprocess.qml" ) );
  QVERIFY( imageCheck( "inverted_polys_projection2", &extent ) );
  mMapSettings.setCrsTransformEnabled( false );
}

#if defined(GDAL_VERSION_NUM) && GDAL_VERSION_MAJOR >= 2
// This test relies on GDAL support of curved polygons
void TestQgsInvertedPolygon::curvedPolygons()
{
  QString myCurvedPolysFileName = mTestDataDir + "curved_polys.gpkg";
  QFileInfo myCurvedPolyFileInfo( myCurvedPolysFileName );
  QgsVectorLayer* curvedLayer = new QgsVectorLayer( myCurvedPolyFileInfo.filePath() + "|layername=polys",
      myCurvedPolyFileInfo.completeBaseName(), "ogr" );
  curvedLayer->setSimplifyMethod( simplifyMethod );
  QgsMapLayerRegistry::instance()->addMapLayers( QList<QgsMapLayer *>() << curvedLayer );

  mReport += "<h2>Inverted polygon renderer, curved polygons test</h2>\n";
  mMapSettings.setLayers( QStringList() << curvedLayer->id() );
  QVERIFY( setQml( mpCurvedPolysLayer, "inverted_polys_single.qml" ) );
  QVERIFY( imageCheck( "inverted_polys_curved" ) );
  mMapSettings.setLayers( QStringList() << curvedLayer->id() );
}
#endif

//
// Private helper functions not called directly by CTest
//

bool TestQgsInvertedPolygon::setQml( QgsVectorLayer* vlayer, const QString& qmlFile )
{
  //load a qml style and apply to our layer
  //the style will correspond to the renderer
  //type we are testing
  bool myStyleFlag = false;
  QString myFileName = mTestDataDir + qmlFile;
  QString error = vlayer->loadNamedStyle( myFileName, myStyleFlag );
  if ( !myStyleFlag )
  {
    qDebug( "%s", error.toLocal8Bit().constData() );
    return false;
  }
  return myStyleFlag;
}

bool TestQgsInvertedPolygon::imageCheck( const QString& theTestType, const QgsRectangle* extent )
{
  //use the QgsRenderChecker test utility class to
  //ensure the rendered output matches our control image
  if ( !extent )
  {
    mMapSettings.setExtent( mpPolysLayer->extent() );
  }
  else
  {
    mMapSettings.setExtent( *extent );
  }
  mMapSettings.setOutputDpi( 96 );
  QgsMultiRenderChecker myChecker;
  myChecker.setControlPathPrefix( QStringLiteral( "symbol_invertedpolygon" ) );
  myChecker.setControlName( "expected_" + theTestType );
  myChecker.setMapSettings( mMapSettings );
  myChecker.setColorTolerance( 20 );
  bool myResultFlag = myChecker.runTest( theTestType, 100 );
  mReport += myChecker.report();
  return myResultFlag;
}

QTEST_MAIN( TestQgsInvertedPolygon )
#include "testqgsinvertedpolygonrenderer.moc"
