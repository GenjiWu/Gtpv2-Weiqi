#include "Window.h"

Player::Player(QWidget *parent, int color, int size, double komi) : QObject(parent)
{
    Process = NULL;
    TextEdit = NULL;
    Side = color;
    Wait = 0;

    Append("name");
    Append("version");

    if (size) Append(QString("boardsize %1").arg(size));
    if (komi) Append(QString("komi %1").arg(komi));
}

Player::~Player()
{
    if (Process)
        Process->kill();
}

void Player::Setup(const QString &str, const QString &arg)
{
    if (Process == NULL) {
        Process = new QProcess;
//      process->setReadChannel(QProcess::StandardOutput);
        connect(Process, SIGNAL(readyReadStandardOutput()), this, SLOT(readStandardOutput()));
        connect(Process, SIGNAL(readyReadStandardError()), this, SLOT(readStandardError()));
    }

    QStringList args = arg.split(' ', QString::SkipEmptyParts);
    QFileInfo Info(str);
    Process->setWorkingDirectory(Info.absolutePath());
    Process->start("\"" + str + "\"", args);
}

int Player::CheckTask(const QString &str)
{
    if (!Task.isEmpty())
        return Task[0].startsWith(str);
    return false;
}

void Player::Send()
{
    if (!Task.isEmpty() && Process && Wait == 0) {
        if ((((Widget*) parent())->View & Widget::PLAY_PAUSE) == 0) {
            Process->write(Task[0].toLatin1());
            Respond.clear();
            Wait = 1;
        }
    }
}

void Player::Append(const QString &str)
{
    Task.append(str + "\r\n");
}

void Player::Remove()
{
    Wait = 0;

    if (!Task.isEmpty()) {
        Task.removeFirst();
        Send();
    }
}

void Player::Play(QString str, int color)
{
    if (Process) {
        if (color == WHITE || (color == 0 && Side == BLACK)) 
		{
            Append("play W " + str);
            if (color == 0) Append("genmove b"); // 没有让子
        }
        else 
		{
            Append("play B " + str);
            if (color == 0) Append("genmove w");
        }

        Send();
    }
}

void Player::Play(int x, int y, int size, int color)
{
    Play((x < 0 ? "PASS" : QChar('A' + (x < 8 ? x : ++x))
        + QString::number(size - y)), color);
}

void Player::readStandardOutput()
{
    QString line;

    while (Process->canReadLine()) 
	{
        line = Process->readLine();
        line.replace("\r\n", "\n");

        if (TextEdit) {
            if ((Side == BLACK || Side == WHITE) || !CheckTask("play")) 
			{ // 滚动
                int ScrollDown = TextEdit->verticalScrollBar()->value() == TextEdit->verticalScrollBar()->maximum();
                TextEdit->insertPlainText(line);
                if (ScrollDown) TextEdit->verticalScrollBar()->setValue(TextEdit->verticalScrollBar()->maximum());
            }
        }

        if (Respond.isEmpty()) 
		{
            if (line.startsWith("="))
                Respond = line.mid(line.indexOf(" ") + 1);
        }
        else Respond += line;
    }

    if (Respond.endsWith("\n\n")) 
	{
        ((Widget*) parent())->GetRespond(Respond.trimmed(), Side);
    }
}

void Player::readStandardError()
{
    QByteArray bArray = Process->readAllStandardError();
    QString Info = bArray;

    if (TextEdit) 
	{
        Info.replace("\r\r\n", "\r\n"); // Python Windows Bug
        int ScrollDown = TextEdit->verticalScrollBar()->value() == TextEdit->verticalScrollBar()->maximum();
        TextEdit->insertPlainText(Info);
        if (ScrollDown) TextEdit->verticalScrollBar()->setValue(TextEdit->verticalScrollBar()->maximum());
    }
}

