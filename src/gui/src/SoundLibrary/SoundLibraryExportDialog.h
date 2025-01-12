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

#ifndef SOUND_LIBRARY_EXPORT_DIALOG_H
#define SOUND_LIBRARY_EXPORT_DIALOG_H

#include "ui_SoundLibraryExportDialog_UI.h"

#include <core/Object.h>
#include <core/Basics/Song.h>
#include <core/Basics/Drumkit.h>
#include <core/Helpers/Filesystem.h>

#include <vector>

///
///
///
class SoundLibraryExportDialog : public QDialog, public Ui_SoundLibraryExportDialog_UI, public H2Core::Object
{
	H2_OBJECT
	Q_OBJECT
	public:
		SoundLibraryExportDialog( QWidget* pParent, const QString& sSelectedKit, H2Core::Filesystem::Lookup lookup );
		~SoundLibraryExportDialog();

private slots:
	void on_exportBtn_clicked();
	void on_browseBtn_clicked();
	void on_cancelBtn_clicked();
	void on_versionList_currentIndexChanged( int index );
	void on_drumkitList_currentIndexChanged( QString str );
	void on_drumkitPathTxt_textChanged( QString str );
	void updateDrumkitList();
private:
	std::vector<H2Core::Drumkit*> m_pDrumkitInfoList;
	QString m_sPreselectedKit;
	H2Core::Filesystem::Lookup m_preselectedKitLookup;
	QString m_sSysDrumkitSuffix;
	QHash<QString, QStringList> m_kit_components;
};


#endif

