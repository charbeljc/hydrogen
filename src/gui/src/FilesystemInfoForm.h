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

#ifndef FILESYSTEMINFOFORM_H
#define FILESYSTEMINFOFORM_H

#include <QWidget>
#include "core/Object.h"

namespace Ui {
class FilesystemInfoForm;
}

class FilesystemInfoForm : public QWidget, public H2Core::Object
{
	H2_OBJECT
	Q_OBJECT
	
public:
	explicit FilesystemInfoForm(QWidget *parent = nullptr);
	~FilesystemInfoForm();
	
private:
	Ui::FilesystemInfoForm *ui;
	
	void updateInfo();

private slots:
	void	on_openTmpButton_clicked();
	void	on_openUsrButton_clicked();
	void	on_openSysButton_clicked();
	
	void	showEvent ( QShowEvent* );
};

#endif // FILESYSTEMINFOFORM_H