Widget::Widget(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setAcceptDrops(true);

    TextEdit = NULL;
    Table = NULL;

    Play[1] = Play[2] = NULL;

    Child_Board.Reset();
    Child_Board.BOARD_DATE = QDate::currentDate().toString("yyyy-MM-dd");

    Mode = 0;
    View = 0;
    Side = 0; // 下黑棋和白棋

    Cursor[0] = QPoint(-1, -1);
    Cursor[1] = QPoint(-1, -1);

	initshift();
}

int Widget::Read(const QString &str, int k) //打开SGF
{
    if (Mode == BOARD_PLAY) {

    }
    else if (Child_Board.Read(str, k)) 
	{
        ((Window*) parent())->SetTitle(QFileInfo(str).fileName());
        Mode = BOARD_FILE;
        ShowTable(1);
        activateWindow();
        setFocus();
        return 1;
    }

    return 0;
}

void Widget::SetTitle(const QString &str)
{
    Title = str;
    if (str.isEmpty()) parentWidget()->setWindowTitle(((Window*) parent())->Title);
    else parentWidget()->setWindowTitle(((Window*) parent())->Title + " - " + Title);
}

void Widget::ShowTable(int init)
{
    if (Table) 
	{
        int n = Child_Board.BOARD_HANDICAP.isEmpty() ? 6 : 7;

        if (init) 
		{
            Table->clear();
            Table->setColumnCount(2);
            Table->setRowCount(n);
            Table->horizontalHeader()->setStretchLastSection(true);
            Table->horizontalHeader()->hide();
            Table->verticalHeader()->hide();

            Table->setItem(0, 0, new QTableWidgetItem("Event"));
            Table->setItem(1, 0, new QTableWidgetItem("Date"));
            Table->setItem(2, 0, new QTableWidgetItem("Black"));
            Table->setItem(3, 0, new QTableWidgetItem("White"));
            Table->setItem(4, 0, new QTableWidgetItem("Komi"));

            if (n == 6)
                Table->setItem(5, 0, new QTableWidgetItem("Result"));
            else {
                Table->setItem(5, 0, new QTableWidgetItem("Handicap"));
                Table->setItem(6, 0, new QTableWidgetItem("Result"));
            }

            Table->setItem(0, 1, new QTableWidgetItem(Child_Board.BOARD_EVENT));
            Table->setItem(1, 1, new QTableWidgetItem(Child_Board.BOARD_DATE));
            Table->setItem(2, 1, new QTableWidgetItem(Child_Board.PLAYER_BLACK));
            Table->setItem(3, 1, new QTableWidgetItem(Child_Board.PLAYER_WHITE));
            Table->setItem(4, 1, new QTableWidgetItem(Child_Board.BOARD_KOMI));

            if (n == 6)
                Table->setItem(5, 1, new QTableWidgetItem(Child_Board.BOARD_RESULT));
            else {
                Table->setItem(5, 1, new QTableWidgetItem(Child_Board.BOARD_HANDICAP));
                Table->setItem(6, 1, new QTableWidgetItem(Child_Board.BOARD_RESULT));
            }
        }
        if (Child_Board.Record.Index > 0) {
            if (Table->rowCount() == n) {
                Table->insertRow(n);
                Table->setItem(n, 0, new QTableWidgetItem("Move"));
            }
            Table->setItem(n, 1, new QTableWidgetItem(tr("%1").arg(Child_Board.Record.Index)));
        }
        else {
            if (Table->rowCount() > n) {
                Table->removeRow(n);
            }
        }
    }

    if (TextEdit) {
        TextEdit->setPlainText(Child_Board.Record.Node[Child_Board.Record.Index].Text);
    }

    View &= ~VIEW_SCORE;
    repaint();
}

