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

#include <core/config.h>
#include <core/Version.h>
#include <core/Hydrogen.h>
#include <core/EventQueue.h>
#include <core/FX/LadspaFX.h>
#include <core/Preferences.h>
#include <core/Helpers/Filesystem.h>

#include "HydrogenApp.h"
#include "Skin.h"
#include "PreferencesDialog.h"
#include "MainForm.h"
#include "PlayerControl.h"
#include "AudioEngineInfoForm.h"
#include "FilesystemInfoForm.h"
#include "LadspaFXProperties.h"
#include "InstrumentRack.h"
#include "Director.h"

#include "PatternEditor/PatternEditorPanel.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"
#include "SoundLibrary/SoundLibraryDatastructures.h"
#include "SoundLibrary/SoundLibraryPanel.h"
#include "PlaylistEditor/PlaylistDialog.h"
#include "SampleEditor/SampleEditor.h"
#include "Mixer/Mixer.h"
#include "Mixer/MixerLine.h"
#include "UndoActions.h"

#include <core/Basics/PatternList.h>
#include <core/Basics/InstrumentList.h>

#include "Widgets/InfoBar.h"

#include <QtGui>
#include <QtWidgets>


using namespace H2Core;


HydrogenApp* HydrogenApp::m_pInstance = nullptr;
const char* HydrogenApp::__class_name = "HydrogenApp";

HydrogenApp::HydrogenApp( MainForm *pMainForm, Song *pFirstSong )
 : Object( __class_name )
 , m_pMainForm( pMainForm )
 , m_pMixer( nullptr )
 , m_pPatternEditorPanel( nullptr )
 , m_pAudioEngineInfoForm( nullptr )
 , m_pSongEditorPanel( nullptr )
 , m_pPlayerControl( nullptr )
 , m_pPlaylistDialog( nullptr )
 , m_pSampleEditor( nullptr )
 , m_pDirector( nullptr )

{
	m_pInstance = this;

	m_pEventQueueTimer = new QTimer(this);
	connect( m_pEventQueueTimer, SIGNAL( timeout() ), this, SLOT( onEventQueueTimer() ) );
	m_pEventQueueTimer->start( QUEUE_TIMER_PERIOD );

	if ( ! Hydrogen::get_instance()->isUnderSessionManagement() ) {
		// When under Non Session Management the new Song will be
		// loaded by the corresponding NSM client instance.
		Hydrogen::get_instance()->setSong( pFirstSong );
	} 
	
	SoundLibraryDatabase::create_instance();

	//setup the undo stack
	m_pUndoStack = new QUndoStack( this );

	updateWindowTitle();

	Preferences *pPref = Preferences::get_instance();

	setupSinglePanedInterface();

	// restore audio engine form properties
	m_pAudioEngineInfoForm = new AudioEngineInfoForm( nullptr );
	WindowProperties audioEngineInfoProp = pPref->getAudioEngineInfoProperties();
	m_pAudioEngineInfoForm->move( audioEngineInfoProp.x, audioEngineInfoProp.y );
	if ( audioEngineInfoProp.visible ) {
		m_pAudioEngineInfoForm->show();
	}
	else {
		m_pAudioEngineInfoForm->hide();
	}
	
	m_pFilesystemInfoForm = new FilesystemInfoForm( nullptr );

	m_pPlaylistDialog = new PlaylistDialog( nullptr );
	m_pDirector = new Director( nullptr );

	// Initially keyboard cursor is hidden.
	m_bHideKeyboardCursor = true;
	
	// Since HydrogenApp does implement some handler functions for
	// Events as well, it should be registered as an Eventlistener
	// itself.
	addEventListener( this );
}



HydrogenApp::~HydrogenApp()
{
	INFOLOG( "[~HydrogenApp]" );
	m_pEventQueueTimer->stop();


	//delete the undo tmp directory
	cleanupTemporaryFiles();

	delete m_pAudioEngineInfoForm;
	delete m_pFilesystemInfoForm;
	delete m_pMixer;
	delete m_pPlaylistDialog;
	delete m_pDirector;
	delete m_pSampleEditor;

	delete SoundLibraryDatabase::get_instance();

	Hydrogen *pEngine = Hydrogen::get_instance();
	if (pEngine) {
		H2Core::Song * pSong = pEngine->getSong();
		// Hydrogen calls removeSong on from its destructor, so here we just delete the objects:
		delete pEngine;
		delete pSong;
	}

	#ifdef H2CORE_HAVE_LADSPA
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		delete m_pLadspaFXProperties[nFX];
	}
	#endif

}



