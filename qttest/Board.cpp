#include "Board.h"

Board::Board()
{
    BOARD_KOMI = "7.5";
    Komi = 7.5;
}

void Board::Reset()
{
	Size = Width = Height = MAX_SIZE;
 
    DATA_WIDTH = Width + 1; //20
    DATA_HEIGHT = Height + 2; //21

    DATA_START = Width + 2;
    DATA_END = (Width + 1) * (Height + 1) - 1;
	
	//Side��¼��Χ4��λ�ù�ϵ
    Side[0] = 1; Side[1] = DATA_WIDTH;
    Side[2] = -1; Side[3] = -DATA_WIDTH;

    Side[4] = DATA_WIDTH + 1; Side[5] = DATA_WIDTH - 1;
    Side[6] = -DATA_WIDTH + 1; Side[7] = -DATA_WIDTH - 1;
	
	//Star��¼��λ
	int v1, v2, v3;
	if (Size == 19)
	{
		v1 = 3;
		v2 = 15;
		v3 = 9;
	}

    Star[0] = v2; Star[1] = v1;
    Star[2] = v1; Star[3] = v2;
    Star[4] = v1; Star[5] = v1;
    Star[6] = v2; Star[7] = v2;
    Star[8] = v3; Star[9] = v3;
    Star[10] = v1; Star[11] = v3;
    Star[12] = v2; Star[13] = v3;
    Star[14] = v3; Star[15] = v1;
    Star[16] = v3; Star[17] = v2;

    Clear();
}

void Board::Clear()
{
    State.Table[DATA_WIDTH * DATA_HEIGHT] = OUT; //?
    State.Ko = 0, State.Pass = 0, State.Turn = BLACK;
    Record.Index = 0, Record2.Index = -1;

    ClearMark();

    for (int i = 1, k = 0; i <= DATA_HEIGHT; i++) {
        for (int j = 1; j <= DATA_WIDTH; j++, k++) {
            if (i == 1 || i == DATA_HEIGHT || j == 1) State.Table[k] = OUT;
            else State.Table[k] = EMPTY;
        }
    }

    std::vector<GoNode>().swap(Record.Node); //���Record.Node

    GoNode node;
    node.Turn = BLACK;
    node.Pass = 0;
    Record.Node.push_back(node);
}

void Board::ClearMark()
{
    memset(Mark, 0 , sizeof(Mark));
    Path = 0;
}

int Board::GetPoint(int x, int y)
{
    return (y + 1) * DATA_WIDTH + x + 1;
}

int Board::GetColor(int x, int y)
{
    return State.Table[GetPoint(x, y)];
}

int Board::SetHandicap(int n) //��������
{
    if (n < 2 || n > 9) return 0;

    BOARD_HANDICAP = QString::number(n);
    SLIST_CLEAR(Handicap);

    for (int i = 0; i < n; i++) {
        if (i >= 4 && (n == 6 || n == 8)) {
            SLIST_PUSH(Handicap, Star[(i + 1) * 2]);
            SLIST_PUSH(Handicap, Star[(i + 1) * 2 + 1]);
        }
        else {
            SLIST_PUSH(Handicap, Star[i * 2]);
            SLIST_PUSH(Handicap, Star[i * 2 + 1]);
        }
    }

    for (int i = 1; i <= Handicap[0]; i += 2)
        Add(Handicap[i], Handicap[i + 1], BLACK);

    Record.Node.back().Turn = WHITE;
    return n;
}

int Board::CheckBound(int x, int y) //���߽磬�Ƿ���������
{
    return (x >= 0 && x < Width && y >= 0 && y < Height);
}

//�˺������ھ����ԣ������ܶ�Ϊ��ʱ����ֱ��ʹ�ã�������٣�������Ƿ�������xy�Ƿ�Ϸ�
int Board::Add(int x, int y, int color) //��������
{
    int current = 0;
    if (CheckBound(x, y)) {
        current = GetPoint(x, y);
        if (State.Table[current] != EMPTY) return 0;
    }
    else return 0;

    int other = OtherColor(color);
    int cp = 0;

    State.Table[current] = color;
    Record.Node.back().Prop.push_back(MakeProp(TOKEN_ADD, color, x, y));
    Status[current].Move = 0;

    for (int i = 0; i < 4; i++) {
        int around = current + Side[i];
        if (State.Table[around] == other) //�����Χ���ǶԷ����ӣ�����˲��ܷ��Ƴ��Է�����
		{
            cp += Remove(around, other, Record.Node.back()); //around�������������û�б��ᣬRemove����0
        }
    }

    if (cp == 0) //�����Χû�жԷ����ӱ��Ƴ������˵㲻����
        Remove(current, color, Record.Node.back());

    return 1;
}