void Widget::GetRespond(const QString &str, int color)
{
    if (color == BLACK || color == WHITE) 
	{
        if (Play[color]->CheckTask("genmove b") && color == BLACK && Child_Board.Record.Node.back().Turn == BLACK ||
            Play[color]->CheckTask("genmove w") && color == WHITE && Child_Board.Record.Node.back().Turn == WHITE)
        {
            int other = Child_Board.OtherColor(color);
            int i = !(View & VIEW_SCORE);

            if (QString::compare(str, "resign", Qt::CaseInsensitive) == 0) 
			{
                Play[color]->Process->write("quit\r\n");
                if (Play[other] && Play[other]->Process)
                    Play[other]->Process->write("quit\r\n");
                Child_Board.BOARD_RESULT = QString(color == BLACK ? "W" : "B") + "+Resign";
                return;
            }
            if (QString::compare(str, "pass", Qt::CaseInsensitive) == 0) 
			{
                Child_Board.Append(-1, -1, color, i);
                repaint();

                if (Child_Board.State.Pass >= 2) 
				{
                    Play[color]->Process->write("quit\r\n");
                    if (Play[other] && Play[other]->Process)
                        Play[other]->Process->write("quit\r\n");
                    return;
                }
            }
            else if (str.size() == 2 || str.size() == 3) 
			{
                int y, x = str[0].toUpper().toLatin1() - 'A';
                if (x > 8) x--;
                if (str.size() == 2) y = str[1].digitValue();
                else y = str.mid(1, 2).toInt();

                Child_Board.Append(x, Child_Board.Size - y, color, i);
                repaint();
            }

            Play[color]->Remove();
            if (Play[other]) Play[other]->Play(str.toUpper());
        }
        else 
		{
            if (Play[color]->CheckTask("name")) 
			{
                if (color == BLACK) Child_Board.PLAYER_BLACK = str;
                else Child_Board.PLAYER_WHITE = str;
            }
            else if (Play[color]->CheckTask("version")) 
			{
                if (color == BLACK) 
				{
                    Child_Board.BLACK_LEVEL = str;
                    if (Play[color]->TextEdit) Play[color]->TextEdit->parentWidget()->
                            setWindowTitle(Child_Board.PLAYER_BLACK + " " + str);
                }
                else 
				{
                    Child_Board.WHITE_LEVEL = str;
                    if (Play[color]->TextEdit) Play[color]->TextEdit->parentWidget()->
                            setWindowTitle(Child_Board.PLAYER_WHITE + " " + str);
                }
            }

            Play[color]->Remove();
        }
    }
}

void Widget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() && Side == 0)
        event->accept();
}

void Widget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        if (Mode == 0 || Mode == BOARD_FILE) {
            Read(event->mimeData()->urls().at(0).toLocalFile());
        }
    }
}

void Widget::wheelEvent(QWheelEvent *event)
{
    if (event->delta() < 0) 
	{
        if (Child_Board.Forward()) ShowTable();
    }
    else if (event->delta() > 0) 
	{
        if (Child_Board.Undo()) ShowTable();
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::NoButton) 
	{
        Cursor[1] = QPoint(-1, -1);

        int Half = GridSize / 2;
        int i2, i = BOARD_LEFT - Half;
        int j2, j = BOARD_TOP - Half;

        for (int x = 0; x < Child_Board.Size; x++) 
		{
            i2 = i + GridSize;
            if (event->x() > i && event->x() <= i2) 
			{
                Cursor[1].setX(x);
                break;
            }
            i = i2;
        }

        for (int y = 0; y < Child_Board.Size; y++) 
		{
            j2 = j + GridSize;
            if (event->y() > j && event->y() <= j2) 
			{
                Cursor[1].setY(y);
                break;
            }
            j = j2;
        }

        if ((Cursor[0].x() < 0 || Cursor[0].y() < 0) &&
            (Cursor[1].x() < 0 || Cursor[1].y() < 0)) return;

        if (Cursor[0] != Cursor[1]) 
		{
            Cursor[0] = Cursor[1];
            repaint();
        }
    }
}