/// Return an HydrogenApp m_pInstance
HydrogenApp* HydrogenApp::get_instance() {
	if (m_pInstance == nullptr) {
		std::cerr << "Error! HydrogenApp::get_instance (m_pInstance = NULL)" << std::endl;
	}
	return m_pInstance;
}




void HydrogenApp::setupSinglePanedInterface()
{
	Preferences *pPref = Preferences::get_instance();
	int uiLayout = pPref->getDefaultUILayout();

	// MAINFORM
	WindowProperties mainFormProp = pPref->getMainFormProperties();
	m_pMainForm->resize( mainFormProp.width, mainFormProp.height );
	m_pMainForm->move( mainFormProp.x, mainFormProp.y );

	m_pSplitter = new QSplitter( nullptr );
	m_pSplitter->setOrientation( Qt::Vertical );
	m_pSplitter->setOpaqueResize( true );

	m_pTab = new QTabWidget( nullptr );
	m_pTab->setObjectName( "TabbedInterface" );

	// SONG EDITOR
	if( uiLayout == Preferences::UI_LAYOUT_SINGLE_PANE) {
		m_pSongEditorPanel = new SongEditorPanel( m_pSplitter );
	} else {
		m_pSongEditorPanel = new SongEditorPanel( m_pTab );
	}

	WindowProperties songEditorProp = pPref->getSongEditorProperties();
	m_pSongEditorPanel->resize( songEditorProp.width, songEditorProp.height );

	if( uiLayout == Preferences::UI_LAYOUT_TABBED) {
		m_pTab->addTab( m_pSongEditorPanel, tr("Song Editor") );
	}

	// this HBox will contain the InstrumentRack and the Pattern editor
	QWidget *pSouthPanel = new QWidget( m_pSplitter );
	pSouthPanel->setObjectName( "SouthPanel" );
	QHBoxLayout *pEditorHBox = new QHBoxLayout();
	pEditorHBox->setSpacing( 5 );
	pEditorHBox->setMargin( 0 );
	pSouthPanel->setLayout( pEditorHBox );

	// INSTRUMENT RACK
	m_pInstrumentRack = new InstrumentRack( nullptr );
	WindowProperties instrumentRackProp = pPref->getInstrumentRackProperties();
	m_pInstrumentRack->setHidden( !instrumentRackProp.visible );

	if( uiLayout == Preferences::UI_LAYOUT_TABBED ){
		m_pTab->setMovable( false );
		m_pTab->setTabsClosable( false );
		m_pTab->addTab( pSouthPanel, tr( "Instrument + Pattern") );
	}

	// PATTERN EDITOR
	m_pPatternEditorPanel = new PatternEditorPanel( nullptr );
	WindowProperties patternEditorProp = pPref->getPatternEditorProperties();
	m_pPatternEditorPanel->resize( patternEditorProp.width, patternEditorProp.height );

	pEditorHBox->addWidget( m_pPatternEditorPanel );
	pEditorHBox->addWidget( m_pInstrumentRack );

	// PLayer control
	m_pPlayerControl = new PlayerControl( nullptr );


	QWidget *mainArea = new QWidget( m_pMainForm );	// this is the main widget
	m_pMainForm->setCentralWidget( mainArea );

	// LAYOUT!!
	m_pMainVBox = new QVBoxLayout();
	m_pMainVBox->setSpacing( 1 );
	m_pMainVBox->setMargin( 0 );
	m_pMainVBox->addWidget( m_pPlayerControl );

	m_pMainVBox->addSpacing( 3 );

	if( uiLayout == Preferences::UI_LAYOUT_SINGLE_PANE) {
		m_pMainVBox->addWidget( m_pSplitter );
	} else {
		m_pMainVBox->addWidget( m_pTab );

	}

	mainArea->setLayout( m_pMainVBox );




	// MIXER
	m_pMixer = new Mixer(nullptr);
	WindowProperties mixerProp = pPref->getMixerProperties();

	m_pMixer->resize( mixerProp.width, mixerProp.height );
	m_pMixer->move( mixerProp.x, mixerProp.y );

	if( uiLayout == Preferences::UI_LAYOUT_TABBED){
		m_pTab->addTab(m_pMixer,tr("Mixer"));
	}

	m_pMixer->updateMixer();

	if ( mixerProp.visible && uiLayout == Preferences::UI_LAYOUT_SINGLE_PANE ) {
		m_pMixer->show();
	}
	else {
		m_pMixer->hide();
	}


#ifdef H2CORE_HAVE_LADSPA
	// LADSPA FX
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pLadspaFXProperties[nFX] = new LadspaFXProperties( nullptr, nFX );
		m_pLadspaFXProperties[nFX]->hide();
		WindowProperties prop = pPref->getLadspaProperties(nFX);
		m_pLadspaFXProperties[nFX]->move( prop.x, prop.y );
		if ( prop.visible ) {
			m_pLadspaFXProperties[nFX]->show();
		}
		else {
			m_pLadspaFXProperties[nFX]->hide();
		}
	}