// Play Board at z for color with bound, ko and empty check
int Board::Play(int x, int y, int color, int mode)
{
    int current = 0;

    if (CheckBound(x, y)) {
        current = GetPoint(x, y);
        if (State.Table[current] != EMPTY || current == State.Ko) return 0; //���ӻ��ڴ��
    }

    if (mode && Record2.Index < 0)
        Record2 = Record; // backup for test play

    Cut(); //����֦

    GoNode node;
    int other = OtherColor(color);
    State.Ko = 0; 

	if (current == 0) //���ҽ���x = y = -1
	{
        State.Pass++;
        node.Prop.push_back(MakeProp(TOKEN_PASS, color, 0, 0));
    }
    else {
        State.Pass = 0;
        State.Table[current] = color;
        GoProp Prop = MakeProp(TOKEN_PLAY, color, x, y, Record.Index + 1);
        node.Prop.push_back(Prop);
        Status[current].Move = Prop.Move;

        int r[4] = {0, 0, 0, 0};
        int size, Ko = 0, cp = 0;

        for (int i = 0; i < 4; i++) // ��Χ״����⣬ֻҪ��һ����enum Color�е�ĳһ�֣���Ӧ��r[]�͵���1
            r[State.Table[current + Side[i]]] = 1;

        for (int i = 0; i < 4; i++) //��������Ƿ��������ܱ����
		{
            int around = current + Side[i];
            if (State.Table[around] == other) 
			{
                size = Remove(around, other, node); //���1����ʱ����1
                if (size == 1) 
					Ko = around; //�п����ڴ��
                cp = cp + size;
            }
        }

        if (cp == 1 && r[color] == 0 && r[0] == 0) //�������ֻ�ᵽ1���ӣ�����Χ�㶼Ϊ�߽��������ӣ�����Ϊ���
            State.Ko = Ko;                         
        if (cp == 0)
            Remove(current, color, node); //Remove�����Լ�����Ƭ���Ӹò��ñ���������벻������
    }

    State.Turn = other;
    node.Turn = State.Turn;
    node.Pass = State.Pass;
    node.Ko = State.Ko;
    Record.Node.push_back(node);
    Record.Index++;
    return 1;
}

int Board::Append(int x, int y, int color, int i)
{
    if (i && (Record.Index == Record.Node.size() - 1))
        return Play(x, y, color);

    int k = Record.Index;
    End();
    int result = Play(x, y, color);
    Undo(Record.Index - k);
    return result;
}

//Remove�����Լ�����Ƭ���Ӹò��ñ���������벻������
int Board::Remove(int z, int color, GoNode &node)
{
    Path++;
    Mark[z] = Path; //�м�����������һ��

    SLIST_CLEAR(Block);
    SLIST_PUSH(Block, z); // point to check

    for (int n = 1; n <= Block[0]; n++) {
        int current = Block[n];
        for (int i = 0; i < 4; i++) {
            int around = current + Side[i];
            int state_around = State.Table[around];
            if (state_around == EMPTY) 
				return 0; // ��Χ�����������Ƴ����˳�����
            if (state_around == color && Mark[around] != Path) // ��Χͬɫ����û�б�ͳ��
			{
                Mark[around] = Path; //�Ѹõ�����ͬһ����
                SLIST_PUSH(Block, around); //�Ѹõ������
            }
        }
    }
	//�����Ƴ������ڵ������嶼��������û����ʱ��������һ��ѭ��,�Ƴ������
    for (int n = 1; n <= Block[0]; n++) {
        int current_ = Block[n];
        State.Table[current_] = EMPTY; //�Ƴ�
        int y = current_ / DATA_WIDTH;
        int x = current_ - y * DATA_WIDTH;
        node.Prop.push_back(MakeProp(TOKEN_TAKE, color, x - 1, y - 1, Status[z].Move));
    }

    return Block[0];
}

