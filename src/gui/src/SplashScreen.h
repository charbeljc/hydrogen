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


#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#include <QSplashScreen>
#include <QTimer>
#include <QPixmap>

#include <core/Object.h>

class SplashScreen : public QSplashScreen, public H2Core::Object
{
    H2_OBJECT
	Q_OBJECT
	public:
		SplashScreen();
		~SplashScreen();

	private slots:
		void onCloseTimer();

	private:
		QPixmap *			m_pBackground;
		static const uint	width = 400;
		static const uint	height = 300;
};


#endif
