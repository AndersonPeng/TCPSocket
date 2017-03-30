#include "droparea.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>


/*===============================
Constructor
===============================*/
DropArea::DropArea(QWidget *parent) : QLabel(parent)
{
    setMinimumSize(200, 50);
    setFrameStyle(QFrame::Sunken|QFrame::StyledPanel);
    setAlignment(Qt::AlignCenter);
    setAcceptDrops(true);
    setAutoFillBackground(true);
    clear();
}


/*===============================
Destructor
===============================*/
DropArea::~DropArea()
{

}


/*===============================
EVENT: on drag enter
===============================*/
void DropArea::dragEnterEvent(QDragEnterEvent *event)
{
    setText(tr("<drop content>"));
    setBackgroundRole(QPalette::Highlight);

    //Set the drop action to the one proposed
    event->acceptProposedAction();

    //Emit changed() signal
    emit changed(event->mimeData());
}


/*===============================
EVENT: on drag move
===============================*/
void DropArea::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}


/*===============================
EVENT: on drop
===============================*/
void DropArea::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if(mimeData->hasImage())
        setPixmap(qvariant_cast<QPixmap>(mimeData->imageData()));
    else if(mimeData->hasHtml()){
        setText(mimeData->html());
        setTextFormat(Qt::RichText);
    }
    else if(mimeData->hasText()){
        setText(mimeData->text());
        setTextFormat(Qt::PlainText);
    }
    else if(mimeData->hasUrls()){
        QList<QUrl> urlList = mimeData->urls();
        QString text;

        for(int i = 0; i < urlList.size() && i < 32; ++i)
            text += urlList.at(i).path() + QLatin1Char('\n');
        setText(text);
    }
    else
        setText(tr("Cannot display data"));

    setBackgroundRole(QPalette::Dark);
    event->acceptProposedAction();
}


/*===============================
EVENT: on drop
===============================*/
void DropArea::dragLeaveEvent(QDragLeaveEvent *event)
{
    clear();
    event->accept();
}


/*===============================
SLOT: clear the area
===============================*/
void DropArea::clear()
{
    setText(tr("<drop content>"));
    setBackgroundRole(QPalette::Dark);

    emit changed();
}