void Widget::mousePressEvent(QMouseEvent *event)
{
    setFocus();

    if (Cursor[0].x() >= 0 && Cursor[0].y() >= 0 && GridSize > 6) {
        if (Mode == BOARD_PLAY) {
            if (((Side ^ 3) & Child_Board.State.Turn) && ((Side ^ 3) & Child_Board.Record.Node.back().Turn)) {
                if (event->buttons() == Qt::LeftButton) {
                    int n = Child_Board.Record.Node.size() - Child_Board.Record.Index - 1;
                    if (Child_Board.Play(Cursor[0].x(), Cursor[0].y(), Child_Board.State.Turn)) {
                        repaint();
                        if (Side) {
                            while (n--) Play[Side]->Append("undo");
                            Play[Side]->Play(Cursor[0].x(), Cursor[0].y(), Child_Board.Size);
                        }
                    }
                }
                else if (event->buttons() == Qt::RightButton) {

                }
            }
        }
        else {
            if (event->buttons() == Qt::LeftButton) {
                if (Child_Board.Play(Cursor[0].x(), Cursor[0].y(), Child_Board.State.Turn, Mode == BOARD_FILE)) {
                    repaint();
                }
            }
        }
    }
}

void Widget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Right: case Qt::Key_Down:
            if (Child_Board.Forward()) ShowTable();
            break;
        case Qt::Key_Left: case Qt::Key_Up:
            if (Child_Board.Undo()) ShowTable();
            break;
        case Qt::Key_Home:
            if (Child_Board.Start()) ShowTable();
            break;
        case Qt::Key_End:
            if (Child_Board.End()) ShowTable();
            break;
        case Qt::Key_PageUp:
            if (Child_Board.Undo(10)) ShowTable();
            break;
        case Qt::Key_PageDown:
            if (Child_Board.Forward(10)) ShowTable();
            break;
        case Qt::Key_F1: // 显示坐标
            View ^= VIEW_LABEL;
            repaint();
            break;
        case Qt::Key_F2: // 显示形势判断
            View ^= VIEW_SCORE;
            if (View & VIEW_SCORE) Child_Board.Judge(2000);
            repaint();
            break;
        case Qt::Key_Escape:
            if (View & VIEW_SCORE) View ^= VIEW_SCORE;
            if (Child_Board.Record2.Index >= 0) 
			{
                while (Child_Board.Record2.Index >= 0) Child_Board.Undo();
            }
            repaint();
            break;
        case Qt::Key_S: // 保存
            Child_Board.BOARD_FILE = "001.SGF";
            if (Child_Board.Write(Child_Board.BOARD_FILE)) 
			{
                Child_Board.BOARD_FILE.clear();
                SetTitle("SAVE");
                return;
            }
            break;
        case Qt::Key_T: // 暂停
            if (Mode == BOARD_PLAY) 
			{
                View ^= PLAY_PAUSE;
                if ((View & PLAY_PAUSE) == 0) 
				{
                    if (Play[BLACK]) Play[BLACK]->Send();
                    if (Play[WHITE]) Play[WHITE]->Send();
                    SetTitle("");
                    repaint();
                }
                else SetTitle("PAUSE");
                return;
            }
            break;
        case Qt::Key_P: // Pass
            if (Mode == BOARD_PLAY) 
			{
                if (((Side ^ 3) & Child_Board.State.Turn) && ((Side ^ 3) & Child_Board.Record.Node.back().Turn)) 
				{
                    int n = Child_Board.Record.Node.size() - Child_Board.Record.Index - 1;
                    Child_Board.Play(-1, -1, Child_Board.State.Turn);
                    repaint();
                    if (Side) 
					{
                        while (n--) Play[Side]->Task.append("undo\r\n");
                        Play[Side]->Play(-1, -1, Child_Board.Size);
                    }
                    SetTitle("PASS");
                    return;
                }
            }
            else {
                Child_Board.Play(-1, -1, Child_Board.State.Turn, Mode == BOARD_FILE);
                repaint();
            }
            break;
    }

    if (!Title.isEmpty() && event->key() != Qt::Key_Control && event->key() != Qt::Key_Alt)
        SetTitle("");
}