#endif

	if( uiLayout == Preferences::UI_LAYOUT_TABBED){
		m_pTab->setCurrentIndex( Preferences::get_instance()->getLastOpenTab() );
		QObject::connect(m_pTab, SIGNAL(currentChanged(int)),this,SLOT(currentTabChanged(int)));
	}
}


InfoBar *HydrogenApp::addInfoBar() {
	InfoBar *pInfoBar = new InfoBar();
	m_pMainVBox->insertWidget( 1, pInfoBar );
	return pInfoBar;
}



void HydrogenApp::currentTabChanged(int index)
{
	Preferences::get_instance()->setLastOpenTab( index );
}

void HydrogenApp::closeFXProperties()
{
#ifdef H2CORE_HAVE_LADSPA
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pLadspaFXProperties[nFX]->close();
	}
#endif
}

bool HydrogenApp::openSong( const QString sFilename ) {

	auto pCoreActionController = Hydrogen::get_instance()->getCoreActionController();
	if ( ! pCoreActionController->openSong( sFilename ) ) {
		QMessageBox::information( m_pMainForm, "Hydrogen", tr("Error loading song.") );
		return false;
	}

	return true;
}

bool HydrogenApp::openSong( H2Core::Song* pSong ) {

	auto pCoreActionController = Hydrogen::get_instance()->getCoreActionController();
	if ( ! pCoreActionController->openSong( pSong ) ) {
		QMessageBox::information( m_pMainForm, "Hydrogen", tr("Error loading song.") );
		return false;
	}

	return true;
}

void HydrogenApp::showMixer(bool show)
{
	/*
		 *   Switch to Mixer tab with alt+m in tabbed mode,
		 *   otherwise open mixer window
		 */

	Preferences *pPref = Preferences::get_instance();
	int uiLayout = pPref->getDefaultUILayout();

	if( uiLayout == Preferences::UI_LAYOUT_TABBED )
	{
		m_pTab->setCurrentIndex( 2 );
	} else {
		m_pMixer->setVisible( show );
	}

	m_pMainForm->update_mixer_checkbox();
}

void HydrogenApp::showInstrumentPanel(bool show)
{
	/*
		 *   Switch to pattern editor/instrument tab in tabbed mode,
		 *   otherwise hide instrument panel
		 */

	Preferences *pPref = Preferences::get_instance();
	int uiLayout = pPref->getDefaultUILayout();

	if( uiLayout == Preferences::UI_LAYOUT_TABBED )
	{
		m_pTab->setCurrentIndex( 1 );
		getInstrumentRack()->setHidden( show );
	} else {
		getInstrumentRack()->setHidden( show );
	}
		m_pMainForm->update_instrument_checkbox( !show );
}



void HydrogenApp::showPreferencesDialog()
{
	PreferencesDialog preferencesDialog(m_pMainForm);
	preferencesDialog.exec();
}




