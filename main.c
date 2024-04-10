#include "raylib.h"
#include "raymath.h"
#include <string.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> 
#include <mysql/mysql.h>

// ui elements
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "resources/style_cyber.h"

// files
#define MAX_FILES 50

// console
#define MAX_INPUT_CHARS 27
#define MAX_CONSOLE_BUFFER 1080 * 1080
char consoleBuffer[MAX_CONSOLE_BUFFER + 1] = {0};

// to be set to display logs onto textbox display
void CustomLog(int msgType, const char *text, va_list args)
{
  char timeStr[64] = { 0 };
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);

  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm_info);
  printf("[%s] ", timeStr);

  switch (msgType)
  {
    case LOG_INFO: printf("[INFO] : "); break;
    case LOG_ERROR: printf("[ERROR]: "); break;
    case LOG_WARNING: printf("[WARN] : "); break;
    case LOG_DEBUG: printf("[DEBUG]: "); break;
    default: break;
  }

  strcpy(consoleBuffer, text);

  vprintf(consoleBuffer, args);

  printf("\n");
}


int main(void)
{
  SetTraceLogCallback(CustomLog);

  // variables sql
  MYSQL *conn = NULL;
  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;
  
  char *server = "localhost";
  char *user = "root";
  char *password = "root1875";
  char *database = "testdb";

  conn = mysql_init(NULL);

  // check connection to db
  // log info for successful connection
  // otherwise log error to console
  if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0))
  {
    TraceLog(LOG_ERROR, mysql_error(conn));
    exit(1);
  }
  TraceLog(LOG_INFO, "Database Connected...\n");

  // variables console
  char consoleInput[MAX_INPUT_CHARS + 1] = "\0";
  int letterCount = 0;
  bool mouseOnText = false;
  Rectangle textBox = {260, 455, 670, 50};

  // variables screen
  const int screenWidth = 960;
  const int screenHeight = 560;

  InitWindow(screenWidth, screenHeight, "Dashboard");
  GuiLoadStyleCyber();

  // variables list view
  int listViewExScrollIndex = 0;
  int listViewExActive = 0;
  int listViewExFocus = -1;
  char *listViewExList[MAX_FILES] = {0};

  // variables drag and drop
  int fileCount = 0;
  char *filePath[MAX_FILES] = {0};

  // allocate space for paths
  for (int i = 0; i < MAX_FILES; i++)
  {
    filePath[i] = (char *)RL_CALLOC(MAX_FILES, 1);
    listViewExList[i] = (char *)RL_CALLOC(MAX_FILES, 1);
  }

  while (!WindowShouldClose())
  {
    if (IsFileDropped())
    {
      FilePathList droppedFiles = LoadDroppedFiles();

      if (fileCount == MAX_FILES)
      {
        TraceLog(LOG_ERROR, "Max File Reached...\n");
      }
      else
      {
        for (int i = 0, offset = fileCount; i < (int)droppedFiles.count; i++)
        {
          TextCopy(filePath[offset + i], droppedFiles.paths[i]);
          DrawText(TextFormat("%i", fileCount++), 210, 520, 20, LIGHTGRAY);

          // debug info for files dropped
          TraceLog(LOG_INFO, filePath[offset + i]);
          TraceLog(LOG_INFO, GetFileNameWithoutExt(filePath[offset + i]));
          TraceLog(LOG_INFO, GetDirectoryPath(filePath[offset + i]));
          TraceLog(LOG_INFO, GetFileExtension(filePath[offset + i]));

          TextCopy(listViewExList[offset + i], GetFileNameWithoutExt(filePath[offset + i]));

          // format id, name, path, extension
          // into sql query and update to database
          const char *query = TextFormat(
              //                                               datetime
              "INSERT INTO files VALUES(%d, '%s', '%s', '%s', NOW())",
              offset,                                       // id
              GetFileNameWithoutExt(filePath[offset + i]),  // filename
              GetDirectoryPath(filePath[offset + i]),       // filepath
              GetFileExtension(filePath[offset + i]));      // extension

          // NOTE: drag and drop simultaneously crashes app
          //       gdb says thread ... (lwp) exited
          //       seems like multiple drops crashes the app
          //       sometimes works so cannot really determine
          if (mysql_query(conn, query))
          {
            TraceLog(LOG_ERROR, query);
            mysql_close(conn);
            exit(1);
          }
        }
      }

      UnloadDroppedFiles(droppedFiles);
    }

    if (CheckCollisionPointRec(GetMousePosition(), textBox))
    {
      mouseOnText = true;
    }
    else
    {
      mouseOnText = false;
    }

    if (mouseOnText)
    {
      SetMouseCursor(MOUSE_CURSOR_IBEAM);
      int key = GetCharPressed();

      while (key > 0)
      {
        if ((key >= 32) && (key <= 125) && (letterCount < MAX_INPUT_CHARS))
        {
          consoleInput[letterCount] = (char)key;
          consoleInput[letterCount + 1] = '\0';
          letterCount++;
        }
        key = GetCharPressed();
      }

      // ==========INCOMPLETE==========
      // copy char array being entered from console
      // into str variable to update to textinput
      // processes sql inputs
      // selective commands:
      // SELECT ALL = SELECT * FROM files;
      if (IsKeyPressed(KEY_ENTER))
      {
        char *str[MAX_INPUT_CHARS];
        strncpy(str, consoleInput, MAX_INPUT_CHARS);
        str[MAX_INPUT_CHARS] = '\0';

        TraceLog(LOG_INFO, str);

        if (strcmp(TextToUpper(str), "SELECT ALL") == 0)
        {
          const char *query = "SELECT * FROM files;";
          if (mysql_query(conn, query))
          {
            TraceLog(LOG_ERROR, query);
            mysql_close(conn);
            exit(1);
          }

          MYSQL_RES *result = mysql_store_result(conn);

          if (result == NULL)
          {
            TraceLog(LOG_ERROR, query);
            mysql_close(conn);
            exit(1);
          }

          int num_fields = mysql_num_fields(result);

          MYSQL_ROW row;
          MYSQL_FIELD *field;

          while ((row = mysql_fetch_row(result)))
          {
            for (int i = 0; i < num_fields; i++)
            {
              // NOTE: seems to be related 
              //       to SetTraceLogCallback(...)
              //       for custom logging
              TraceLog(LOG_INFO, (row[i] ? row[i] : "NULL"));
            }
            strncpy(consoleBuffer, row, num_fields);
          }
        }
      }

      // delete characters
      if (IsKeyPressed(KEY_BACKSPACE))
      {
        letterCount--;
        if (letterCount < 0)
        {
          letterCount = 0;
        }
        consoleInput[letterCount] = '\0';
      }
    }
    else
    {
      SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }

    BeginDrawing();

      ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

      // console
      GuiSetStyle(DEFAULT, TEXT_WRAP_MODE, TEXT_WRAP_WORD);
      GuiTextBox((Rectangle){260, 25, 670, 430}, consoleBuffer, MAX_CONSOLE_BUFFER, false);
      
      // console textinput
      DrawText(consoleInput, (int)textBox.x + 5, (int)textBox.y + 8, 40, MAROON);
      if (mouseOnText)
      {
        DrawRectangleLines((int)textBox.x, (int)textBox.y, (int)textBox.width, (int)textBox.height, RED);

        if (letterCount < MAX_INPUT_CHARS)
        {
          DrawText("_", (int)textBox.x + 8 + MeasureText(consoleInput, 40), (int)textBox.y + 12, 40, MAROON);
        }
      }
      else
      {
        DrawRectangleLines((int)textBox.x, (int)textBox.y, (int)textBox.width, (int)textBox.height, DARKGRAY);
      }

      // list view
      GuiListViewEx((Rectangle){35, 25, 200, 480}, listViewExList, MAX_FILES, &listViewExScrollIndex, &listViewExActive, &listViewExFocus);
      DrawText("TOTAL FILES : ", 35, 520, 20, LIGHTGRAY);
      DrawText(TextFormat("%i", fileCount), 210, 520, 20, LIGHTGRAY);

    EndDrawing();
  }

  // delete all records once window is closed
  const char *queryDelete = "DELETE FROM files;";
  if (mysql_query(conn, queryDelete))
  {
    TraceLog(LOG_ERROR, queryDelete);
    mysql_close(conn);
    exit(1);
  }

  mysql_free_result(res);
  mysql_close(conn);
  CloseWindow();

  TraceLog(LOG_INFO, "All Database Files Cleared ...\n");
  TraceLog(LOG_INFO, "Database Connection Closed Successfully...\n");
  return 0;
}