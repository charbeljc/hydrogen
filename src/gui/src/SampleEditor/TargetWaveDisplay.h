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

#ifndef TARGET_WAVE_DISPLAY
#define TARGET_WAVE_DISPLAY

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <core/Basics/Sample.h>
#include <memory>

class SampleEditor;

namespace H2Core
{
	class InstrumentLayer;
	class EnvelopePoint;
}

class TargetWaveDisplay : public QWidget, public H2Core::Object
{
	H2_OBJECT
	Q_OBJECT

	public:
		explicit TargetWaveDisplay(QWidget* pParent);
		~TargetWaveDisplay();

		void updateDisplay( H2Core::InstrumentLayer *pLayer );
		void updateDisplayPointer();
		void paintLocatorEventTargetDisplay( int pos, bool last_event);
		void paintEvent(QPaintEvent *ev);
		H2Core::Sample::PanEnvelope* get_pan() { return &m_PanEnvelope; }
		H2Core::Sample::VelocityEnvelope* get_velocity() { return &m_VelocityEnvelope; }

	private:
		QPixmap m_Background;

		QString m_sSampleName;
		QString m_sInfo;

		int m_nX;
		int m_nY;
		int m_nLocator;

		int *m_pPeakData_Left;
		int *m_pPeakData_Right;

		unsigned m_nSampleLength;

		bool m_VMove;
		bool m_UpdatePosition;

		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseReleaseEvent(QMouseEvent *ev);

		H2Core::Sample::PanEnvelope m_PanEnvelope;
		H2Core::Sample::VelocityEnvelope m_VelocityEnvelope;
};


#endif