void HydrogenApp::setStatusBarMessage( const QString& msg, int msec )
{
	getPlayerControl()->resetStatusLabel();
	getPlayerControl()->showMessage( msg, msec );
}

void HydrogenApp::updateWindowTitle()
{
	Song *pSong = Hydrogen::get_instance()->getSong();
	assert(pSong);

	QString title;

	// special handling for initial title
	QString qsSongName( pSong->getName() );

	if( qsSongName == "Untitled Song" && !pSong->getFilename().isEmpty() ){
		qsSongName = pSong->getFilename().section( '/', -1 );
	}

	if(pSong->getIsModified()){
		title = qsSongName + " (" + QString(tr("modified")) + ")";
	} else {
		title = qsSongName;
	}

	m_pMainForm->setWindowTitle( ( "Hydrogen " + QString( get_version().c_str()) + QString( " - " ) + title ) );
}

void HydrogenApp::setScrollStatusBarMessage( const QString& msg, int msec, bool test )
{
	getPlayerControl()->showScrollMessage( msg, msec , test);
}



void HydrogenApp::showAudioEngineInfoForm()
{
	m_pAudioEngineInfoForm->hide();
	m_pAudioEngineInfoForm->show();
}

void HydrogenApp::showFilesystemInfoForm()
{
	m_pFilesystemInfoForm->hide();
	m_pFilesystemInfoForm->show();
}

void HydrogenApp::showPlaylistDialog()
{
	if ( m_pPlaylistDialog->isVisible() ) {
		m_pPlaylistDialog->hide();
	} else {
		m_pPlaylistDialog->show();
	}
	m_pMainForm->update_playlist_checkbox();
}


void HydrogenApp::showDirector()
{
	if ( m_pDirector->isVisible() ) {
		m_pDirector->hide();
	} else {
		m_pDirector->show();
	}
	m_pMainForm->update_director_checkbox();
}


void HydrogenApp::showSampleEditor( QString name, int mSelectedComponemt, int mSelectedLayer )
{

	if ( m_pSampleEditor ){
		QApplication::setOverrideCursor(Qt::WaitCursor);
		m_pSampleEditor->close();
		delete m_pSampleEditor;
		m_pSampleEditor = nullptr;
		QApplication::restoreOverrideCursor();
	}
	QApplication::setOverrideCursor(Qt::WaitCursor);
	m_pSampleEditor = new SampleEditor( nullptr, mSelectedComponemt, mSelectedLayer, name );
	m_pSampleEditor->show();
	QApplication::restoreOverrideCursor();
}

void HydrogenApp::onDrumkitLoad( QString name ){
	setStatusBarMessage( tr( "Drumkit loaded: [%1]" ).arg( name ), 2000 );
	m_pPatternEditorPanel->updateSLnameLabel( );
}

void HydrogenApp::songModifiedEvent()
{
	updateWindowTitle();
}