void Widget::initshift()
{
	for (int i = 0; i < MAX_SIZE; ++i)
		shift[i] = qrand() % 5 - 2;
}
void Widget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
	
    GridSize = std::min(size().width(), size().height()) / (Child_Board.Size + 2.5); //棋盘格子大小
    int theme = 0; //
	qDebug() << GridSize << endl;
	qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
	
    QColor ColorBack(216, 172, 76); //棋盘背景色
    painter.setBackground(QBrush(ColorBack));
    if (theme) painter.drawTiledPixmap(0, 0, size().width(), size().height(), QPixmap("C:/Users/10309/source/repos/qttest/qttest/RESOURSE/Wood.png"));
    else painter.fillRect(0, 0, size().width(), size().height(), ColorBack);

    int Width = (Child_Board.Size - 1) * GridSize; //棋盘宽度
    int Grid = GridSize * 2;
    double Font = GridSize / 2.8;
    double Radius = GridSize * 0.4823 - 0.5869;
	Radius -= 1.5;
	qDebug() << Radius << endl;
	//到左、上边界的距离
    BOARD_LEFT = (size().width() - Width) / 2;
    BOARD_TOP = (size().height() - Width) / 2;

    BOARD_RIGHT = BOARD_LEFT + Width;
    BOARD_DOWN = BOARD_TOP + Width;

    if (GridSize > 6) {
        painter.setPen(QPen(QColor(48, 48, 48), 0, Qt::SolidLine, Qt::FlatCap));

        for (int i = 0, d = 0; i < Child_Board.Size; i++, d += GridSize) {
            painter.drawLine(BOARD_LEFT + d, BOARD_TOP, BOARD_LEFT + d, BOARD_DOWN);
            painter.drawLine(BOARD_LEFT, BOARD_TOP + d, BOARD_RIGHT + 1, BOARD_TOP + d);
        }

        painter.setRenderHint(QPainter::HighQualityAntialiasing);

        if (Font >= 6) //绘制坐标
		{
            painter.setPen(Qt::black);
            painter.setFont(QFont("Arial", Font));

            // 坐标 
            if (View & VIEW_LABEL) {
                int k1 = BOARD_TOP - GridSize * 1.05;
                int k2 = BOARD_DOWN + GridSize * 1.05;

                for (int i = 0, x = BOARD_LEFT; i < Child_Board.Size; i++, x += GridSize) {
                    QChar Value = QChar('A' + i + (i < 8 ? 0 : 1));
                    if (!(View & VIEW_SCORE)) //形势 
						painter.drawText(x - GridSize + 1, k1 - GridSize + 1, Grid, Grid, Qt::AlignCenter, Value);
                    painter.drawText(x - GridSize + 1, k2 - GridSize + 1, Grid, Grid, Qt::AlignCenter, Value);
                }

                k1 = BOARD_LEFT - GridSize * 1.18;
                k2 = BOARD_RIGHT + GridSize * 1.17;

                for (int i = 0, y = BOARD_TOP; i < Child_Board.Size; i++, y += GridSize) {
                    QString Value = QString::number(Child_Board.Size - i);
                    painter.drawText(k1 - GridSize - 0, y - GridSize + 1, Grid, Grid, Qt::AlignCenter, Value);
                    painter.drawText(k2 - GridSize - 0, y - GridSize + 1, Grid, Grid, Qt::AlignCenter, Value);
                }
            }
            if (View & VIEW_SCORE) {
                painter.setFont(QFont("Arial", Font + 1));
                painter.drawText(0, BOARD_TOP - GridSize * 1.2 - GridSize + 1, size().width(), Grid, Qt::AlignCenter, Child_Board.BOARD_SCORE);
            }
        }

        // 绘制星位
        if (Child_Board.Size >= 13) {
            painter.setPen(QPen(QColor(15, 15, 15), 1.2, Qt::SolidLine));
            painter.setBrush(Qt::black);

            for (int i = 0, k = 0; i < 9; i++, k += 2) {
                if ((Child_Board.Size & 1) == 0 && i >= 4) break;
                painter.drawEllipse(BOARD_LEFT + Child_Board.Star[k] * GridSize - 2, BOARD_TOP + Child_Board.Star[k + 1] * GridSize - 2, 5, 5);
            }
        }

        if (Radius >= 0) {
            int size = GridSize / 4.8;
            QPolygonF Mark, Triangle;
            Mark << QPoint(-1 * size, -1 * size) << QPoint(1 * size, -1 * size) << QPoint(-1 * size, 1 * size) << QPoint(1 * size, 1 * size);
            double dsize = GridSize / 4.2;
            Triangle << QPoint(0, -1 * dsize) << QPoint(0.866 * dsize, 0.5 * dsize) << QPoint(-0.866 * dsize, 0.5 * dsize);
            for (int j = 0, y = BOARD_TOP; j < Child_Board.Size; j++, y += GridSize) {
                for (int i = 0, x = BOARD_LEFT; i < Child_Board.Size; i++, x += GridSize) {

                    int k = Child_Board.GetPoint(i, j);
                    int color = Child_Board.State.ColorTable[k];
                    int label = Child_Board.Status[k].Label;

                    // 空
                    if ((View & VIEW_SCORE) && label != BOTH_AREA) {
                        painter.setPen(Qt::NoPen);
                        if (label == BLACK_AREA || label == WHITE_DEAD) {
                            if (theme) 
								painter.setBrush(QBrush(Child_Board.Status[k].Color));
                            else 
								painter.setBrush(QBrush(QColor(0, 0, 0), Qt::BDiagPattern));
                        }
                        else {
                            if 
								(theme) painter.setBrush(QBrush(Child_Board.Status[k].Color));
                            else 
								painter.setBrush(QBrush(QColor(255, 255, 255), Qt::Dense5Pattern));
                        }
						
                        int half = int (Radius);
                        int size = half * 2 + 1;
                        if (theme) 
							painter.drawRect(x - Radius, y - Radius, GridSize, GridSize);
                        else 
							painter.drawRect(x - half, y - half, size, size);
			
                    }
					//棋子
					//仅在绘制棋子时开启抗锯齿
					painter.setRenderHint(QPainter::Antialiasing, true);
					painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
                    if (color == BLACK || color == WHITE) {
                        int half = GridSize / 2;
                        int size = half * 2 + 1;
						
                        if (color == BLACK) {
                            painter.setPen(QPen(Qt::black, 1, Qt::SolidLine));
                            painter.setBrush(QColor(0, 0, 0));
                            painter.drawEllipse(QPointF(x + shift[i], y + shift[j]), Radius, Radius);
                        }
                        else {
                            painter.setPen(QPen(QColor(36, 36, 36), 1.01, Qt::SolidLine));
                            painter.setBrush(QColor(240, 240, 240));
                            painter.drawEllipse(QPointF(x + shift[j], y + shift[i]), Radius, Radius);
                        }
                    }
					painter.setRenderHint(QPainter::Antialiasing, false);
					painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
                    // 死棋画叉
                    if ((View & VIEW_SCORE) && label != BOTH_AREA) {
                        if (label == BLACK_DEAD || label == WHITE_DEAD) {
                            QPolygonF Poly = Mark.translated(x + 0.55, y + 1);
                            painter.setPen(QPen(color == BLACK ? Qt::white : Qt::black, 1.1, Qt::SolidLine));
                            painter.drawLine(Poly[0], Poly[3]);
                            painter.drawLine(Poly[1], Poly[2]);
                        }
                    }
                }
            }
			
            // 最近一手 //
            int total = Child_Board.Record.Node[Child_Board.Record.Index].Prop.size();
            for (int i = 0; i < total; i++) {
                Board::GoOperation Prop = Child_Board.Record.Node[Child_Board.Record.Index].Prop[i];
                int color = Child_Board.GetColor(Prop.Col, Prop.Row);
                int x = BOARD_LEFT + Prop.Col * GridSize;
                int y = BOARD_TOP + Prop.Row * GridSize;

                if (Prop.Label == Board::TOKEN_PLAY || Prop.Label == Board::TOKEN_CIRCLE) {
                    QColor color2 = (Qt::red);
                    int Half = double (GridSize) / 12;
                    painter.setPen(QPen(color2, 1.04, Qt::SolidLine));
                    painter.setBrush(color2);
                    painter.drawEllipse(QPointF(x + 0.5, y + 0.5), Half, Half);
                }
                else if (Prop.Label == Board::TOKEN_TRIANGLE) {
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(QBrush(color == BLACK ? Qt::white : Qt::black));
                    painter.drawPolygon(Triangle.translated(x + 0.65, y + 1.4));
                }
                else if (Prop.Label == Board::TOKEN_MARK) {
                    QPolygonF Poly = Mark.translated(x + 0.55, y + 1);
                    painter.setPen(QPen(color == BLACK ? Qt::white : Qt::black, 1.2, Qt::SolidLine));
                    painter.drawLine(Poly[0], Poly[3]);
                    painter.drawLine(Poly[1], Poly[2]);
                }
                else if (Prop.Label == Board::TOKEN_SQUARE) {
                    int Half = GridSize / 6;
                    int size = Half * 2;
                    painter.setBrush(Qt::NoBrush);
                    painter.setPen(QPen(color == BLACK ? QColor(240, 240, 240) : Qt::black, 1, Qt::SolidLine));
                    painter.setRenderHint(QPainter::Antialiasing, false);
                    painter.drawRect(x - Half , y - Half, size, size);
                    painter.setRenderHint(QPainter::Antialiasing);
                }
                else if (Prop.Label == Board::TOKEN_LABEL || Prop.Label == Board::TOKEN_NUMBER) {
                    if (Font >= 6) {
                        if (color == EMPTY) painter.setBackgroundMode(Qt::OpaqueMode);
                        painter.setPen(QPen(color == BLACK ? Qt::white : Qt::black, 1, Qt::SolidLine));
                        painter.setFont(QFont("Arial", Font));
                        painter.drawText(x - GridSize + 1, y - GridSize + 1, Grid, Grid, Qt::AlignCenter,
                            Prop.Label == Board::TOKEN_LABEL ? QChar(Prop.Value) : QString::number(Prop.Value));
                        if (color == EMPTY) painter.setBackgroundMode(Qt::TransparentMode);
                    }
                }
            }
	
            // 光标 //
			
            if (Cursor[0].x() >=0 && Cursor[0].y() >= 0) {
				QColor color = Child_Board.Record2.Index < 0 ? (Child_Board.State.Turn == BLACK ? Qt::black : QColor(240, 240, 240)) : Qt::red;
                painter.setPen(QPen(color, 2, Qt::SolidLine));
                painter.setBrush(color);
                painter.drawEllipse(BOARD_LEFT + Cursor[0].x() * GridSize - 2, BOARD_TOP + Cursor[0].y() * GridSize - 2, 5, 5);
            }


			// 按钮
        }
    }

}

