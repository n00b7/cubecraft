#include "global.h"
#include "drawing.h"
#include "field.h"
#include "file.h"
#include "keyboard.h"
#include "main.h"
#include "menu.h"
#include "text.h"
#include "title_screen.h"
#include "version.h"
#include "world.h"

#define MAX_SAVE_FILES 5

//Title TPL data
#include "title_tpl.h"
#include "title.h"

#define TITLE_BANNER_WIDTH 432
#define TITLE_BANNER_HEIGHT 72

static TPLFile titleTPL;
static GXTexObj titleTexture;
static unsigned int pressStartBlinkCounter;

static struct MenuItem mainMenuItems[] = {
    {"Start Game"},
#if defined(PLATFORM_WII)
    {"Exit to Homebrew Channel"},
#elif defined(PLATFORM_GAMECUBE)
    {"Reset GameCube"},
#endif
};

static struct Menu mainMenu = {
    "Main Menu",
    mainMenuItems,
    ARRAY_LENGTH(mainMenuItems),
};

static char saveFiles[MAX_SAVE_FILES][SAVENAME_MAX];
static char saveFileLabels[MAX_SAVE_FILES][SAVENAME_MAX + 3];  //+3 is for the 
static struct MenuItem filesMenuItems[MAX_SAVE_FILES + 1];
static int fileNum;

static struct Menu filesMenu = {
    "Select A File",
    filesMenuItems,
    ARRAY_LENGTH(filesMenuItems),
};

static struct MenuItem newgameMenuItems[] = {
    {"Enter Name"},
    {"Enter Seed"},
    {"Start!"},
    {"Back"},
};

static struct Menu newgameMenu = {
    "New Game",
    newgameMenuItems,
    ARRAY_LENGTH(newgameMenuItems),
};

static struct MenuItem startgameMenuItems[] = {
    {"Start!"},
    {"Erase File"},
    {"Back"},
};

static struct Menu startgameMenu = {
    "Start Game",
    startgameMenuItems,
    ARRAY_LENGTH(startgameMenuItems),
};

static struct MenuItem eraseconfirmMenuItems[] = {
    {"Yes"},
    {"No"},
};

static struct Menu eraseconfirmMenu = {
    "Erase this file?",
    eraseconfirmMenuItems,
    ARRAY_LENGTH(eraseconfirmMenuItems),
};

static void start_title_screen(void);
static void main_menu_init(void);
static void files_menu_init(void);
static void newgame_menu_init(void);
static void startgame_menu_init(void);

float bkgndAngle = 0.0;

static void draw_title_banner(void)
{
    int x = (gDisplayWidth - TITLE_BANNER_WIDTH) / 2;
    int y = 100;
    
    drawing_set_fill_texture(&titleTexture, TITLE_BANNER_WIDTH, TITLE_BANNER_HEIGHT);
    drawing_draw_textured_rect(x, y, TITLE_BANNER_WIDTH, TITLE_BANNER_HEIGHT);
}

static void draw_version_copyright(void)
{
    text_init();
    text_set_font_size(8, 16);
    text_draw_string(gDisplayWidth / 2, 400, TEXT_HCENTER, "Version "VERSION);
    text_draw_string(gDisplayWidth / 2, 416, TEXT_HCENTER, "Copyright (C) 2016 Cameron Hall (camthesaxman)");
}

static void draw_title_background(void)
{
    Mtx posMtx;
    Mtx rotMtx;
    guVector axis;
    
    drawing_set_3d_mode();
    guMtxIdentity(posMtx);
    axis = (guVector){0.0, 1.0, 0.0};
    guMtxRotAxisDeg(rotMtx, &axis, bkgndAngle);
    guMtxApplyTrans(posMtx, posMtx, 0.0, -70.0, 0.0);
    guMtxConcat(rotMtx, posMtx, posMtx);
    GX_LoadPosMtxImm(posMtx, GX_PNMTX0);
    world_render_chunks_at(0, 0);
    
    bkgndAngle += 0.05;
    if (bkgndAngle >= 360.0)
        bkgndAngle = 0.0;
    
    drawing_set_2d_mode();
    draw_title_banner();
    draw_version_copyright();
}

//==================================================
// Start Game Menu
//==================================================

static void eraseconfirm_menu_main(void)
{
    switch (menu_process_input())
    {
        case 0:  //Yes
            file_delete(saveFiles[fileNum]);
        case MENU_CANCEL:
        case 1:  //No
            menu_wait_close_anim(files_menu_init);
    }
}

static void eraseconfirm_menu_draw(void)
{
    draw_title_background();
    menu_draw();
}