int Board::CheckRemove(int z, int color)
{
    Path++;
    Mark[z] = Path;

    SLIST_CLEAR(Block);
    SLIST_PUSH(Block, z); // point to check

    for (int n = 1; n <= Block[0]; n++) {
        int k = Block[n];
        for (int i = 0; i < 4; i++) {
            int k2 = k + Side[i];
            int p = State.Table[k2];
            if (p == EMPTY) 
				return 0; // liberty
            if (p == color && Mark[k2] != Path) 
			{
                Mark[k2] = Path;
                SLIST_PUSH(Block, k2);
            }
        }
    }

    for (int n = 1; n <= Block[0]; n++) {
        int k = Block[n];
        State.Table[k] = EMPTY;
        SLIST_PUSH(Move, k);
    }

    return Block[0];
}

// Play Board at z and returns 1 if ko or fill eye
int Board::CheckPlay(int z, int color, int other)
{
    if (z == State.Ko) return 1;
    int r[4] = { 0, 0, 0, 0 };
    for (int i = 0; i < 4; i++) // round check
        r[State.Table[z + Side[i]]] = 1;
    if (r[0] == 0 && r[other] == 0) // fill eye
        return 1;

    State.Table[z] = color;
    int size, Ko = 0, cp = 0;

    for (int i = 0; i < 4; i++) {
        int k = z + Side[i];
        if (State.Table[k] == other) {
            size = CheckRemove(k, other);
            if (size == 1) {
                Ko = 1;
                State.Ko = k; // may be ko
            }
            cp = cp | size;
        }
    }

    if (cp == 0) {
        if (r[0] == 0 && r[color] == 0) 
		{ // fill eye and suicde
            State.Table[z] = EMPTY;
            return 1;
        }
        CheckRemove(z, color);
    }

    State.Pass = 0;
    if (Ko == 0) State.Ko = 0;
    return 0;
}

void Board::Rand()
{
    int color = State.Turn;
    int other = OtherColor(color);
    int n = BOARD_MOVE_MAX;

    int Move2[MOVE_MAX];

    ClearMark();

    SLIST_CLEAR(Move);
    SLIST_CLEAR(Move2);

    for (int i = DATA_START; i <= DATA_END; i++)
        if (State.Table[i] == EMPTY) 
			SLIST_PUSH(Move, i); // empty moves

    State.Pass = 0;

    while (State.Pass < 2 && n--) 
	{
        if (Move[0] == 0) 
		{
            State.Pass++;
            State.Ko = 0;
        }
        else {
            int i = SLIST_RAND(Move);
            if (CheckPlay(Move[i], color, other)) 
			{
                SLIST_PUSH(Move2, Move[i]);
                SLIST_DELETE(Move, i);
                continue;
            }

            SLIST_DELETE(Move, i);
        }

        SLIST_MERGE(Move, Move2);
        SLIST_CLEAR(Move2);
        other = color;
        color = OtherColor(color);
    }
}