Window::Window()
{
    Child_Widget = new Widget(this);
    SetTitle("Window");
    setCentralWidget(Child_Widget);
    resize(800, 700);

	status = new QStatusBar(this);

	menu[0] = new QMenu(u8"操作");
	menu[0]->addAction(u8"上一步/悔棋");
	menu[0]->addAction(u8"下一步");
	menu[0]->addAction(u8"形势判断");
	menu[0]->addAction(u8"转到开始");
	menu[0]->addAction(u8"转到结尾");
	menu[0]->addAction(u8"保存到SGF");

	menu[2] = new QMenu(u8"对局");
	menu[2]->addAction(u8"PASS");
	menu[2]->addAction(u8"暂停");

	menuBar = new QMenuBar(this);
	menuBar->addMenu(menu[0]);
	menuBar->addMenu(menu[2]);
	menuBar->setGeometry(0, 0, this->width()/10, 25);

	connect(menuBar, SIGNAL(triggered(QAction*)), this, SLOT(trigerMenu(QAction*)));
}

void Window::SetTitle(QString str)
{
    if(!str.isEmpty()) Title = str;
    setWindowTitle(Title);
}
void Window::trigerMenu(QAction * act)
{
	if (act->text() == u8"上一步/悔棋")
	{
		if (Child_Widget->Child_Board.Undo())
			Child_Widget->ShowTable();
	}
	else if (act->text() == u8"下一步")
	{
		if (Child_Widget->Child_Board.Forward())
			Child_Widget->ShowTable();
	}
	else if (act->text() == u8"形势判断")
	{
		Child_Widget->View ^= Child_Widget->VIEW_SCORE;
		if (Child_Widget->View & Child_Widget->VIEW_SCORE) 
			Child_Widget->Child_Board.Judge(2000);
		repaint();
	}
	else if (act->text() == u8"转到开始")
	{
		if (Child_Widget->Child_Board.Start())
			Child_Widget->ShowTable();
	}
	else if (act->text() == u8"转到结尾")
	{
		if (Child_Widget->Child_Board.End())
			Child_Widget->ShowTable();
	}
	else if (act->text() == u8"保存到SGF")
	{
		Child_Widget->Child_Board.BOARD_FILE = "001.SGF";
		if (Child_Widget->Child_Board.Write(Child_Widget->Child_Board.BOARD_FILE)) {
			Child_Widget->Child_Board.BOARD_FILE.clear();
			SetTitle("SAVE");
			return;
		}
	}
	else if (act->text() == u8"PASS")
	{
		QKeyEvent pKey1(QEvent::KeyPress, Qt::Key_P, Qt::NoModifier);
		QCoreApplication::sendEvent(Child_Widget, &pKey1);
	}
	else if (act->text() == u8"暂停")
	{
		QKeyEvent pKey1(QEvent::KeyPress, Qt::Key_T, Qt::NoModifier);
		QCoreApplication::sendEvent(Child_Widget, &pKey1);
	}

}

