#ifndef _BOARD_H_
#define _BOARD_H_

#include <QtCore>
#include <QColor>

#define MAX_SIZE 19
#define MIN_SIZE 2

#define BOARD_MAX (MAX_SIZE + 1) * (MAX_SIZE + 2) + 1
#define MOVE_MAX  MAX_SIZE * MAX_SIZE + 2
#define BOARD_MOVE_MAX MAX_SIZE * MAX_SIZE * 2

inline void _CLEAR(int *ls) { ls[0] = 0; }
inline void _PUSH(int *ls, int i) { ls[++ls[0]] = i; }
inline int _POP(int *ls) { return ls[ls[0]--]; }
inline void _ADD(int *ls, int i) {
    for (int k = 1; k <= ls[0]; k++) if (ls[k] == i) return;
    ls[++ls[0]] = i; }
inline void _DELETE(int *ls, int t) { ls[t] = ls[ls[0]--]; } //第t项等于被删除的最后一项
inline int _RAND(int *ls) { return rand() % ls[0] + 1; } //返回一个随机下标
inline void _MERGE(int *ls, int *ls2) //合并
{
    for (int k = 1; k <= ls2[0]; k++) ls[++ls[0]] = ls2[k]; 
}
inline void _PRINT(int *ls) 
{
    for (int k = 1; k <= ls[0]; k++) printf("%d ", ls[k]); printf("\n"); 
}

enum Color
{
    EMPTY, BLACK, WHITE, OUT
};

enum 
{
    BOTH_AREA, BLACK_AREA, WHITE_AREA,
    BLACK_DEAD, WHITE_DEAD
};

class Board
{
public:

    enum
    {
        PASS, RESIGN
    };

    enum
    {
        TOKEN_NONE, TOKEN_BLACK, TOKEN_WHITE, TOKEN_ADD, TOKEN_ADD_BLACK, TOKEN_ADD_WHITE, TOKEN_PLAY,
        TOKEN_TAKE, TOKEN_PASS, TOKEN_LABEL, TOKEN_NUMBER, TOKEN_MARK, TOKEN_TRIANGLE, TOKEN_CIRCLE,
        TOKEN_SQUARE, TOKEN_PLAY_BLACK, TOKEN_PLAY_WHITE, TOKEN_COMMENT, TOKEN_TIME, TOKEN_EVENT,
        TOKEN_PLACE, TOKEN_DATE, TOKEN_RESULT, TOKEN_SIZE, TOKEN_KOMI, TOKEN_HANDICAP
    };

    inline static int OtherColor(int color)
    {
        return color ^ 3;
    }

    struct BoardState
    {
        int ColorTable[BOARD_MAX]; //enum Color
        int Ko, Turn, Pass; //Ko打劫 Turn轮到谁下 Pass累计Pass次数
    };

    struct BStatus
    {
        QColor Color;
        int Move;
        int Label;
    };

    struct GoOperation
    {
        int Label, Value;
        int Col, Row;
		int Move;
    };

    struct GoNode // 记录
    {
        std::vector<GoOperation> Prop;
        QString Text;
        int Ko, Turn, Pass;
    };

    struct Tree
    {
        std::vector<GoNode> Node;
        int Index;
    };

    Board();

    void Clear();
    void ClearMark();
    void Reset();

    int Read(const QString &str, int k = 0);
    int Write(const QString &str);

    int GetColor(int x, int y);
    int GetPoint(int x, int y);
    int CheckBound(int x, int y);
    int Play(int x, int y, int color, int mode = 0);
    int Append(int x, int y, int color, int i = 1);
    int Add(int x, int y, int color);
    int Remove(int z, int color, GoNode &node);
    int CheckPlay(int z, int color, int other);
    int CheckRemove(int z, int color);
    int SetHandicap(int n);

    int GetProp(QString &str, int i, int j);
    QString GetText(QString &str, int i, int j);
    GoOperation MakeProp(int label, int value, int x, int y, int index = 0);

    int Start();
    int Forward(int k = 1);
    int End();
    int Undo(int k = 1);

    void Cut();
    void Judge();
    void Judge(int total);

    int Width, Height, Size;
    int Path;

    int Mark[BOARD_MAX];
    int Move[MOVE_MAX];
    int Block[MOVE_MAX];

    int Side[8];
    int Star[18];
    int Handicap[20];

    int BLACK_SCORE;
    int WHITE_SCORE;

    double FINAL_SCORE;
    double Komi;

    int DATA_WIDTH, DATA_HEIGHT;
    int DATA_START, DATA_END;

	double shift[MAX_SIZE];

    BStatus Status[BOARD_MAX];
    Tree Record, Record2;
    BoardState State;

    QString BOARD_EVENT, BOARD_DATE;
    QString PLAYER_BLACK, PLAYER_WHITE;
    QString BLACK_LEVEL, WHITE_LEVEL;
    QString BOARD_HANDICAP, BOARD_KOMI;
    QString BOARD_RESULT, BOARD_FILE;
    QString BOARD_SCORE;

};
#endif
