/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  <copyright holder> <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "screenshotdialog.h"
#include <QTimer>
#include <QPixmap>
#include <QImage>
#include "application.h"
#include <QDesktopWidget>

#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

using namespace LxImage;

ScreenshotDialog::ScreenshotDialog(QWidget* parent, Qt::WindowFlags f): QDialog(parent, f) {
  ui.setupUi(this);
}

ScreenshotDialog::~ScreenshotDialog() {
  qDebug("destroy");
}

void ScreenshotDialog::done(int r) {
  if(r == QDialog::Accepted) {
    hide();
    XSync(QX11Info::display(), 0); // make sure we're not visible
    QDialog::done(r);

    int delay = ui.delay->value();
    if(delay > 0) {
      QTimer::singleShot(delay * 1000, this, SLOT(doScreenshot()));
    }
    else {
      // the dialog object will be deleted in doScreenshot().
      doScreenshot();
    }
  }
  else {
    deleteLater();
  }
}

QRect ScreenshotDialog::windowFrame(WId wid) {
  QRect result;
  XWindowAttributes wa;
  if(XGetWindowAttributes(QX11Info::display(), wid, &wa)) {
    Window child;
    int x, y;
    // translate to root coordinate
    XTranslateCoordinates(QX11Info::display(), wid, wa.root, 0, 0, &x, &y, &child);
    qDebug("%d, %d, %d, %d", x, y, wa.width, wa.height);
    result.setRect(x, y, wa.width, wa.height);

    // get the frame widths added by the window manager
    Atom atom = XInternAtom(QX11Info::display(), "_NET_FRAME_EXTENTS", false);
    unsigned long type, resultLen, rest;
    int format;
    unsigned char* data = NULL;
    if(XGetWindowProperty(QX11Info::display(), wid, atom, 0, G_MAXLONG, false,
      XA_CARDINAL, &type, &format, &resultLen, &rest, &data) == Success) {
    }
    if(data) { // left, right, top, bottom
      long* offsets = reinterpret_cast<long*>(data);
      result.setLeft(result.left() - offsets[0]);
      result.setRight(result.right() + offsets[1]);
      result.setTop(result.top() - offsets[2]);
      result.setBottom(result.bottom() + offsets[3]);
      XFree(data);
    }
  }
  return result;
}

WId ScreenshotDialog::activeWindowId() {
  WId root = WId(QX11Info::appRootWindow());
  Atom atom = XInternAtom(QX11Info::display(), "_NET_ACTIVE_WINDOW", false);
  unsigned long type, resultLen, rest;
  int format;
  WId result = 0;
  unsigned char* data = NULL;
  if(XGetWindowProperty(QX11Info::display(), root, atom, 0, 1, false,
      XA_WINDOW, &type, &format, &resultLen, &rest, &data) == Success) {
    result = *reinterpret_cast<long*>(data);
    XFree(data);
  }
  return result;
}

void ScreenshotDialog::doScreenshot() {
  qDebug("screenshot!");
  WId wid = 0;
  int x = 0, y = 0, width = -1, height = -1;
  wid = QApplication::desktop()->winId(); // get desktop window
  if(ui.currentWindow->isChecked()) {
    WId activeWid = activeWindowId();
    if(activeWid) {
      if(ui.includeFrame->isChecked()) {
        QRect rect = windowFrame(activeWid);
        x = rect.x();
        y = rect.y();
        width = rect.width();
        height = rect.height();
      }
      else
        wid = activeWid;
    }
  }

  QPixmap pixmap;
  pixmap = QPixmap::grabWindow(wid, x, y, width, height);
  QImage image = pixmap.toImage();
  Application* app = static_cast<Application*>(qApp);
  MainWindow* window = app->createWindow();
  window->pasteImage(image);
  window->show();

  deleteLater(); // destroy ourself
}