void Window::paintEvent(QPaintEvent *event)
{

}

void Window::CreateDock()
{
	if (Child_Widget->Mode == 0 || Child_Widget->Mode == BOARD_FILE) {
		QDockWidget *dock = new QDockWidget("", this);
		dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
		Child_Widget->Table = new QTableWidget(dock);
		dock->setWidget(Child_Widget->Table);
		addDockWidget(Qt::RightDockWidgetArea, dock);

		Child_Widget->Table->setFrameStyle(QFrame::NoFrame);
		Child_Widget->Table->setFont(QFont(" ", 12));

		dock = new QDockWidget("", this);
		dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
		Child_Widget->TextEdit = new QTextEdit(dock);
		Child_Widget->TextEdit->setFontPointSize(10);
		dock->setWidget(Child_Widget->TextEdit);
		addDockWidget(Qt::RightDockWidgetArea, dock);

		//按钮
	}
    else if (Child_Widget->Mode == BOARD_PLAY) {
        if (Child_Widget->Play[BLACK]) {
            QDockWidget *dock = new QDockWidget(" ", this);
            dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
            Child_Widget->Play[BLACK]->TextEdit = new QTextEdit(dock);
            Child_Widget->Play[BLACK]->TextEdit->setFontPointSize(10);
            dock->setWidget(Child_Widget->Play[BLACK]->TextEdit);
            addDockWidget(Qt::RightDockWidgetArea, dock);
        }

        if (Child_Widget->Play[WHITE]) {
            QDockWidget *dock = new QDockWidget(" ", this);
            dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
            Child_Widget->Play[WHITE]->TextEdit = new QTextEdit(dock);
            Child_Widget->Play[WHITE]->TextEdit->setFontPointSize(10);
            dock->setWidget(Child_Widget->Play[WHITE]->TextEdit);
            addDockWidget(Qt::RightDockWidgetArea, dock);
        }
    }
}

void Window::CreateMenu()
{
	
}

void Window::TriggerMenuBar()
{
}