void HydrogenApp::onEventQueueTimer()
{
	// use the timer to do schedule instrument slaughter;
	EventQueue *pQueue = EventQueue::get_instance();

	Event event;
	while ( ( event = pQueue->pop_event() ).type != EVENT_NONE ) {
		
		// Provide the event to all EventListeners registered to
		// HydrogenApp. By registering itself as EventListener and
		// implementing at least on the methods used below a
		// particular GUI component can react on specific events.
		for (int i = 0; i < (int)m_EventListeners.size(); i++ ) {
			EventListener *pListener = m_EventListeners[ i ];

			switch ( event.type ) {
			case EVENT_STATE:
				pListener->stateChangedEvent( event.value );
				break;

			case EVENT_PATTERN_CHANGED:
				pListener->patternChangedEvent();
				break;

			case EVENT_PATTERN_MODIFIED:
				pListener->patternModifiedEvent();
				break;

			case EVENT_SONG_MODIFIED:
				pListener->songModifiedEvent();
				break;

			case EVENT_SELECTED_PATTERN_CHANGED:
				pListener->selectedPatternChangedEvent();
				break;

			case EVENT_SELECTED_INSTRUMENT_CHANGED:
				pListener->selectedInstrumentChangedEvent();
				break;

			case EVENT_PARAMETERS_INSTRUMENT_CHANGED:
				pListener->parametersInstrumentChangedEvent();
				break;

			case EVENT_MIDI_ACTIVITY:
				pListener->midiActivityEvent();
				break;

			case EVENT_NOTEON:
				pListener->noteOnEvent( event.value );
				break;

			case EVENT_ERROR:
				pListener->errorEvent( event.value );
				break;

			case EVENT_XRUN:
				pListener->XRunEvent();
				break;

			case EVENT_METRONOME:
				pListener->metronomeEvent( event.value );
				break;

			case EVENT_RECALCULATERUBBERBAND:
				pListener->rubberbandbpmchangeEvent();
				break;

			case EVENT_PROGRESS:
				pListener->progressEvent( event.value );
				break;

			case EVENT_JACK_SESSION:
				pListener->jacksessionEvent( event.value );
				break;

			case EVENT_PLAYLIST_LOADSONG:
				pListener->playlistLoadSongEvent( event.value );
				break;

			case EVENT_UNDO_REDO:
				pListener->undoRedoActionEvent( event.value );
				break;

			case EVENT_TEMPO_CHANGED:
				pListener->tempoChangedEvent( event.value );
				break;
				
			case EVENT_UPDATE_PREFERENCES:
				pListener->updatePreferencesEvent( event.value );
				break;
			
			case EVENT_UPDATE_SONG:
				pListener->updateSongEvent( event.value );
				break;
				
			case EVENT_QUIT:
				pListener->quitEvent( event.value );
				break;

			case EVENT_TIMELINE_ACTIVATION:
				pListener->timelineActivationEvent( event.value );
				break;

			case EVENT_TIMELINE_UPDATE:
				pListener->timelineUpdateEvent( event.value );
				break;

			case EVENT_JACK_TRANSPORT_ACTIVATION:
				pListener->jackTransportActivationEvent( event.value );
				break;

			case EVENT_JACK_TIMEBASE_ACTIVATION:
				pListener->jackTimebaseActivationEvent( event.value );
				break;
				
			case EVENT_SONG_MODE_ACTIVATION:
				pListener->songModeActivationEvent( event.value );
				break;
				
			case EVENT_LOOP_MODE_ACTIVATION:
				pListener->loopModeActivationEvent( event.value );
				break;

			case EVENT_ACTION_MODE_CHANGE:
				pListener->actionModeChangeEvent( event.value );
				break;
				
			default:
				ERRORLOG( QString("[onEventQueueTimer] Unhandled event: %1").arg( event.type ) );
			}
		}

	}

	// midi notes
	while( !pQueue->m_addMidiNoteVector.empty() ){
		Song *pSong = Hydrogen::get_instance()->getSong();
		Instrument *pInstrument = pSong->getInstrumentList()->get( pQueue->m_addMidiNoteVector[0].m_row );
		// find if a (pitch matching) note is already present
		Note *pOldNote = pSong->getPatternList()->get( pQueue->m_addMidiNoteVector[0].m_pattern )
														->find_note( pQueue->m_addMidiNoteVector[0].m_column,
																	 pQueue->m_addMidiNoteVector[0].m_column,
																	 pInstrument,
																	 pQueue->m_addMidiNoteVector[0].nk_noteKeyVal,
																	 pQueue->m_addMidiNoteVector[0].no_octaveKeyVal );
		auto pUndoStack = HydrogenApp::get_instance()->m_pUndoStack;
		pUndoStack->beginMacro( tr( "Input Midi Note" ) );
		if( pOldNote ) { // note found => remove it
			SE_addOrDeleteNoteAction *action = new SE_addOrDeleteNoteAction( pOldNote->get_position(),
																	 pOldNote->get_instrument_id(),
																	 pQueue->m_addMidiNoteVector[0].m_pattern,
																	 pOldNote->get_length(),
																	 pOldNote->get_velocity(),
																	 pOldNote->get_pan_l(),
																	 pOldNote->get_pan_r(),
																	 pOldNote->get_lead_lag(),
																	 pOldNote->get_key(),
																	 pOldNote->get_octave(),
																	 pOldNote->get_probability(),
																	 /*isDelete*/ true,
																	 /*hearNote*/ false,
																	 /*isMidi*/ false,
																	 /*isInstrumentMode*/ false,
																	 /*isNoteOff*/ false );
			pUndoStack->push( action );
		}
		// add the new note
		SE_addOrDeleteNoteAction *action = new SE_addOrDeleteNoteAction( pQueue->m_addMidiNoteVector[0].m_column,
																	 pQueue->m_addMidiNoteVector[0].m_row,
																	 pQueue->m_addMidiNoteVector[0].m_pattern,
																	 pQueue->m_addMidiNoteVector[0].m_length,
																	 pQueue->m_addMidiNoteVector[0].f_velocity,
																	 pQueue->m_addMidiNoteVector[0].f_pan_L,
																	 pQueue->m_addMidiNoteVector[0].f_pan_R,
																	 0.0,
																	 pQueue->m_addMidiNoteVector[0].nk_noteKeyVal,
																	 pQueue->m_addMidiNoteVector[0].no_octaveKeyVal,
																	 1.0f,
																	 /*isDelete*/ false,
																	 false,
																	 pQueue->m_addMidiNoteVector[0].b_isMidi,
																	 pQueue->m_addMidiNoteVector[0].b_isInstrumentMode,
																	 false );
		pUndoStack->push( action );
		pUndoStack->endMacro();
		pQueue->m_addMidiNoteVector.erase( pQueue->m_addMidiNoteVector.begin() );
	}
}


