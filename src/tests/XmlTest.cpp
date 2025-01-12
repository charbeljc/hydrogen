/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "XmlTest.h"

#include <unistd.h>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/Sample.h>

#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include "TestHelper.h"

CPPUNIT_TEST_SUITE_REGISTRATION( XmlTest );


static bool check_samples_data( H2Core::Drumkit* dk, bool loaded )
{
	int count = 0;
	H2Core::InstrumentComponent::setMaxLayers( 16 );
	H2Core::InstrumentList* instruments = dk->get_instruments();
	for( int i=0; i<instruments->size(); i++ ) {
		count++;
		H2Core::Instrument* pInstr = ( *instruments )[i];
		for (std::vector<H2Core::InstrumentComponent*>::iterator it = pInstr->get_components()->begin() ; it != pInstr->get_components()->end(); ++it) {
			H2Core::InstrumentComponent* pComponent = *it;
			for ( int nLayer = 0; nLayer < H2Core::InstrumentComponent::getMaxLayers(); nLayer++ ) {
				H2Core::InstrumentLayer* pLayer = pComponent->get_layer( nLayer );
				if( pLayer ) {
					auto pSample = pLayer->get_sample();
					if( loaded ) {
						if( pSample->get_data_l()==nullptr || pSample->get_data_r()==nullptr ) {
							return false;
						}
					} else {
						if( pSample->get_data_l() != nullptr || pSample->get_data_r() != nullptr ) {
							return false;
						}
					}
				}

			}
		}
	}
	return ( count==4 );
}



void XmlTest::testDrumkit()
{
	QString dk_path = H2Core::Filesystem::tmp_dir()+"/dk0";

	H2Core::Drumkit* dk0 = nullptr;
	H2Core::Drumkit* dk1 = nullptr;
	H2Core::Drumkit* dk2 = nullptr;

	// load without samples
	dk0 = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit") );
	CPPUNIT_ASSERT( dk0!=nullptr );
	CPPUNIT_ASSERT( dk0->samples_loaded()==false );
	CPPUNIT_ASSERT( check_samples_data( dk0, false ) );
	CPPUNIT_ASSERT_EQUAL( 4, dk0->get_instruments()->size() );
	//dk0->dump();

	// Check if drumkit was valid (what we assume in this test)
	CPPUNIT_ASSERT( ! H2Core::Filesystem::file_exists( H2TEST_FILE( "/drumkits/baseKit/drumkit.xml.bak" ) ) );
	
	// manually load samples
	dk0->load_samples();
	CPPUNIT_ASSERT( dk0->samples_loaded()==true );
	CPPUNIT_ASSERT( check_samples_data( dk0, true ) );
	//dk0->dump();
	
	// load with samples
	dk0 = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit" ), true );
	CPPUNIT_ASSERT( dk0!=nullptr );
	CPPUNIT_ASSERT( dk0->samples_loaded()==true );
	CPPUNIT_ASSERT( check_samples_data( dk0, true ) );
	//dk0->dump();
	
	// unload samples
	dk0->unload_samples();
	CPPUNIT_ASSERT( dk0->samples_loaded()==false );
	CPPUNIT_ASSERT( check_samples_data( dk0, false ) );
	//dk0->dump();
	
	/*
	// save drumkit elsewhere
	dk0->set_name( "dk0" );
	CPPUNIT_ASSERT( dk0->save( dk_path, false ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( dk_path+"/drumkit.xml" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( dk_path+"/crash.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( dk_path+"/hh.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( dk_path+"/kick.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( dk_path+"/snare.wav" ) );
	// load file
	dk1 = H2Core::Drumkit::load_file( dk_path+"/drumkit.xml" );
	CPPUNIT_ASSERT( dk1!=nullptr );
	//dk1->dump();
	// copy constructor
	dk2 = new H2Core::Drumkit( dk1 );
	dk2->set_name( "COPY" );
	CPPUNIT_ASSERT( dk2!=nullptr );
	// save file
	CPPUNIT_ASSERT( dk2->save_file( dk_path+"/drumkit.xml", true ) );
	*/
	
	delete dk0;
	//delete dk1;
	//delete dk2;
}