static void eraseconfirm_menu_init(void)
{
    menu_init(&eraseconfirmMenu);
    set_main_callback(eraseconfirm_menu_main);
    set_draw_callback(eraseconfirm_menu_draw);
}

static void startgame_menu_main(void)
{
    switch (menu_process_input())
    {
        case 0:  //Start!
            world_close();
            assert(strlen(saveFiles[fileNum]) > 0);
            file_load_world(saveFiles[fileNum]);
            field_init();
            break;
        case 1:  //Erase File
            menu_wait_close_anim(eraseconfirm_menu_init);
            break;
        case MENU_CANCEL:
        case 2:  //Back
            menu_wait_close_anim(files_menu_init);
            break;
    }
}

static void startgame_menu_draw(void)
{
    draw_title_background();
    menu_draw();
}

static void startgame_menu_init(void)
{
    menu_init(&startgameMenu);
    set_main_callback(startgame_menu_main);
    set_draw_callback(startgame_menu_draw);
}

//==================================================
// New Game Menu
//==================================================

static char nameKeyboardBuffer[SAVENAME_MAX];
static char seedKeyboardBuffer[SEED_MAX];

static bool check_if_already_exists(const char *name)
{
    if (!strstricmp(name, nameKeyboardBuffer))
    {
        menu_msgbox_init("File already exists.");
        return false;
    }
    else
    {
        return true;
    }
}

static void name_kb_main(void)
{
    if (menu_msgbox_is_open())
    {
        menu_msgbox_process_input();
    }
    else
    {
        switch (keyboard_process_input())
        {
            case KEYBOARD_OK:
                file_enumerate(check_if_already_exists);
                if (!menu_msgbox_is_open())
                {
                    memset(gSaveFile.name, '\0', sizeof(gSaveFile.name));
                    strcpy(gSaveFile.name, nameKeyboardBuffer);
                    newgame_menu_init();
                }
                break;
            case KEYBOARD_CANCEL:
                newgame_menu_init();
                break;
        }
    }
}

static void name_kb_draw(void)
{
    draw_title_background();
    keyboard_draw();
    menu_draw();
}

static void name_kb_init(void)
{
    memset(nameKeyboardBuffer, '\0', sizeof(nameKeyboardBuffer));
    strcpy(nameKeyboardBuffer, gSaveFile.name);
    keyboard_init("Enter World Name", nameKeyboardBuffer, ARRAY_LENGTH(nameKeyboardBuffer));
    set_main_callback(name_kb_main);
    set_draw_callback(name_kb_draw);
}

static void seed_kb_main(void)
{
    switch (keyboard_process_input())
    {
        case KEYBOARD_OK:
            memset(gSaveFile.seed, '\0', sizeof(gSaveFile.seed));
            strcpy(gSaveFile.seed, seedKeyboardBuffer);
            newgame_menu_init();
            break;
        case KEYBOARD_CANCEL:
            newgame_menu_init();
            break;
    }
}

static void seed_kb_draw(void)
{
    draw_title_background();
    keyboard_draw();
}

static void seed_kb_init(void)
{
    memset(seedKeyboardBuffer, '\0', sizeof(seedKeyboardBuffer));
    strcpy(seedKeyboardBuffer, gSaveFile.seed);
    keyboard_init("Enter World Seed", seedKeyboardBuffer, ARRAY_LENGTH(seedKeyboardBuffer));
    set_main_callback(seed_kb_main);
    set_draw_callback(seed_kb_draw);
}

static void newgame_menu_main(void)
{
    if (menu_msgbox_is_open())
    {
        menu_msgbox_process_input();
    }
    else
    {
        switch (menu_process_input())
        {
            case 0:  //Enter Name
                menu_wait_close_anim(name_kb_init);
                break;
            case 1:  //Enter Seed
                menu_wait_close_anim(seed_kb_init);
                break;
            case 2:  //Start!
                if (strlen(gSaveFile.name) == 0)
                {
                    menu_msgbox_init("You must enter a name.");
                    break;
                }
                if (strlen(gSaveFile.seed) == 0)
                {
                    menu_msgbox_init("You must enter a seed.");
                    break;
                }
                
                world_close();
                //Initialize player's starting position
                gSaveFile.spawnX = 5;
                gSaveFile.spawnY = 200;
                gSaveFile.spawnZ = 5;
                gSaveFile.modifiedChunks = NULL;
                gSaveFile.modifiedChunksCount = 0;
                memset(gSaveFile.inventory, 0, sizeof(gSaveFile.inventory));
                file_save_world();
                assert(strlen(gSaveFile.name) > 0);
                assert(strlen(gSaveFile.seed) > 0);
                field_init();
                break;
            case MENU_CANCEL:
            case 3:  //Back
                menu_wait_close_anim(files_menu_init);
                break;
        }
    }
}