void HydrogenApp::addEventListener( EventListener* pListener )
{
	if (pListener) {
		m_EventListeners.push_back( pListener );
	}
}


void HydrogenApp::removeEventListener( EventListener* pListener )
{
	for ( uint i = 0; i < m_EventListeners.size(); i++ ) {
		if ( pListener == m_EventListeners[ i ] ) {
			m_EventListeners.erase( m_EventListeners.begin() + i );
		}
	}
}

/**
 * Removes temporary files that were created
 * for undo'ing things.
 */
void HydrogenApp::cleanupTemporaryFiles()
{
	Filesystem::rm( Filesystem::tmp_dir(), true );
}

void HydrogenApp::updatePreferencesEvent( int nValue ) {
	
	QString sPreferencesFilename;
	
	// Local path of the preferences used during session management.
	const QString sPreferencesOverwritePath = 
		H2Core::Filesystem::getPreferencesOverwritePath();
	if ( sPreferencesOverwritePath.isEmpty() ) {
		sPreferencesFilename = Filesystem::usr_config_path();
	} else {
		sPreferencesFilename = sPreferencesOverwritePath;
	}
		
	if ( nValue == 0 ) {
		setScrollStatusBarMessage( tr("Preferences saved.") + 
								   QString(" Into: ") + 
								   sPreferencesFilename, 2000 );
	} else if ( nValue == 1 ) {
		
		// Since the Preferences have changed, we also have to reflect
		// these changes in the GUI - its format, colors, fonts,
		// selections etc.
		// But we won't change the layout!
		Preferences *pPref = Preferences::get_instance();
		int uiLayout = pPref->getDefaultUILayout();

		WindowProperties audioEngineInfoProp = pPref->getAudioEngineInfoProperties();
		m_pAudioEngineInfoForm->move( audioEngineInfoProp.x, audioEngineInfoProp.y );
		if ( audioEngineInfoProp.visible ) {
			m_pAudioEngineInfoForm->show();
		}
		else {
			m_pAudioEngineInfoForm->hide();
		}

		// MAINFORM
		WindowProperties mainFormProp = pPref->getMainFormProperties();
		m_pMainForm->resize( mainFormProp.width, mainFormProp.height );
		m_pMainForm->move( mainFormProp.x, mainFormProp.y );

		m_pSplitter->setOrientation( Qt::Vertical );
		m_pSplitter->setOpaqueResize( true );

		// SONG EDITOR
		WindowProperties songEditorProp = pPref->getSongEditorProperties();
		m_pSongEditorPanel->resize( songEditorProp.width, songEditorProp.height );

		// PATTERN EDITOR
		WindowProperties patternEditorProp = pPref->getPatternEditorProperties();
		m_pPatternEditorPanel->resize( patternEditorProp.width, patternEditorProp.height );
		
		WindowProperties instrumentRackProp = pPref->getInstrumentRackProperties();
		m_pInstrumentRack->setHidden( !instrumentRackProp.visible );

		WindowProperties mixerProp = pPref->getMixerProperties();

		m_pMixer->resize( mixerProp.width, mixerProp.height );
		m_pMixer->move( mixerProp.x, mixerProp.y );

		m_pMixer->updateMixer();

		if ( mixerProp.visible && uiLayout == Preferences::UI_LAYOUT_SINGLE_PANE ) {
			m_pMixer->show();
		}
		else {
			m_pMixer->hide();
		}
		
#ifdef H2CORE_HAVE_LADSPA
		// LADSPA FX
		for (uint nFX = 0; nFX < MAX_FX; nFX++) {
			m_pLadspaFXProperties[nFX]->hide();
			WindowProperties prop = pPref->getLadspaProperties(nFX);
			m_pLadspaFXProperties[nFX]->move( prop.x, prop.y );
			if ( prop.visible ) {
				m_pLadspaFXProperties[nFX]->show();
			}
			else {
				m_pLadspaFXProperties[nFX]->hide();
			}
		}
#endif

		// Inform the user about which file was loaded.
		setScrollStatusBarMessage( tr("Preferences loaded.") + 
								   QString(" From: ") + 
								   sPreferencesFilename, 2000 );

	
	} else {
		ERRORLOG( QString( "Unknown event parameter [%1] in HydrogenApp::updatePreferencesEvent" )
				  .arg( nValue ) );
	}
	
}

