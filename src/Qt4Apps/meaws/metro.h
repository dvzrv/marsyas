/*
** Copyright (C) 2007 Graham Percival <gperciva@uvic.ca>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <QWidget>
#include <QTimer>
#include <QAction>
#include <QDialog>
#include <QPainter>
#include "MarSystemManager.h"
#include "MarSystemQtWrapper.h"

using namespace Marsyas;
using namespace MarsyasQt;

class Metro: public QWidget {
	Q_OBJECT

public:
	Metro(QWidget *parent, string audioFilename);
	~Metro();
	void stopMetro();
	void startMetro();
	void setIcon(QAction* getVisualAct);

// communication with the QT front-end
public slots:
  void setTempo(int tempo);
	void setIntro(int beats);
	void beat();
	void beatFinished();
	void toggleBigMetro();

protected:
  void paintEvent(QPaintEvent *);

private:
	int tempo;
	int introBeats;

	QTimer *timer;
	QAction *visualMetroBeatAct; // from mainwindow
	QTimer *flashSpeed;
	bool bigDisplay;

	QColor normalBeatColor;
	QColor activeBeatColor;
	QColor drawBeatColor;

// audio stuff
	bool audio;
	void setupAudio(string audioFilename);
	MarSystemQtWrapper *mrsWrapper;
  MarSystem *metroNet;
  MarControlPtr positionPtr;
};