void Board::Rand(int total)
{
    struct State State2 = State;
    int Count[BOARD_MAX];
    memset(Count, 0, sizeof(Mark));

    for (int n = 0; n < total; n++) {
        Rand();
        for (int i = DATA_START; i <= DATA_END; i++) 
		{ 
			// update result
            int k = State.Table[i];
            if (k == BLACK) 
				Count[i]++;
            else if (k == WHITE)
				Count[i]--;
            else if (k == EMPTY) 
			{
                int k2 = State.Table[i+1];
                if (k2 == BLACK)
					Count[i]++;   
                else if (k2 == WHITE) 
					Count[i]--;
                else {
                    k2 = State.Table[i-1];
                    if (k2 == BLACK)
						Count[i]++;
                    else 
						Count[i]--;
                }
            }
        }
        State = State2;
    }

    // Find largest value in block and set block to that value
    ClearMark();

    for (int i = DATA_START; i <= DATA_END; i++) {
        int k = State.Table[i];
        if ((k == BLACK || k == WHITE) && Mark[i] == 0) { // not visit
            Path++;
            Mark[i] = Path;
            SLIST_CLEAR(Block);
            SLIST_PUSH(Block, i);
            int Value = (k == BLACK ? -total : total);

            for (int n = 1; n <= Block[0]; n++) {
                int score = Count[Block[n]];
                if ((k == BLACK && score > Value) ||
                    (k == WHITE && score < Value)) Value = score;
                for (int d = 0; d < 4; d++) {
                    int i2 = Block[n] + Side[d];
                    int k2 = State.Table[i2];
                    if (k2 == k && Mark[i2] == 0) { // not vist
                        Mark[i2] = Path;
                        SLIST_PUSH(Block, i2);
                    }
                }
            }

            for (int n = 1; n <= Block[0]; n++) {
                Count[Block[n]] = Value;
            }
        }
    }

    BLACK_SCORE = 0;
    WHITE_SCORE = 0;

    for (int j = 1, k = 0; j <= DATA_HEIGHT; j++) {
        for (int i = 1; i <= DATA_WIDTH; i++, k++) {
            if (j != 1 && j != DATA_HEIGHT && i != 1) {
                double d = (double) Count[k] / total;
                if (d >= 0.1) { // black
                    Status[k].Label = (State.Table[k] == WHITE ? WHITE_DEAD : BLACK_AREA);
                    Status[k].Color = (d >= 0.5 ? QColor(128, 0, 0, 128) : QColor(64, 0, 0, 64));
                    BLACK_SCORE++;
                }
                else if (d <= -0.1) { // white
                    Status[k].Label = (State.Table[k] == BLACK ? BLACK_DEAD : WHITE_AREA);
                    Status[k].Color = (d <= -0.5 ? QColor(255, 255, 255, 128) : QColor(255, 255, 255, 64));
                    WHITE_SCORE++;
                }
                else {
                    Status[k].Label = BOTH_AREA;
                    Status[k].Color = QColor(255, 255, 255, 0);
                }
            }
        }
    }

    FINAL_SCORE = BLACK_SCORE - WHITE_SCORE - Komi;
    BOARD_SCORE = QString("B %1 W %2 + %3 = %4 RE : %5%6").arg(BLACK_SCORE).arg(WHITE_SCORE).arg(BOARD_KOMI).arg((double) WHITE_SCORE + Komi).arg((FINAL_SCORE > 0 ? "B + " : (FINAL_SCORE < 0 ? "W + " : ""))).arg(fabs(FINAL_SCORE));
}

int Board::Start()
{
    if (Record.Index > 0) {
        while(Undo()) { }
        return 1;
    }

    return 0;
}

int Board::End()
{
    if (Record.Index < Record.Node.size() - 1) {
        while (Forward()) { }
        return 1;
    }

    return 0;
}

int Board::Undo(int k)
{
    if (Record.Index <= 0 || k <= 0) return 0;

    while (Record.Index && k) {
        for (int i = Record.Node[Record.Index].Prop.size() - 1; i >= 0; i--) {
            GoProp Prop = Record.Node[Record.Index].Prop[i];
            if (Prop.Label == TOKEN_PLAY || Prop.Label == TOKEN_ADD)
                State.Table[GetPoint(Prop.Col, Prop.Row)] = 0;
            else if (Prop.Label == TOKEN_TAKE)
                State.Table[GetPoint(Prop.Col, Prop.Row)] = Prop.Value;
        }

        Record.Index--; k--;
        State.Turn = Record.Node[Record.Index].Turn;
        State.Pass = Record.Node[Record.Index].Pass;

        if (Record.Index == Record2.Index) {
            Record = Record2; // restore from test play
            Record2.Index = -1;
        }
    }

    return 1;
}

int Board::Forward(int k)
{
    int last = Record.Node.size() - 1;
    if (Record.Index >= last) return 0;

    while (Record.Index < last && k) {
        Record.Index++;
        int total = Record.Node[Record.Index].Prop.size();
        State.Turn = Record.Node[Record.Index].Turn;
        State.Pass = Record.Node[Record.Index].Pass;

        for (int i = 0; i < total; i++) {
            GoProp Prop = Record.Node[Record.Index].Prop[i];
            int z = GetPoint(Prop.Col, Prop.Row);

            if (Prop.Label == TOKEN_PLAY || Prop.Label == TOKEN_ADD) {
                State.Table[z] = Prop.Value;
                Status[z].Move = Prop.Move;
            }
            else if (Prop.Label == TOKEN_TAKE) {
                State.Table[z] = 0;
            }
        }

        k--;
    }

    return 1;
}

void Board::Cut()
{
    int n = Record.Node.size() - Record.Index - 1;
    while (n--) Record.Node.pop_back();
}

Board::GoProp Board::MakeProp(int label, int value, int x, int y, int index)
{
    GoProp Prop;

    Prop.Label = label;
    Prop.Value = value;
    Prop.Move = index;
    Prop.Col = x;
    Prop.Row = y;

    return Prop;
}