void HydrogenApp::updateSongEvent( int nValue ) {

	Hydrogen* pHydrogen = Hydrogen::get_instance();	
	
	if ( nValue == 0 || nValue == 1 ) {

		// Set a Song prepared by the core part.
		Song* pNextSong = pHydrogen->getNextSong();

		if ( ! pHydrogen->getNextSongPath().isEmpty() ) {
			pNextSong->setFilename( pHydrogen->getNextSongPath() );
		}
		
		pHydrogen->setSong( pNextSong );

		// Cleanup
		closeFXProperties();
		m_pUndoStack->clear();
		
		// Add the new loaded song in the "last used song" vector.
		// This behavior is prohibited under session management. Only
		// songs open during normal runs will be listed.
		if ( ! pHydrogen->isUnderSessionManagement() ) {
			Preferences::get_instance()->insertRecentFile( pNextSong->getFilename() );
		}

		// Update GUI components
		m_pSongEditorPanel->updateAll();
		m_pPatternEditorPanel->updateSLnameLabel();
		updateWindowTitle();
		getInstrumentRack()->getSoundLibraryPanel()->update_background_color();
		getSongEditorPanel()->updatePositionRuler();
		pHydrogen->getTimeline()->deleteAllTags();
	
		// Trigger a reset of the Director and MetronomeWidget.
		EventQueue::get_instance()->push_event( EVENT_METRONOME, 2 );
		EventQueue::get_instance()->push_event( EVENT_METRONOME, 3 );
	
		m_pSongEditorPanel->updateAll();
		m_pPatternEditorPanel->updateSLnameLabel();
		updateWindowTitle();
		
		if ( nValue == 1 ) {	
			pHydrogen->restartDrivers();
		}
		
	} else if ( nValue == 2 ) {
		
		QString filename = pHydrogen->getSong()->getFilename();
		
		// Song was saved.
		setScrollStatusBarMessage( tr("Song saved.") + QString(" Into: ") + filename, 2000 );
		updateWindowTitle();
		EventQueue::get_instance()->push_event( EVENT_METRONOME, 3 );
		
	} else if ( nValue == 3 ) {

		// The event was triggered before the Song was fully loaded by
		// the core. It's most likely to be present by now, but it's
		// probably better to avoid displaying its path just to be
		// sure.
		QMessageBox::information( m_pMainForm, "Hydrogen", tr("Song is read-only.\nUse 'Save as' to enable autosave." ) );
	}
}

void HydrogenApp::quitEvent( int nValue ) {

	m_pMainForm->closeAll();
	
}
