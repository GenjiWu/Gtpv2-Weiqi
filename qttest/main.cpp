#include "Window.h"

int main(int argc, char *argv[])
{
    QApplication App(argc, argv);
    QStringList args = QCoreApplication::arguments();
    Window Main;
	
    if (argc == 2 || argc == 3) {
        if (!args[1].startsWith("-"))  //´ò¿ªSGFÆåÆ×
		{
            Main.Child_Widget->Mode = BOARD_FILE;
            Main.CreateDock();
			Main.CreateMenu();
            Main.Child_Widget->Read(args[1], (argc == 2 ? 0 : args[2].toInt()));
        }
    }

    if (argc > 2 && Main.Child_Widget->Mode == 0) //¶ÔÞÄ
	{
        QString str[2], arg[2];
        int color = EMPTY;

        for (int i = 1; i < argc; i++) 
		{
            if (args[i] == "-black") 
			{
                color = BLACK;
            }
            else if (args[i] == "-white") 
			{
                color = WHITE;
            }
            else if (args[i] == "-size") 
			{
                if (i + 1 < argc) 
				{
                    int size = args[i + 1].toInt();
                    Main.Child_Widget->Child_Board.Reset();
                    i++;
                }
            }
            else if (args[i] == "-komi") 
			{
                if (i + 1 < argc) 
				{
                    Main.Child_Widget->Child_Board.BOARD_KOMI = args[i + 1];
                    Main.Child_Widget->Child_Board.Komi = Main.Child_Widget->Child_Board.BOARD_KOMI.toDouble();
                    i++;
                }
            }
            else if (args[i] == "-handicap") 
			{
                if (i + 1 < argc) 
				{
                    Main.Child_Widget->Child_Board.BOARD_HANDICAP = args[i + 1];
                    i++;
                }
            }
            else {
                if (color == BLACK) 
				{
                    if (str[0].isEmpty()) str[0] = args[i];
                    else arg[0] = args[i];
                }
                else if (color == WHITE) 
				{
                    if (str[1].isEmpty()) str[1] = args[i];
                    else arg[1] = args[i];
                }
            }
        }

        // Start //
        if (color != EMPTY) 
		{
            if (!str[0].isEmpty()) 
			{
                Main.Child_Widget->Side |= BLACK; // AIµçÄÔÖ´ºÚ
                Main.Child_Widget->Play[BLACK] = new Player(Main.Child_Widget, BLACK, Main.Child_Widget->Child_Board.Size, Main.Child_Widget->Child_Board.Komi);
                Main.Child_Widget->Play[BLACK]->Setup(str[0], arg[0]);
            }
            if (!str[1].isEmpty()) 
			{
                Main.Child_Widget->Side |= WHITE; // AIµçÄÔÖ´°×
                Main.Child_Widget->Play[WHITE] = new Player(Main.Child_Widget, WHITE, Main.Child_Widget->Child_Board.Size, Main.Child_Widget->Child_Board.Komi);
                Main.Child_Widget->Play[WHITE]->Setup(str[1], arg[1]);
            }

            Main.Child_Widget->Mode = BOARD_PLAY;
            Main.CreateDock();
			Main.CreateMenu();
            if (!Main.Child_Widget->Child_Board.BOARD_HANDICAP.isEmpty()) 
			{
                if (Main.Child_Widget->Child_Board.SetHandicap(Main.Child_Widget->Child_Board.BOARD_HANDICAP.toInt())) 
				{
                    Main.Child_Widget->Child_Board.State.Turn = WHITE;
                    for (int i = 1; i <= Main.Child_Widget->Child_Board.Handicap[0]; i += 2) 
					{
                        if (Main.Child_Widget->Play[BLACK])
                            Main.Child_Widget->Play[BLACK]->Play(Main.Child_Widget->Child_Board.Handicap[i], Main.Child_Widget->Child_Board.Handicap[i + 1], Main.Child_Widget->Child_Board.Size, BLACK);
                        if (Main.Child_Widget->Play[WHITE])
                            Main.Child_Widget->Play[WHITE]->Play(Main.Child_Widget->Child_Board.Handicap[i], Main.Child_Widget->Child_Board.Handicap[i + 1], Main.Child_Widget->Child_Board.Size, BLACK);
                    }
                }
                else Main.Child_Widget->Child_Board.BOARD_HANDICAP.clear();
            }

            if (Main.Child_Widget->Play[BLACK]) 
			{
                if (Main.Child_Widget->Child_Board.BOARD_HANDICAP.isEmpty())
                    Main.Child_Widget->Play[BLACK]->Append("genmove b");
                Main.Child_Widget->Play[BLACK]->Send();
            }
            if (Main.Child_Widget->Play[WHITE]) 
			{
                if (!Main.Child_Widget->Child_Board.BOARD_HANDICAP.isEmpty())
                    Main.Child_Widget->Play[WHITE]->Append("genmove w");
                Main.Child_Widget->Play[WHITE]->Send();
            }
        }
    }

    if (Main.Child_Widget->Mode == 0) 
	{
        Main.CreateDock();
    }

    Main.show();
    Main.Child_Widget->setFocus();
    Main.showMaximized();
	
    return App.exec();
}