QString Board::GetText(QString &str, int i, int j)
{
    return str.mid(i + 1, j - i - 1);
}

int Board::GetProp(QString &str, int i, int j)
{
    QString str2 = str.mid(i, j - i);
    str2 = str2.toUpper();

    if (str2 == "B")
        return TOKEN_BLACK;
    if (str2 == "W")
        return TOKEN_WHITE;
    if (str2 == "C" || str2 == "TC")
        return TOKEN_COMMENT;
    if (str2 == "SZ")
        return TOKEN_SIZE;
    if (str2 == "PB")
        return TOKEN_PLAY_BLACK;
    if (str2 == "PW")
        return TOKEN_PLAY_WHITE;
    if (str2 == "AB")
        return TOKEN_ADD_BLACK;
    if (str2 == "AW")
        return TOKEN_ADD_WHITE;
    if (str2 == "EV" || str2 == "TE")
        return TOKEN_EVENT;
    if (str2 == "LB")
        return TOKEN_LABEL;
    if (str2 == "MA")
        return TOKEN_MARK;
    if (str2 == "TR")
        return TOKEN_TRIANGLE;
    if (str2 == "CR")
        return TOKEN_CIRCLE;
    if (str2 == "SQ")
        return TOKEN_SQUARE;
    if (str2 == "DT" || str2 == "RD")
        return TOKEN_DATE;
    if (str2 == "KM" || str2 == "KO")
        return TOKEN_KOMI;
    if (str2 == "RE")
        return TOKEN_RESULT;
    if (str2 == "HA")
        return TOKEN_HANDICAP;

    return TOKEN_NONE;
}

int Board::Read(const QString &str, int k)
{
    QFile file(str);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return 0;
    QTextStream Stream(&file);
    QString buffer = Stream.readAll();
    file.close();

    int fsize = buffer.size();
    if (fsize <= 0) return 0;

    int token = TOKEN_NONE;
    int v1 = -1, v2 = 0;
    int p = -1, q = -2;

    Reset();
    Komi = 7.5;

    BOARD_FILE = str; BOARD_KOMI = "7.5";
    BOARD_EVENT.clear(); BOARD_DATE.clear();
    PLAYER_BLACK.clear(); PLAYER_WHITE.clear();
    BLACK_LEVEL.clear(); WHITE_LEVEL.clear();
    BOARD_HANDICAP.clear(); BOARD_RESULT.clear();

    for (int i = 0; i < fsize; i++) {
        QChar c = buffer[i];

        if (c == '[' && v1 < 0) { // '[' not in []
            v1 = i;
        }
        else if (c == ')' && v1 < 0) { // ')' not in []
            break;
        }
        else if (c == '\\') {
            if (q >= 0 && i == q + 1) q = -2; // double slashes, ignore it. if q = -1 then i == q + 1 may be true
            else q = i;                       // position of '\'
        }
        else if (c == ']' && i != q + 1 && v1 >= 0) { // not '\]' and have '['
            v2 = i;
            if (p >= 0) token = GetProp(buffer, p, v1); // use previous token for second property

            if (token == TOKEN_BLACK || token == TOKEN_WHITE) {
                int color = (token == TOKEN_BLACK ? BLACK : WHITE);
                if (v2 - v1 == 3) {
                    Play(buffer[v1 + 1].toLower().toLatin1() - 'a', buffer[v1 + 2].toLower().toLatin1() - 'a', color);
                }
                else Play(-1, -1, color); // pass
            }
            else if (token == TOKEN_ADD_BLACK || token == TOKEN_ADD_WHITE) {
                int color = (token == TOKEN_ADD_BLACK ? BLACK : WHITE);
                if (v2 - v1 == 3) {
                    Add(buffer[v1 + 1].toLower().toLatin1() - 'a', buffer[v1 + 2].toLower().toLatin1() - 'a', color);
                }
            }
            else if (token == TOKEN_LABEL) {
                if (v2 - v1 >= 5 && buffer[v1 + 3] == ':') {
                    GoProp Prop;
                    Prop.Label = TOKEN_LABEL;
                    Prop.Value = buffer[v1 + 4].toLatin1();
                    Prop.Col = buffer[v1 + 1].toLower().toLatin1() - 'a';
                    Prop.Row = buffer[v1 + 2].toLower().toLatin1() - 'a';
                    Record.Node[Record.Index].Prop.push_back(Prop);
                }
            }
            else if (token == TOKEN_MARK || token == TOKEN_TRIANGLE || token == TOKEN_CIRCLE || token == TOKEN_SQUARE) {
                if (v2 - v1 == 3) {
                    GoProp Prop;
                    Prop.Label = token;
                    Prop.Col = buffer[v1 + 1].toLower().toLatin1() - 'a';
                    Prop.Row = buffer[v1 + 2].toLower().toLatin1() - 'a';
                    Record.Node[Record.Index].Prop.push_back(Prop);
                }
            }
            else if (token == TOKEN_SIZE) {
                Size = GetText(buffer, v1, v2).toInt();
                Reset();
            }
            else if (token == TOKEN_COMMENT) {
                QString Text = GetText(buffer, v1, v2);
                Text.replace("\\\\", "\\");
                Text.replace("\\]", "]");
                Record.Node[Record.Index].Text += Text;
            }
            else if (token == TOKEN_EVENT)
                BOARD_EVENT = GetText(buffer, v1, v2);
            else if (token == TOKEN_DATE)
                BOARD_DATE = GetText(buffer, v1, v2);
            else if (token == TOKEN_RESULT)
                BOARD_RESULT = GetText(buffer, v1, v2);
            else if (token == TOKEN_PLAY_BLACK)
                PLAYER_BLACK = GetText(buffer, v1, v2);
            else if (token == TOKEN_PLAY_WHITE)
                PLAYER_WHITE = GetText(buffer, v1, v2);
            else if (token == TOKEN_KOMI) {
                BOARD_KOMI = GetText(buffer, v1, v2);
                Komi = BOARD_KOMI.toDouble();
            }
            else if (token == TOKEN_HANDICAP) {
                BOARD_HANDICAP = GetText(buffer, v1, v2);
                if (BOARD_HANDICAP.toInt() <= 1) BOARD_HANDICAP.clear();
            }

            p = v1 = -1; // reset
        }
        else if (c == ' ' || c == '(' || c == ')' || c == ';' || c == '\n' || c == '\r' || c == '\f') {

        }
        else {
            if (p < 0 && v1 < 0) { // is reset
               p = i;
            }
        }
    }

    if (k > 0) while (k < Record.Index) Undo();
    else Start();
    return 1;
}