static void newgame_menu_draw(void)
{
    draw_title_background();
    menu_draw();
}

static void newgame_menu_init(void)
{
    menu_init(&newgameMenu);
    set_main_callback(newgame_menu_main);
    set_draw_callback(newgame_menu_draw);
}

//==================================================
// Files Menu
//==================================================

static void files_menu_main(void)
{
    int item;
    
    item = menu_process_input();
    if (item == MENU_NORESULT)
        return;
    if (item == MAX_SAVE_FILES || item == MENU_CANCEL)  //Back
    {
        menu_wait_close_anim(main_menu_init);
    }
    else
    {
        if (saveFiles[item][0] == '\0')  //This is an empty save file slot
        {
            memset(gSaveFile.name, '\0', sizeof(gSaveFile.name));
            memset(gSaveFile.seed, '\0', sizeof(gSaveFile.seed));
            menu_wait_close_anim(newgame_menu_init);
        }
        else
        {
            fileNum = item;
            menu_wait_close_anim(startgame_menu_init);
        }
    }
}

static void files_menu_draw(void)
{
    draw_title_background();
    menu_draw();
}

static bool enum_files_callback(const char *name)
{
    if (fileNum == MAX_SAVE_FILES)
    {
        return false;
    }
    else
    {
        assert(strlen(name) < SAVENAME_MAX);
        strcpy(saveFiles[fileNum], name);
        sprintf(saveFileLabels[fileNum], "%i. %s", fileNum + 1, name);
        fileNum++;
        return true;
    }
}

static void files_menu_init(void)
{
    for (int i = 0; i < MAX_SAVE_FILES; i++)
        sprintf(saveFileLabels[i], "asdkjfgh");
    
    //Populate file menu
    fileNum = 0;
    file_enumerate(enum_files_callback);
    //Add empty labels for the remaining slots
    while (fileNum < MAX_SAVE_FILES)
    {
        saveFiles[fileNum][0] = '\0';
        sprintf(saveFileLabels[fileNum], "%i. (New Game)", fileNum + 1);
        fileNum++;
    }
    for (int i = 0; i < MAX_SAVE_FILES; i++)
        filesMenuItems[i].text = saveFileLabels[i];
    filesMenuItems[MAX_SAVE_FILES].text = "Back";
    menu_init(&filesMenu);
    set_main_callback(files_menu_main);
    set_draw_callback(files_menu_draw);
}

//==================================================
// Main Menu
//==================================================

static void main_menu_main(void)
{
    switch (menu_process_input())
    {
        case MENU_CANCEL:
            menu_wait_close_anim(start_title_screen);
            break;
        case 0:  //Start Game
            menu_wait_close_anim(files_menu_init);
            break;
        case 1:  //Exit to Homebrew Channel
            exit(0);
            break;
    }
}

static void main_menu_draw(void)
{
    draw_title_background();
    menu_draw();
}

static void main_menu_init(void)
{
    menu_init(&mainMenu);
    set_main_callback(main_menu_main);
    set_draw_callback(main_menu_draw);
}

//==================================================
// Title Screen
//==================================================

static void title_screen_main(void)
{
    if (menu_msgbox_is_open())
    {
        menu_msgbox_process_input();
    }
    else
    {
        if (gControllerPressedKeys & PAD_BUTTON_START)
            main_menu_init();
    }
}

static void title_screen_draw(void)
{
    draw_title_background();
    if (menu_msgbox_is_open())
    {
        menu_draw();
    }
    else
    {
        if (!(pressStartBlinkCounter & 0x20))
        {
            text_set_font_size(16, 32);
            text_draw_string(gDisplayWidth / 2, 300, TEXT_HCENTER, "Press Start");
        }
        pressStartBlinkCounter++;
    }
}

static void start_title_screen(void)
{
    drawing_set_2d_mode();
    set_main_callback(title_screen_main);
    set_draw_callback(title_screen_draw);
    pressStartBlinkCounter = 0;
}

void title_screen_init(void)
{
    //initialize background world
    strcpy(gSaveFile.seed, "12345");
    gSaveFile.modifiedChunks = NULL;
    gSaveFile.modifiedChunksCount = 0;
    
    if (file_get_error() != NULL)
        menu_msgbox_init(file_get_error());
    world_init();
    start_title_screen();
}

void title_screen_load_textures(void)
{
    TPL_OpenTPLFromMemory(&titleTPL, (void *)title_tpl, title_tpl_size);
    TPL_GetTexture(&titleTPL, titleTextureId, &titleTexture);
    GX_InitTexObjFilterMode(&titleTexture, GX_NEAR, GX_NEAR);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_InvalidateTexAll();
}