void XmlTest::testShippedDrumkits()
{
	H2Core::XMLDoc doc;
	for ( auto ii : H2Core::Filesystem::sys_drumkit_list() ) {
		CPPUNIT_ASSERT( doc.read( QString( "%1%2/drumkit.xml" )
								  .arg( H2Core::Filesystem::sys_drumkits_dir() )
								  .arg( ii ),
								  H2Core::Filesystem::drumkit_xsd_path() ) );

	}
}

//Load drumkit which includes instrument with invalid ADSR values.
// Expected behavior: The drumkit will be loaded successfully. 
//					  In addition, the drumkit file will be saved with 
//					  correct ADSR values.
void XmlTest::testDrumkit_UpgradeInvalidADSRValues()
{
	H2Core::Drumkit* pDrumkit = nullptr;

	//1. Check, if the drumkit has been loaded
	pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/invAdsrKit") );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	
	//2. Make sure that the instruments of the drumkit have been loaded correctly (see GH issue #839)
	H2Core::InstrumentList* pInstruments = pDrumkit->get_instruments();
	CPPUNIT_ASSERT( pInstruments != nullptr );
	
	H2Core::Instrument* pFirstInstrument = pInstruments->get(0);
	CPPUNIT_ASSERT( pFirstInstrument != nullptr );
	
	H2Core::InstrumentLayer* pLayer = pFirstInstrument->get_components()->front()->get_layer(0);
	CPPUNIT_ASSERT( pLayer != nullptr );
	
	auto pSample = pLayer->get_sample();
	CPPUNIT_ASSERT( pSample != nullptr );
	
	CPPUNIT_ASSERT( pSample->get_filename() == QString("snare.wav"));
	
	//3. Make sure that the original (invalid) file has been saved as a backup
	CPPUNIT_ASSERT( H2Core::Filesystem::file_exists( H2TEST_FILE( "/drumkits/invAdsrKit/drumkit.xml.bak") ) );
		
	if( pDrumkit ) {
		delete pDrumkit;
	}

	//4. Load the drumkit again to assure updated file is valid
	pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/invAdsrKit") );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( ! H2Core::Filesystem::file_exists( H2TEST_FILE( "/drumkits/invAdsrKit/drumkit.xml.bak.1") ) );
		 
	if ( pDrumkit ) {
		delete pDrumkit;
	}
	
	// Cleanup
	CPPUNIT_ASSERT( H2Core::Filesystem::file_copy( H2TEST_FILE( "/drumkits/invAdsrKit/drumkit.xml.bak" ), H2TEST_FILE( "/drumkits/invAdsrKit/drumkit.xml" ), true ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::rm( H2TEST_FILE( "/drumkits/invAdsrKit/drumkit.xml.bak"), false ) );
}

void XmlTest::testPattern()
{
	QString pat_path = H2Core::Filesystem::tmp_dir()+"/pat";

	H2Core::Pattern* pat0 = nullptr;
	H2Core::Drumkit* dk0 = nullptr;
	H2Core::InstrumentList* instruments = nullptr;

	dk0 = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit" ) );
	CPPUNIT_ASSERT( dk0!=nullptr );
	instruments = dk0->get_instruments();
	CPPUNIT_ASSERT( instruments->size()==4 );

	pat0 = H2Core::Pattern::load_file( H2TEST_FILE( "/pattern/pat.h2pattern" ), instruments );
	CPPUNIT_ASSERT( pat0 );

	pat0->save_file( "dk_name", "author", "license", pat_path );

	delete pat0;
	delete dk0;
}

void XmlTest::tearDown() {

	QDirIterator it( TestHelper::get_instance()->getTestDataDir(),
					 QDirIterator::Subdirectories);
	QStringList filters;
	filters << "*.bak*";
	
	while ( it.hasNext() ) {
		it.next();
		const QDir testFolder( it.next() );
		const QStringList backupFiles = testFolder.entryList( filters, QDir::NoFilter, QDir::NoSort );

		for ( auto& bbackupFile : backupFiles ) {
			
			H2Core::Filesystem::rm( testFolder.absolutePath()
									.append( "/" )
									.append( bbackupFile ), false );
		}
	}
}