int Board::Write(const QString &str)
{
    QFile file(str);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return 0;
    QTextStream Stream(&file);

    Stream << QString("(;SZ[%1]KM[%2]").arg(QString::number(Size), BOARD_KOMI);
    if (!BOARD_HANDICAP.isEmpty()) Stream << "HA[" << BOARD_HANDICAP << "]";
    Stream << QString("PB[%3]PW[%4]DT[%5]RE[%6]").arg(PLAYER_BLACK.left(64) + (BLACK_LEVEL.isEmpty() ? "" : " ") + BLACK_LEVEL.left(32),
        PLAYER_WHITE.left(64) + (WHITE_LEVEL.isEmpty() ? "" : " ") + WHITE_LEVEL.left(32), BOARD_DATE, BOARD_RESULT);

    for (int i = 0; i < Record.Node.size(); i++) {
        int token = TOKEN_NONE;

        for (int k = 0; k < Record.Node[i].Prop.size(); k++) {
            GoProp Prop = Record.Node[i].Prop[k];

            if (Prop.Label == TOKEN_ADD) {
                if (Prop.Value == BLACK) {
                    if (token != TOKEN_ADD_BLACK) Stream << "AB";
                    token = TOKEN_ADD_BLACK;
                }
                else if (Prop.Value == WHITE) {
                    if (token != TOKEN_ADD_WHITE) Stream << "AW";
                    token = TOKEN_ADD_WHITE;
                }

                Stream << QString("[%1%2]").arg(QChar('a' + Prop.Col), QChar('a' + Prop.Row));
            }
            else if (Prop.Label == TOKEN_PLAY) {
                Stream << ";" << (Prop.Value == BLACK ? "B" : "W") <<
                    QString("[%1%2]").arg(QChar('a' + Prop.Col), QChar('a' + Prop.Row));
                token = TOKEN_PLAY;
            }
            else if (Prop.Label == TOKEN_PASS) {
                Stream << ";" << (Prop.Value == BLACK ? "B" : "W") << "[]";
                token = TOKEN_PASS;
            }
        }
    }

    Stream << ")";
    file.close();
    return 1;
}
